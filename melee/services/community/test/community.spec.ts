import { env } from "cloudflare:workers";
import { beforeAll, describe, expect, it, vi } from "vitest";
import { exportJWK, generateKeyPair, SignJWT } from "jose";
import migration from "../migrations/0001_community.sql?raw";
import feedbackCapacity from "../migrations/0002_feedback_uploads.sql?raw";
import worker from "../src/index";
import { adminNews, publicNews } from "../src/news";
import { adminFeedback, submitFeedback } from "../src/feedback";
import { HttpError } from "../src/http";
import { cleanAbandonedAttachments } from "../src/cleanup";
import { crc32, deflateSync } from "node:zlib";
import { feedbackImageSize } from "../src/feedback-image";

const testEnv = env as CloudflareBindings;
const request = (path: string, body?: unknown) => new Request(`https://community.test${path}`, body === undefined ? undefined : {
  method: "POST", headers: { "Content-Type": "application/json", "Content-Length": String(new TextEncoder().encode(JSON.stringify(body)).length) },
  body: JSON.stringify(body),
});
async function body(response: Response): Promise<Record<string, unknown>> { return response.json(); }

beforeAll(async () => {
  await testEnv.COMMUNITY.exec(migration.replace(/\n/g, " "));
  await testEnv.COMMUNITY.exec(feedbackCapacity.replace(/\n/g, " "));
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

  it("preserves the audience on edits and sends an explicit resend to the current audience", async () => {
    const input = { revision: 0, title: "Maintenance", body: "Soon.", news_id: null, published_at: Date.now()-100, expires_at: null, withdrawn: false, resend: false };
    const made = await body(await adminNews(request("/admin/api/announcements", input), testEnv));
    const key = String(made.id);
    const row = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    expect(row.published_at).toBe(row.updated_at);
    const lateLaunch = Number(row.published_at) + 1;
    expect((await body(await publicNews(request(`/v1/feed?since=${lateLaunch}`), testEnv))).announcements).toEqual([]);
    expect((await body(await publicNews(request(`/v1/announcements?since=${lateLaunch}`), testEnv))).entries).toEqual([]);
    const oldDelivery = `/v1/announcements/${row.delivery_id}`;
    await expect(publicNews(request(`${oldDelivery}?since=${lateLaunch}`), testEnv)).rejects.toMatchObject({status:404});
    const earlyLaunch = Number(row.published_at);
    expect((await body(await publicNews(request(`/v1/feed?since=${earlyLaunch}`), testEnv))).announcements)
      .toEqual([expect.objectContaining({ delivery_id: row.delivery_id })]);
    expect((await body(await publicNews(request(`${oldDelivery}?since=${earlyLaunch}`), testEnv))).published_at)
      .toBe(row.published_at);
    await adminNews(request(`/admin/api/announcements/${key}`, { ...input, revision: 1, body: "Later.", published_at: Date.now()+60_000 }), testEnv);
    const edited = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    expect(edited.delivery_id).toBe(row.delivery_id);
    expect(edited.published_at).toBe(row.published_at);
    await adminNews(request(`/admin/api/announcements/${key}`, { ...input, revision: 2, resend: true }), testEnv);
    const resent = await body(await adminNews(request(`/admin/api/announcements/${key}`), testEnv));
    expect(resent.delivery_id).not.toBe(row.delivery_id);
    expect(resent.published_at).toBe(resent.updated_at);
    expect(Number(resent.published_at)).toBeGreaterThanOrEqual(lateLaunch);
    expect((await body(await publicNews(request(`/v1/feed?since=${lateLaunch}`), testEnv))).announcements)
      .toEqual([expect.objectContaining({ delivery_id: resent.delivery_id })]);
    expect((await body(await publicNews(request(`/v1/feed?since=${lateLaunch}`), testEnv))).headline)
      .toMatchObject({title: "Edited update"});
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
    const filtered = await body(await publicNews(request(`/v1/feed?since=${at+1}`), testEnv));
    expect(filtered.announcements).toEqual([]);
    expect(filtered.announcements_next).toBeNull();
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
    const token = (claims: Record<string, unknown> = {}, expires: string | null = "1h") => {
      const assertion = new SignJWT({ email: settings.ADMIN_EMAIL, ...claims })
        .setProtectedHeader({ alg: "RS256", kid: "test" }).setIssuer(`https://${settings.ACCESS_TEAM_DOMAIN}`)
        .setAudience(String(claims.aud ?? settings.ACCESS_AUDIENCE));
      if (expires) assertion.setExpirationTime(expires);
      return assertion.sign(privateKey);
    };
    const read = (jwt: string) => worker.fetch(new Request("https://community.test/admin/api/news", {
      headers: { "Cf-Access-Jwt-Assertion": jwt },
    }), settings);
    try {
      expect((await worker.fetch(request("/admin/api/news"), settings)).status).toBe(401);
      expect((await read(await token())).status).toBe(200);
      expect((await read(await token({ email: "someone-else@example.com" }))).status).toBe(403);
      expect((await read(await token({ aud: "another-application" }))).status).toBe(403);
      expect((await read(await token({}, "-1h"))).status).toBe(403);
      expect((await read(await token({}, null))).status).toBe(403);
      for (const suffix of ["", "/screen", "/context", "/context?preview=1"]) {
        expect((await worker.fetch(request(`/admin/api/feedback/${crypto.randomUUID()}${suffix}`), settings)).status).toBe(401);
      }
      const write = request("/admin/api/news", { revision: 0, title: "Signed article", ticker: "", body: "Hello.", published_at: null, archived: false });
      write.headers.set("Cf-Access-Jwt-Assertion", await token());
      expect((await worker.fetch(write.clone(), settings)).status).toBe(403);
      write.headers.set("Origin", "https://community.test");write.headers.set("X-Freak-Admin", "1");
      expect((await worker.fetch(write, settings)).status).toBe(201);
    } finally { remote.mockRestore(); }
  });
});

