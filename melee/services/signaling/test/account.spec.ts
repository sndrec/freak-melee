import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { Buffer } from "node:buffer";
import { accountKey, type KeyChallenge } from "./account-key";
import { secret } from "../src/account-protocol";

interface Profile { id: string; handle: string; name: string; bio: string }
interface Grant { profile: Profile; token: string; expires: number; recovery_code: string }
type Key = ReturnType<typeof accountKey>;
function fixture() {
  const handle = "test_" + crypto.randomUUID().replaceAll("-", "").slice(0, 18);
  const headers = { "Content-Type": "application/json", "CF-Connecting-IP": crypto.randomUUID() };
  async function request(path: string, body?: unknown, token?: string) {
    return exports.default.fetch(new Request(`https://signal.test/v1/accounts/${handle}/${path}`, {
      method: body === undefined ? "GET" : "POST", headers: { ...headers, ...(token ? { Authorization: `Bearer ${token}` } : {}) },
      body: body === undefined ? undefined : JSON.stringify(body),
    }), env);
  }
  async function challenge(kind: string, key: Key, body = {}, token?: string) {
    const response = await request(`${kind}/options`, { ...body, public_key: key.publicKey }, token);
    expect(response.status).toBe(200); return response.json<KeyChallenge>();
  }
  async function finish(kind: string, key: Key, start: KeyChallenge, token?: string) {
    const response = await request(`${kind}/finish`, key.proof(handle, start), token);
    expect(response.status).toBe(200); return response.json<Grant>();
  }
  async function create() {
    const key = accountKey(), grant = await finish("register", key, await challenge("register", key));
    return { key, grant };
  }
  async function login(key: Key) { return finish("login", key, await challenge("login", key)); }
  return { handle, request, challenge, finish, create, login, stub: env.ACCOUNTS.getByName(handle) };
}

