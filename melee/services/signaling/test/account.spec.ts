import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import type { PublicKeyCredentialCreationOptionsJSON, PublicKeyCredentialRequestOptionsJSON } from "@simplewebauthn/server";
import { authenticator } from "./account-authenticator";
import { isoBase64URL } from "@simplewebauthn/server/helpers";

const worker = exports.default as unknown as Fetcher;
interface Profile { id: string; handle: string; name: string; bio: string }
interface Grant { profile: Profile; token: string; expires: number; recovery_code: string }
interface Options<T> { attempt: string; origin: string; options: T }

function fixture() {
  const handle = "test_" + crypto.randomUUID().replaceAll("-", "").slice(0,18);
  const headers = { "Content-Type": "application/json", "CF-Connecting-IP": crypto.randomUUID() };
  async function request(path: string, body?: unknown, token?: string) {
    return worker.fetch(`https://signal.test/v1/accounts/${handle}/${path}`, {
      method: body === undefined ? "GET" : "POST", headers: { ...headers, ...(token ? { Authorization: `Bearer ${token}` } : {}) },
      body: body === undefined ? undefined : JSON.stringify(body),
    });
  }
  async function registration(kind = "register", body = {}, token?: string) {
    const result = await request(`${kind}/options`, body, token);
    expect(result.status).toBe(200);
    return result.json<Options<PublicKeyCredentialCreationOptionsJSON>>();
  }
  async function authentication(kind = "login", token?: string) {
    const result = await request(`${kind}/options`, {}, token);
    expect(result.status).toBe(200);
    return result.json<Options<PublicKeyCredentialRequestOptionsJSON>>();
  }
  async function create() {
    const device = await authenticator();
    const start = await registration();
    const response = await request("register/finish", { attempt: start.attempt,
      credential: await device.register(start.options, start.origin), device: "First PC" });
    expect(response.status).toBe(200);
    return { device, grant: await response.json<Grant>() };
  }
  async function login(device: Awaited<ReturnType<typeof authenticator>>) {
    const start = await authentication();
    const result = await request("login/finish", { attempt: start.attempt,
      credential: await device.sign(start.options, start.origin), device: "Another PC" });
    expect(result.status).toBe(200);
    return result.json<Grant>();
  }
  return { handle, request, registration, authentication, create, login,
    stub: env.ACCOUNTS.getByName(handle) };
}

