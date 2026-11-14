import { env } from "cloudflare:workers";
import { beforeAll, describe, expect, it, vi } from "vitest";
import { exportJWK, generateKeyPair, SignJWT } from "jose";
import migration from "../migrations/0001_community.sql?raw";
import worker from "../src/index";
import { adminNews, publicNews } from "../src/news";
import { adminFeedback, submitFeedback } from "../src/feedback";
import { HttpError } from "../src/http";
import { cleanAbandonedAttachments } from "../src/cleanup";

const testEnv = env as CloudflareBindings;
const request = (path: string, body?: unknown) => new Request(`https://community.test${path}`, body === undefined ? undefined : {
  method: "POST", headers: { "Content-Type": "application/json", "Content-Length": String(new TextEncoder().encode(JSON.stringify(body)).length) },
  body: JSON.stringify(body),
});
async function body(response: Response): Promise<Record<string, unknown>> { return response.json(); }

beforeAll(async () => {
  await testEnv.COMMUNITY.exec(migration.replace(/\n/g, " "));
});

describe("community publishing", () => {
  it("routes the entry URL to the protected admin directory", async () => {
    const response = await worker.fetch(request("/"), testEnv);
    expect(response.status).toBe(302);
    expect(response.headers.get("Location")).toBe("https://community.test/admin/");
  });
  it("keeps drafts private and preserves publication order when editing", async () => {
    const input = { revision: 0, title: "First update", ticker: "A headline", body: "The article.", published_at: null, archived: false };
    const created = await body(await adminNews(request("/admin/api/news", input), testEnv));
    const key = String(created.id);
    const empty = await body(await publicNews(request("/v1/feed"), testEnv));
    expect(empty.headline).toBeNull();
    const date = Date.now() - 1000;
    await adminNews(request(`/admin/api/news/${key}`, { ...input, revision: 1, published_at: date }), testEnv);
    const published = await publicNews(request(`/v1/news/${key}`), testEnv);
    const etag = published.headers.get("ETag")!;
    expect((await body(published)).published_at).toBe(date);
    expect((await publicNews(new Request(`https://community.test/v1/news/${key}`, { headers: { "If-None-Match": etag } }), testEnv)).status).toBe(304);
    await adminNews(request(`/admin/api/news/${key}`, { ...input, title: "Edited update", revision: 2, published_at: date }), testEnv);
    await expect(adminNews(request(`/admin/api/news/${key}`, { ...input, revision: 2 }), testEnv)).rejects.toMatchObject({ status: 409 });
    const feed = await body(await publicNews(request("/v1/feed"), testEnv));
    expect(feed.headline).toMatchObject({ id: key, title: "Edited update", published_at: date });
  });

  it("changes delivery IDs only on an explicit resend and withdraws notices", async () => {
    const input = { revision: 0, title: "Maintenance", body: "Soon.", news_id: null, published_at: Date.now()-100, expires_at: null, withdrawn: false, resend: false };
    const made = await body(await adminNews(request("/admin/api/announcements", input), testEnv));
    const key = String(made.id);
    const row = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    await adminNews(request(`/admin/api/announcements/${key}`, { ...input, revision: 1, body: "Later." }), testEnv);
    const edited = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    expect(edited.delivery_id).toBe(row.delivery_id);
    await adminNews(request(`/admin/api/announcements/${key}`, { ...input, revision: 2, resend: true }), testEnv);
    const resent = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    expect(resent.delivery_id).not.toBe(row.delivery_id);
    await adminNews(request(`/admin/api/announcements/${key}`, { ...input, revision: 3, withdrawn: true }), testEnv);
    expect((await body(await publicNews(request("/v1/feed"), testEnv))).announcements).toEqual([]);
  });

  it("pages through every active announcement and refreshes an older delivery by ID", async () => {
    const at = Date.now() - 2000;
    const ids = Array.from({length: 130}, () => crypto.randomUUID()).sort().reverse();
    const deliveries = ids.map(() => crypto.randomUUID());
    await testEnv.COMMUNITY.batch(ids.map((key, i) => testEnv.COMMUNITY.prepare(
      "INSERT INTO announcements(id,delivery_id,title,body,published_at,updated_at) VALUES(?,?,?,?,?,?)"
    ).bind(key, deliveries[i], `Notice ${i}`, "Read this.", at, at)));
    const feed = await body(await publicNews(request("/v1/feed"), testEnv));
    expect((feed.announcements as unknown[]).length).toBe(64);
    const found = (feed.announcements as {id:string}[]).map(row => row.id);
    let next = feed.announcements_next;
    while (next) {
      const page = await body(await publicNews(request(`/v1/announcements?before=${next}`), testEnv));
      found.push(...(page.entries as {id:string}[]).map(row => row.id));
      next = page.next;
    }
    expect(found).toEqual(ids);
    const route = `/v1/announcements/${deliveries[100]}`;
    expect(await body(await publicNews(request(route), testEnv))).toMatchObject({title:"Notice 100"});
    await testEnv.COMMUNITY.prepare("UPDATE announcements SET body='Updated',revision=revision+1 WHERE id=?").bind(ids[100]).run();
    expect(await body(await publicNews(request(route), testEnv))).toMatchObject({body:"Updated",revision:2});
    await testEnv.COMMUNITY.prepare("UPDATE announcements SET withdrawn=1 WHERE id=?").bind(ids[100]).run();
    await expect(publicNews(request(route), testEnv)).rejects.toMatchObject({status:404});
    await testEnv.COMMUNITY.prepare("UPDATE announcements SET expires_at=? WHERE id=?").bind(at, ids[101]).run();
    await expect(publicNews(request(`/v1/announcements/${deliveries[101]}`), testEnv)).rejects.toMatchObject({status:404});
    await expect(publicNews(request("/v1/announcements?before=broken"), testEnv)).rejects.toMatchObject({status:400});
    await testEnv.COMMUNITY.batch(ids.map(key => testEnv.COMMUNITY.prepare("DELETE FROM announcements WHERE id=?").bind(key)));
  });

  it("denies admin routes when authentication is unconfigured", async () => {
    const response = await worker.fetch(request("/admin/api/news"), {
      ...testEnv, ACCESS_TEAM_DOMAIN: "", ACCESS_AUDIENCE: "",
    });
    expect(response.status).toBe(503);
  });

  it("checks signed identity, audience, expiry and write origin on admin routes", async () => {
    const { publicKey, privateKey } = await generateKeyPair("RS256");
    const key = await exportJWK(publicKey);
    const settings = { ...testEnv, ACCESS_TEAM_DOMAIN: "freak-test.cloudflareaccess.com", ACCESS_AUDIENCE: "test-admin" };
    const remote = vi.spyOn(globalThis, "fetch").mockResolvedValue(Response.json({ keys: [{ ...key, kid: "test", alg: "RS256", use: "sig" }] }));
    const token = (claims: Record<string, unknown> = {}, expires = "1h") => new SignJWT({ email: settings.ADMIN_EMAIL, ...claims })
      .setProtectedHeader({ alg: "RS256", kid: "test" }).setIssuer(`https://${settings.ACCESS_TEAM_DOMAIN}`)
      .setAudience(String(claims.aud ?? settings.ACCESS_AUDIENCE)).setExpirationTime(expires).sign(privateKey);
    const read = (jwt: string) => worker.fetch(new Request("https://community.test/admin/api/news", {
      headers: { "Cf-Access-Jwt-Assertion": jwt },
    }), settings);
    try {
      expect((await worker.fetch(request("/admin/api/news"), settings)).status).toBe(401);
      expect((await read(await token())).status).toBe(200);
      expect((await read(await token({ email: "someone-else@example.com" }))).status).toBe(403);
      expect((await read(await token({ aud: "another-application" }))).status).toBe(403);
      expect((await read(await token({}, "-1h"))).status).toBe(403);
      const write = request("/admin/api/news", { revision: 0, title: "Signed article", ticker: "", body: "Hello.", published_at: null, archived: false });
      write.headers.set("Cf-Access-Jwt-Assertion", await token());
      expect((await worker.fetch(write.clone(), settings)).status).toBe(403);
      write.headers.set("Origin", "https://community.test");write.headers.set("X-Freak-Admin", "1");
      expect((await worker.fetch(write, settings)).status).toBe(201);
    } finally { remote.mockRestore(); }
  });
});