function screenshot(rows = new Uint8Array([1, 32, 64, 128, 0, 20, 40, 60]), width = 1, height = 2) {
  function chunk(name: string, data: Uint8Array) {
    const result = new Uint8Array(data.length + 12), view = new DataView(result.buffer);
    view.setUint32(0, data.length);
    result.set(new TextEncoder().encode(name), 4);
    result.set(data, 8);
    view.setUint32(data.length + 8, crc32(result.subarray(4, data.length + 8)));
    return result;
  }
  const header = Uint8Array.of(0,0,0,1, 0,0,0,2, 8,2,0,0,0);
  const dimensions = new DataView(header.buffer);
  dimensions.setUint32(0, width);
  dimensions.setUint32(4, height);
  const data = deflateSync(rows);
  const chunks = [Uint8Array.of(137,80,78,71,13,10,26,10), chunk("IHDR", header),
    chunk("IDAT", data.subarray(0, 3)), chunk("IDAT", data.subarray(3)), chunk("IEND", new Uint8Array())];
  const result = new Uint8Array(chunks.reduce((length, part) => length + part.length, 0));
  let offset = 0;
  for (const part of chunks) { result.set(part, offset); offset += part.length; }
  return result;
}

function report(id: string, pinText = "Fix here", png = screenshot(),
  context = new TextEncoder().encode('{"frame":123,"motion":"Dash"}')): Request {
  const metadata = new TextEncoder().encode(JSON.stringify({ id, capture_at: "2026-11-13T12:00:00Z", build: "test-build", pins: [{ id: 0, x: .25, y: .75, text: pinText }] }));
  const bytes = new Uint8Array(20+metadata.length+png.length+context.length);
  bytes.set(new TextEncoder().encode("FMREPORT"));
  const view = new DataView(bytes.buffer);
  view.setUint32(8,metadata.length,true);view.setUint32(12,png.length,true);view.setUint32(16,context.length,true);
  bytes.set(metadata,20);bytes.set(png,20+metadata.length);bytes.set(context,20+metadata.length+png.length);
  return new Request("https://community.test/v1/feedback", { method: "POST", body: bytes,
    headers: { "Content-Type": "application/octet-stream", "Content-Length": String(bytes.length), "CF-Connecting-IP": id } });
}