describe("persistent account-key accounts", () => {
  it("stores sign-up data only for a valid request and retires abandoned registrations", async () => {
    const f = fixture(), key = accountKey();
    const hasTables = () => runInDurableObject(f.stub, (_instance, state) =>
      state.storage.sql.exec("SELECT name FROM sqlite_schema WHERE name='account'").toArray().length !== 0);
    async function status(promise: Promise<Response>) {
      const response = await promise;
      await response.arrayBuffer();
      return response.status;
    }
    expect(await status(f.request("profile"))).toBe(404);
    expect(await f.stub.lookupProfile()).toBeNull();
    expect(await status(f.request("login/options", {public_key:key.publicKey}))).toBe(404);
    expect(await status(f.request("register/options", {public_key:"invalid"}))).toBe(400);
    expect(await hasTables()).toBe(false);
    await f.challenge("register", key);
    expect(await hasTables()).toBe(true);
    await evictDurableObject(f.stub);
    await runDurableObjectAlarm(f.stub);
    expect(await hasTables()).toBe(true);
    await runInDurableObject(f.stub, (_instance, state) => state.storage.sql.exec("UPDATE key_challenges SET expires=0").toArray());
    await runDurableObjectAlarm(f.stub);
    expect(await hasTables()).toBe(false);
    const { grant } = await f.create();
    await evictDurableObject(f.stub);
    await runDurableObjectAlarm(f.stub);
    expect((await f.request("me", undefined, grant.token)).status).toBe(200);
    expect((await f.stub.lookupProfile())?.id).toBe(grant.profile.id);
  });
  it("registers and signs in with real Ed25519 proofs, persisting public identity and public keys", async () => {
    const f = fixture(), { key, grant } = await f.create();
    expect(grant.profile).toMatchObject({ handle: f.handle, name: f.handle, bio: "" });
    expect(grant.token).toHaveLength(43); expect(grant.recovery_code).toHaveLength(43);
    await evictDurableObject(f.stub);
    const again = await f.login(key);
    expect(again.profile.id).toBe(grant.profile.id); expect(again.token).not.toBe(grant.token);
    expect((await f.request("profile", { name: "Twilight", bio: "Hello!" }, again.token)).status).toBe(200);
    const profile = await (await f.request("profile")).json<{profile: Profile}>();
    expect(profile.profile.name).toBe("Twilight");
    const records = await runInDurableObject(f.stub, (_instance, state) => JSON.stringify({
      keys: state.storage.sql.exec("SELECT * FROM account_keys").toArray(),
      sessions: state.storage.sql.exec("SELECT * FROM sessions").toArray(),
      account: state.storage.sql.exec("SELECT * FROM account").toArray(),
    }));
    expect(records).toContain(key.publicKey); expect(records).not.toContain(key.seed());
    expect(records).not.toContain(grant.token); expect(records).not.toContain(grant.recovery_code);
    const me = await (await f.request("me", undefined, grant.token)).json<{keys: {id: string}[]; sessions: {current: boolean}[]}>();
    expect(me.keys).toHaveLength(1); expect(me.keys[0].id).toBe(key.publicKey);
    expect(me.sessions).toHaveLength(2); expect(me.sessions.filter(session => session.current)).toHaveLength(1);
  });
  it("consumes challenges once and admits one concurrent registration for a handle", async () => {
    const f = fixture(), a = accountKey(), b = accountKey();
    const first = await f.challenge("register", a), second = await f.challenge("register", b);
    const responses = await Promise.all([f.request("register/finish", a.proof(f.handle, first)), f.request("register/finish", b.proof(f.handle, second))]);
    expect(responses.map(response => response.status).sort()).toEqual([200, 409]);
    expect((await f.request("register/finish", a.proof(f.handle, first))).status).toBe(400);
    expect((await f.request("register/options", { public_key: a.publicKey })).status).toBe(409);
  });
  it("requires key approval for security changes, adds keys and revokes sessions belonging to a removed key", async () => {
    const f = fixture(), { key, grant } = await f.create(), replacement = accountKey();
    await runInDurableObject(f.stub, (_instance, state) => { state.storage.sql.exec("UPDATE sessions SET verified=0"); });
    expect((await f.request("recovery/rotate", {}, grant.token)).status).toBe(403);
    await f.finish("reauth", key, await f.challenge("reauth", key, {}, grant.token), grant.token);
    expect((await f.request("keys/remove", { id: key.publicKey }, grant.token)).status).toBe(409);
    await f.finish("add", replacement, await f.challenge("add", replacement, { key_name: "Second key" }, grant.token), grant.token);
    const second = await f.login(replacement);
    expect((await f.request("keys/remove", { id: key.publicKey }, second.token)).status).toBe(200);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.request("login/options", { public_key: key.publicKey })).status).toBe(401);
    expect((await f.request("me", undefined, second.token)).status).toBe(200);
  });
  it("recovers with a replacement key, preserves identity and revokes prior keys and sessions", async () => {
    const f = fixture(), { key, grant } = await f.create(), replacement = accountKey();
    await f.request("profile", { name: "Twilight", bio: "Saved profile" }, grant.token);
    const cancelled = await f.challenge("recover", replacement, { recovery_code: grant.recovery_code });
    expect((await f.request("cancel", { attempt: cancelled.attempt })).status).toBe(200);
    expect((await f.request("recover/finish", replacement.proof(f.handle, cancelled))).status).toBe(400);
    const recovered = await f.finish("recover", replacement, await f.challenge("recover", replacement, { recovery_code: grant.recovery_code }));
    expect(recovered.profile).toMatchObject({ id: grant.profile.id, name: "Twilight", bio: "Saved profile" });
    expect(recovered.recovery_code).not.toBe(grant.recovery_code);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.request("recover/options", { public_key: replacement.publicKey, recovery_code: grant.recovery_code })).status).toBe(401);
    expect((await f.request("login/options", { public_key: key.publicKey })).status).toBe(401);
    expect((await f.login(replacement)).profile.id).toBe(grant.profile.id);
  });
  it("cancels a removed key's outstanding proofs, including after the same key is enrolled again", async () => {
    const f = fixture(), { key, grant } = await f.create(), retained = accountKey(), another = accountKey();
    await f.finish("add", retained, await f.challenge("add", retained, {}, grant.token), grant.token);
    const active = await f.login(retained);
    const login = await f.challenge("login", key);
    const add = await f.challenge("add", another, {}, grant.token);
    const keptLogin = await f.challenge("login", retained);
    expect((await f.request("keys/remove", {id: key.publicKey}, active.token)).status).toBe(200);
    await f.finish("add", key, await f.challenge("add", key, {}, active.token), active.token);
    expect((await f.request("login/finish", key.proof(f.handle, login))).status).toBe(400);
    expect((await f.request("add/finish", another.proof(f.handle, add), active.token)).status).toBe(400);
    expect((await f.finish("login", retained, keptLogin)).profile.id).toBe(grant.profile.id);
    expect((await f.login(key)).profile.id).toBe(grant.profile.id);
  });
  it("keeps key management and recovery available when public login slots fill", async () => {
    const f = fixture(), { key, grant } = await f.create(), replacement = accountKey();
    for (let i=0; i<16; ++i) await f.challenge("login", key);
    expect((await f.request("login/options", {public_key:key.publicKey})).status).toBe(429);
    for (let i=0; i<4; ++i) await f.challenge("reauth", key, {}, grant.token);
    expect((await f.request("reauth/options", {public_key:key.publicKey}, grant.token)).status).toBe(429);
    const recovery = {recovery_code:grant.recovery_code};
    const first = await f.challenge("recover", replacement, recovery);
    await f.challenge("recover", replacement, recovery);
    expect((await f.request("recover/options", {...recovery,public_key:replacement.publicKey})).status).toBe(429);
    const restored = await f.finish("recover", replacement, first);
    // Previous versions stored Web Crypto's SHA-256 digest, encoded as unpadded base64url.
    const hash = async (value: string) => Buffer.from(await crypto.subtle.digest("SHA-256",
      new TextEncoder().encode(value))).toString("base64url");
    const stored = await runInDurableObject(f.stub, (_instance,state) => ({
      recovery: state.storage.sql.exec("SELECT recovery_hash FROM account").one().recovery_hash,
      session: state.storage.sql.exec("SELECT hash FROM sessions").one().hash,
    }));
    expect(stored).toEqual({recovery: await hash(restored.recovery_code), session: await hash(restored.token)});
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.login(replacement)).profile.id).toBe(grant.profile.id);
  });
  it("rejects expired attempts/sessions and bounds untrusted JSON and key material", async () => {
    const f = fixture(), key = accountKey(), attempt = await f.challenge("register", key);
    await runInDurableObject(f.stub, (_instance, state) => { state.storage.sql.exec("UPDATE key_challenges SET expires=0"); });
    expect((await f.request("register/finish", key.proof(f.handle, attempt))).status).toBe(400);
    expect((await f.request("register/options", { public_key: "bad" })).status).toBe(400);
    expect((await f.request("register/options", { public_key: key.publicKey, name: "x".repeat(65537) })).status).toBe(413);
    const { grant } = await f.create();
    await runInDurableObject(f.stub, (_instance, state) => { state.storage.sql.exec("UPDATE sessions SET expires=0"); });
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
  });
  it("binds signed proofs to service, account, purpose, key and challenge", async () => {
    const f = fixture(), { key } = await f.create();
    for (const changes of [{ origin: "https://elsewhere.test" }, { purpose: "recover" }, { public_key: accountKey().publicKey }, { challenge: secret() }, { user_id: secret() }]) {
      const start = await f.challenge("login", key);
      expect((await f.request("login/finish", key.proof(f.handle, { ...start, ...changes }))).status).toBe(401);
    }
    const challenge = await f.challenge("login", key);
    expect((await f.request("login/finish", key.proof("another_account", challenge))).status).toBe(401);
    const other = await f.challenge("login", key);
    expect((await f.request("login/finish", accountKey().proof(f.handle, other))).status).toBe(401);
    const malformed = await f.challenge("login", key);
    expect((await f.request("login/finish", { attempt: malformed.attempt, signature: "x" })).status).toBe(400);
  });
  it("bounds parallel challenges and retained device sessions", async () => {
    const f = fixture(), { key } = await f.create();
    const attempts = await Promise.all(Array.from({ length: 20 }, () => f.request("login/options", { public_key: key.publicKey })));
    expect(attempts.filter(response => response.status === 200)).toHaveLength(16);
    expect(attempts.filter(response => response.status === 429)).toHaveLength(4);
    for (const response of attempts) if (response.status === 200) await f.finish("login", key, await response.json<KeyChallenge>());
    const record = await runInDurableObject(f.stub, (_instance, state) => state.storage.sql.exec<{count: number}>("SELECT COUNT(*) AS count FROM sessions").one());
    expect(record.count).toBe(16);
  });
  it("requires authenticated sessions and encrypted account transport", async () => {
    const f = fixture(); await f.create();
    expect((await f.request("profile", { name: "Wrong", bio: "" })).status).toBe(401);
    expect((await f.request("me", undefined, secret())).status).toBe(401);
    expect((await exports.default.fetch(new Request(`http://untrusted.test/v1/accounts/${f.handle}/profile`), env)).status).toBe(403);
    expect((await f.request("profile")).headers.get("Cache-Control")).toBe("no-store");
  });
  it("revokes devices and rotates recovery codes while keeping identity", async () => {
    const f = fixture(), { key, grant } = await f.create(), second = await f.login(key);
    const me = await (await f.request("me", undefined, grant.token)).json<{ sessions: {id: string; current: boolean}[] }>();
    await runInDurableObject(f.stub, (_instance, state) => { state.storage.sql.exec("UPDATE sessions SET verified=0"); });
    expect((await f.request("sessions/revoke", { id: me.sessions.find(session => !session.current)!.id }, grant.token)).status).toBe(403);
    expect((await f.request("me", undefined, second.token)).status).toBe(200);
    await f.finish("reauth", key, await f.challenge("reauth", key, {}, grant.token), grant.token);
    expect((await f.request("sessions/revoke", { id: me.sessions.find(session => !session.current)!.id }, grant.token)).status).toBe(200);
    expect((await f.request("me", undefined, second.token)).status).toBe(401);
    const rotated = await (await f.request("recovery/rotate", {}, grant.token)).json<{ recovery_code: string }>();
    expect(rotated.recovery_code).not.toBe(grant.recovery_code);
    expect((await f.request("sessions/revoke-all", {}, grant.token)).status).toBe(200);
    expect((await f.request("me", undefined, grant.token)).status).toBe(401);
    expect((await f.login(key)).profile.id).toBe(grant.profile.id);
  });
});