function report(id: string, pinText = "Fix here"): Request {
  // Only the upload validator's PNG header contract is needed in this fixture.
  const png = new Uint8Array(33);
  png.set([137,80,78,71,13,10,26,10]);png.set(new TextEncoder().encode("IHDR"),12);
  new DataView(png.buffer).setUint32(16,640);new DataView(png.buffer).setUint32(20,480);
  const metadata = new TextEncoder().encode(JSON.stringify({ id, capture_at: "2026-11-13T12:00:00Z", build: "test-build", pins: [{ id: 0, x: .25, y: .75, text: pinText }] }));
  const context = new TextEncoder().encode('{"frame":123,"motion":"Dash"}');
  const bytes = new Uint8Array(20+metadata.length+png.length+context.length);
  bytes.set(new TextEncoder().encode("FMREPORT"));
  const view = new DataView(bytes.buffer);
  view.setUint32(8,metadata.length,true);view.setUint32(12,png.length,true);view.setUint32(16,context.length,true);
  bytes.set(metadata,20);bytes.set(png,20+metadata.length);bytes.set(context,20+metadata.length+png.length);
  return new Request("https://community.test/v1/feedback", { method: "POST", body: bytes,
    headers: { "Content-Type": "application/octet-stream", "Content-Length": String(bytes.length) } });
}