describe("feedback storage", () => {
  it("checks the complete capture before creating feedback or storing attachments", async () => {
    const image = screenshot(), key = crypto.randomUUID();
    expect(await feedbackImageSize(image)).toEqual([1, 2]);
    const rows = new Uint8Array((1920 * 3 + 1) * 40).fill(64);
    for (let row = 0; row < 40; ++row) rows[row * (1920 * 3 + 1)] = row % 5;
    expect(await feedbackImageSize(screenshot(rows, 1920, 40))).toEqual([1920, 40]);
    const badChecksum = image.slice(); badChecksum[32] ^= 1;
    for (const incomplete of [image.slice(0, 33), image.slice(0, -1), badChecksum,
      screenshot(new Uint8Array(4)), screenshot(new Uint8Array(9)), screenshot(Uint8Array.of(0,0,0,0,5,0,0,0))]) {
      await expect(feedbackImageSize(incomplete)).rejects.toMatchObject({ status: 400 });
    }
    await expect(submitFeedback(report(key, "capture", image.slice(0, 33)), testEnv)).rejects.toMatchObject({ status: 400 });
    expect(await testEnv.COMMUNITY.prepare("SELECT id FROM feedback WHERE id=?").bind(key).first()).toBeNull();
    expect((await testEnv.REPORTS.list({ prefix: `feedback/${key}/` })).objects).toHaveLength(0);
    expect((await testEnv.COMMUNITY.prepare("SELECT count(*) n FROM feedback_uploads").first())?.n).toBe(0);
    await expect(submitFeedback(report("-".repeat(36)), testEnv)).rejects.toMatchObject({ status: 400 });
  });
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

  it("reserves space for concurrent captures and releases it when a report is deleted", async () => {
    const ids = [crypto.randomUUID(), crypto.randomUUID()];
    const amount = screenshot().length + new TextEncoder().encode('{"frame":123,"motion":"Dash"}').length;
    const used = Number(await testEnv.COMMUNITY.prepare(
      "SELECT coalesce(sum(screenshot_size+context_size),0) AS bytes FROM feedback").first("bytes"));
    const reserve = testEnv.COMMUNITY.prepare("INSERT INTO feedback_uploads VALUES(?,?,unixepoch()*1000+?)");
    await reserve.bind("other-upload", 2 * 1024 ** 3 - used - amount, 600_000).run();
    await reserve.bind("expired-upload", 2 * 1024 ** 3, -1000).run();
    try {
      const results = await Promise.all(ids.map(id => worker.fetch(report(id), testEnv)));
      expect(results.map(r => r.status).sort()).toEqual([201, 503]);
      const won = results[0].status === 201 ? 0 : 1;
      expect((await worker.fetch(report(ids[won]), testEnv)).status).toBe(200);
      expect((await testEnv.REPORTS.list({ prefix: `feedback/${ids[1-won]}/` })).objects).toHaveLength(0);
      await adminFeedback(new Request(`https://community.test/admin/api/feedback/${ids[won]}`, { method: "DELETE" }), testEnv);
      expect((await worker.fetch(report(ids[1-won]), testEnv)).status).toBe(201);
      await cleanAbandonedAttachments(testEnv);
      expect(await testEnv.COMMUNITY.prepare("SELECT bytes FROM feedback_uploads WHERE object_prefix='expired-upload'").first()).toBeNull();
    } finally {
      await testEnv.COMMUNITY.prepare("DELETE FROM feedback_uploads WHERE object_prefix IN ('other-upload','expired-upload')").run();
    }
  });

  it("keeps both attachments if the database commit succeeds but its acknowledgement is lost", async () => {
    const key = crypto.randomUUID();
    const db = testEnv.COMMUNITY;
    const uncertain = { ...testEnv, COMMUNITY: {
      prepare: db.prepare.bind(db),
      async batch(statements: D1PreparedStatement[]) {
        await db.batch(statements);
        throw new Error("Acknowledgement unavailable");
      },
    } as D1Database };
    await expect(submitFeedback(report(key), uncertain)).rejects.toThrow("Acknowledgement unavailable");
    expect((await submitFeedback(report(key), testEnv)).status).toBe(200);
    expect((await testEnv.REPORTS.list({ prefix: `feedback/${key}/` })).objects).toHaveLength(2);
    for (const attachment of ["screen", "context"])
      expect((await adminFeedback(request(`/admin/api/feedback/${key}/${attachment}`), testEnv)).status).toBe(200);
  });

  it("reserves one inbox slot per upload and lets abandoned reservations expire", async () => {
    const key = crypto.randomUUID();
    await testEnv.COMMUNITY.exec(`
      WITH RECURSIVE slots(i) AS (VALUES(1) UNION ALL SELECT i+1 FROM slots WHERE i<4096)
      INSERT INTO feedback_uploads SELECT 'occupied-'||i,1,unixepoch()*1000+60000 FROM slots`.replace(/\n/g, " "));
    try {
      expect((await worker.fetch(report(key), testEnv)).status).toBe(503);
      expect((await testEnv.REPORTS.list({ prefix: `feedback/${key}/` })).objects).toHaveLength(0);
      await testEnv.COMMUNITY.prepare("UPDATE feedback_uploads SET expires_at=0 WHERE object_prefix LIKE 'occupied-%'").run();
      expect((await worker.fetch(report(key), testEnv)).status).toBe(201);
      await cleanAbandonedAttachments(testEnv);
      expect(await testEnv.COMMUNITY.prepare("SELECT object_prefix FROM feedback_uploads LIMIT 1").first()).toBeNull();
    } finally {
      await testEnv.COMMUNITY.prepare("DELETE FROM feedback_uploads WHERE object_prefix LIKE 'occupied-%'").run();
    }
  });

  it("displays a bounded diagnostic preview and preserves the complete download", async () => {
    const key = crypto.randomUUID(), context = new Uint8Array(75_000).fill(65);
    expect((await submitFeedback(report(key, "Context preview", screenshot(), context), testEnv)).status).toBe(201);
    const route = `/admin/api/feedback/${key}/context`;
    const preview = await adminFeedback(request(route + "?preview=1"), testEnv);
    expect(new Uint8Array(await preview.arrayBuffer())).toEqual(context.subarray(0, 65536));
    const full = await adminFeedback(request(route), testEnv);
    expect(new Uint8Array(await full.arrayBuffer())).toEqual(context);
  });
});