describe("persistent passkey accounts", () => {
  it("registers and signs in with real signatures, retaining identity through hibernation", async () => {
    const f = fixture(), { device, grant } = await f.create();
    expect(grant.profile).toMatchObject({ handle: f.handle, name: f.handle, bio: "" });
    expect(grant.profile.id).toHaveLength(43);
    expect(grant.token).toHaveLength(43); expect(grant.recovery_code).toHaveLength(43);
    const me = await f.request("me", undefined, grant.token);
    expect(me.status).toBe(200);
    expect((await me.json<{ profile: Profile }>()).profile.id).toBe(grant.profile.id);
    await evictDurableObject(f.stub);
    const second = await f.login(device);
    expect(second.profile.id).toBe(grant.profile.id);
    expect(second.token).not.toBe(grant.token);
    expect((await f.request("profile", { name: "Twilight", bio: "Hello!\nI make fighters." }, second.token)).status).toBe(200);
    const publicProfile = await (await f.request("profile")).json<{ profile: Profile }>();
    expect(publicProfile.profile).toMatchObject({ name: "Twilight", bio: "Hello!\nI make fighters." });
    expect((await f.request("logout", {}, second.token)).status).toBe(200);
    expect((await f.request("me", undefined, second.token)).status).toBe(401);
    expect((await f.request("me", undefined, grant.token)).status).toBe(200);
    const records = await runInDurableObject(f.stub, (_instance, state) => ({
      account: state.storage.sql.exec("SELECT * FROM account").toArray(),
      keys: state.storage.sql.exec("SELECT * FROM passkeys").toArray(),
      sessions: state.storage.sql.exec("SELECT * FROM sessions").toArray(),
    }));
    expect(JSON.stringify(records)).not.toContain(grant.token);
    expect(JSON.stringify(records)).not.toContain(grant.recovery_code);
    expect(Object.keys(records.keys[0]).sort()).toEqual(["counter", "created", "id", "label", "public_key"]);
  });

  it.each(["origin", "rp", "verification", "presence", "challenge"])("rejects bad registration %s", async field => {
    const f = fixture(), device = await authenticator(), start = await f.registration();
    const options = { ...start.options, challenge: field === "challenge" ? isoBase64URL.fromBuffer(new Uint8Array(32)) : start.options.challenge };
    const credential = await device.register(options, field === "origin" ? "https://other.test" : start.origin,
      field === "verification" ? 0x41 : field === "presence" ? 0x44 : 0x45,
      field === "rp" ? "other.test" : start.options.rp.id!);
    const result = await f.request("register/finish", { attempt: start.attempt, credential });
    expect(result.status).toBe(400);
    expect((await f.request("profile")).status).toBe(404);
    expect((await f.request("register/finish", { attempt: start.attempt, credential })).status).toBe(400);
  });

  it.each(["signature", "origin", "rp", "verification", "challenge", "user"])("rejects bad sign-in %s", async field => {
    const f = fixture(), { device } = await f.create(), start = await f.authentication();
    const options = { ...start.options, challenge: field === "challenge" ? isoBase64URL.fromBuffer(new Uint8Array(32)) : start.options.challenge };
    const response = await device.sign(options, field === "origin" ? "https://other.test" : start.origin,
      field === "verification" ? 1 : 5, field === "rp" ? "other.test" : start.options.rpId!);
    if (field === "signature") response.response.signature = isoBase64URL.fromBuffer(new Uint8Array(64));
    if (field === "user") response.response.userHandle = isoBase64URL.fromBuffer(new Uint8Array(32));
    expect((await f.request("login/finish", { attempt: start.attempt, credential: response })).status).toBe(401);
  });

  it("consumes challenges once and admits only one concurrent registration for a handle", async () => {
    const f = fixture(), a = await authenticator(), b = await authenticator();
    const first = await f.registration(), second = await f.registration();
    const results = await Promise.all([
      f.request("register/finish", { attempt: first.attempt, credential: await a.register(first.options, first.origin) }),
      f.request("register/finish", { attempt: second.attempt, credential: await b.register(second.options, second.origin) }),
    ]);
    expect(results.map(result => result.status).sort()).toEqual([200, 409]);
    expect((await f.request("register/options", {})).status).toBe(409);
  });

  it("adds/removes passkeys and requires fresh approval for security changes", async () => {
    const f = fixture(), { device, grant } = await f.create(), replacement = await authenticator();
    const add = await f.registration("add", {}, grant.token);
    expect((await f.request("add/finish", { attempt: add.attempt,
      credential: await replacement.register(add.options, add.origin), passkey_name: "Backup key" }, grant.token)).status).toBe(200);
    await f.login(replacement);
    await runInDurableObject(f.stub, (_instance, state) => {
      state.storage.sql.exec("UPDATE sessions SET verified=0");
    });
    expect((await f.request("recovery/rotate", {}, grant.token)).status).toBe(403);
    const start = await f.authentication("reauth", grant.token);
    expect((await f.request("reauth/finish", { attempt: start.attempt,
      credential: await device.sign(start.options, start.origin) }, grant.token)).status).toBe(200);
    expect((await f.request("passkeys/remove", { id: device.id }, grant.token)).status).toBe(200);
    expect((await f.request("passkeys/remove", { id: replacement.id }, grant.token)).status).toBe(409);
    await f.login(replacement);
  });

  it("recovers with a new passkey, rotates the code and revokes previous sessions and passkeys", async () => {
    const f = fixture(), { device, grant } = await f.create(), replacement = await authenticator();
    const pendingLogin = await f.authentication();
    const oldProof = await device.sign(pendingLogin.options, pendingLogin.origin);
    const begin = await f.registration("recover", { recovery_code: grant.recovery_code });
    const result = await f.request("recover/finish", { attempt: begin.attempt,
      credential: await replacement.register(begin.options, begin.origin) });
    expect(result.status).toBe(200);
    const recovered = await result.json<Grant>();
    expect(recovered.profile.id).toBe(grant.profile.id);
    expect(recovered.recovery_code).not.toBe(grant.recovery_code);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.request("recover/options", { recovery_code: grant.recovery_code })).status).toBe(401);
    expect((await f.request("login/finish", { attempt: pendingLogin.attempt, credential: oldProof })).status).toBe(400);
    await f.login(replacement);
    const old = await f.authentication();
    expect((await f.request("login/finish", { attempt: old.attempt,
      credential: await device.sign(old.options, old.origin) })).status).toBe(401);
    expect((await f.request("sessions/revoke-all", {}, recovered.token)).status).toBe(200);
    expect((await f.request("me", undefined, recovered.token)).status).toBe(401);
  });

  it("rejects expired attempts/sessions and bounds untrusted input", async () => {
    const f = fixture(), { device, grant } = await f.create(), start = await f.authentication();
    await runInDurableObject(f.stub, (_instance, state) => {
      state.storage.sql.exec("UPDATE challenges SET expires=0; UPDATE sessions SET expires=0;");
    });
    expect((await f.request("login/finish", { attempt: start.attempt,
      credential: await device.sign(start.options, start.origin) })).status).toBe(400);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.request("login/options", { ignored: "x".repeat(65536) })).status).toBe(413);
    expect((await f.request("login/options", { ignored: true })).headers.get("Cache-Control")).toBe("no-store");
    expect((await worker.fetch("https://signal.test/v1/accounts/bad-name/login/options", { method: "POST" })).status).toBe(400);
    const cancel = await f.authentication();
    expect((await f.request("cancel", { attempt: cancel.attempt })).status).toBe(200);
    expect((await f.request("login/finish", { attempt: cancel.attempt,
      credential: await device.sign(cancel.options, cancel.origin) })).status).toBe(400);
  });

  it("rejects replay, cross-origin responses and stale authenticator counters", async () => {
    const f = fixture(), { device } = await f.create();
    const start = await f.authentication();
    const proof = await device.sign(start.options, start.origin);
    const body = { attempt: start.attempt, credential: proof };
    const outcomes = await Promise.all([f.request("login/finish", body), f.request("login/finish", body)]);
    expect(outcomes.map(response => response.status).sort()).toEqual([200, 400]);
    const stale = await f.authentication();
    expect((await f.request("login/finish", { attempt: stale.attempt,
      credential: await device.sign(stale.options, stale.origin, 5, stale.options.rpId!, 1) })).status).toBe(401);
    const cross = await f.authentication();
    const credential = await device.sign(cross.options, cross.origin);
    const data = JSON.parse(isoBase64URL.toUTF8String(credential.response.clientDataJSON));
    data.crossOrigin = true;
    credential.response.clientDataJSON = isoBase64URL.fromUTF8String(JSON.stringify(data));
    expect((await f.request("login/finish", { attempt: cross.attempt, credential })).status).toBe(400);
  });

  it("bounds parallel pending attempts and persisted device sessions", async () => {
    const f = fixture(), { device, grant } = await f.create();
    const starts = await Promise.all(Array.from({ length: 18 }, () => f.request("login/options", {})));
    expect(starts.filter(result => result.status === 200)).toHaveLength(16);
    expect(starts.filter(result => result.status === 429)).toHaveLength(2);
    // Release these test requests, then consume the attempts in sequence as
    // separate device sign-ins. No synthetic session rows are inserted.
    for (const response of starts) {
      if (response.status !== 200) { await response.text(); continue; }
      const start = await response.json<Options<PublicKeyCredentialRequestOptionsJSON>>();
      const result = await f.request("login/finish", { attempt: start.attempt,
        credential: await device.sign(start.options, start.origin) });
      expect(result.status).toBe(200); await result.text();
    }
    const count = await runInDurableObject(f.stub, (_instance, state) =>
      state.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM sessions").one().count);
    expect(count).toBe(16);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
  });

  it("requires authentication and encrypted transport for account changes", async () => {
    const f = fixture(); await f.create();
    expect((await f.request("me")).status).toBe(401);
    expect((await f.request("profile", { name: "Someone else", bio: "" })).status).toBe(401);
    expect((await f.request("recover/options", { recovery_code: isoBase64URL.fromBuffer(new Uint8Array(32)) })).status).toBe(401);
    expect((await worker.fetch(`http://signal.test/v1/accounts/${f.handle}/profile`)).status).toBe(403);
    const responses = await Promise.all(Array.from({ length: 61 }, () =>
      worker.fetch(`https://signal.test/v1/accounts/${f.handle}/profile`, { headers: { "CF-Connecting-IP": "rate-test" } })));
    expect(responses.some(response => response.status === 429)).toBe(true);
    for (const response of responses) await response.text();
  });

  it("revokes one device and rotates recovery codes without changing account identity", async () => {
    const f = fixture(), { device, grant } = await f.create(), second = await f.login(device);
    const me = await (await f.request("me", undefined, second.token)).json<{
      sessions: { id: string; current: boolean }[];
    }>();
    const first = me.sessions.find(session => !session.current)!;
    expect((await f.request("sessions/revoke", { id: first.id }, second.token)).status).toBe(200);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    const rotated = await (await f.request("recovery/rotate", {}, second.token)).json<{ recovery_code: string }>();
    expect(rotated.recovery_code).toHaveLength(43);
    expect((await f.request("recover/options", { recovery_code: grant.recovery_code })).status).toBe(401);
    expect((await f.registration("recover", { recovery_code: rotated.recovery_code })).options.user.id).toBe(grant.profile.id);
  });
});