describe("feedback storage", () => {
  it("stores one report on retry, preserves capture time, and rejects changed notes under the same ID", async () => {
    const key = crypto.randomUUID();
    expect((await submitFeedback(report(key),testEnv)).status).toBe(201);
    expect((await submitFeedback(report(key),testEnv)).status).toBe(200);
    await expect(submitFeedback(report(key,"Different report"),testEnv)).rejects.toMatchObject({ status: 409 });
    const stored = await body(await adminFeedback(request(`/admin/api/feedback/${key}`),testEnv));
    expect(stored.capture_at).toBe("2026-11-13T12:00:00Z");
    expect(stored.notes).toEqual([{id:0,x:.25,y:.75,text:"Fix here"}]);
    expect((await adminFeedback(request(`/admin/api/feedback/${key}/screen`),testEnv)).headers.get("Content-Type")).toBe("image/png");
    const deletion = new Request(`https://community.test/admin/api/feedback/${key}`, { method: "DELETE" });
    expect((await adminFeedback(deletion,testEnv)).status).toBe(200);
    await expect(adminFeedback(request(`/admin/api/feedback/${key}`),testEnv)).rejects.toBeInstanceOf(HttpError);
  });

  it("collects abandoned attachments while retaining completed reports", async () => {
    const key = crypto.randomUUID();
    await submitFeedback(report(key), testEnv);
    const stored = await body(await adminFeedback(request(`/admin/api/feedback/${key}`), testEnv));
    const abandoned = `feedback/${crypto.randomUUID()}/abandoned/context.bin`;
    await testEnv.REPORTS.put(abandoned, "Uncommitted");
    await cleanAbandonedAttachments(testEnv);
    expect(await testEnv.REPORTS.head(abandoned)).not.toBeNull();
    const clock = vi.spyOn(Date, "now").mockReturnValue(Date.now() + 2 * 86400000);
    try { await cleanAbandonedAttachments(testEnv); }
    finally { clock.mockRestore(); }
    expect(await testEnv.REPORTS.head(abandoned)).toBeNull();
    expect(await testEnv.REPORTS.head(String(stored.screenshot_key))).not.toBeNull();
    expect(await testEnv.REPORTS.head(String(stored.context_key))).not.toBeNull();
  });
});
