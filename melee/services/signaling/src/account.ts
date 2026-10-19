import { DurableObject } from "cloudflare:workers";
import { generateRegistrationOptions, generateAuthenticationOptions,
  verifyRegistrationResponse, verifyAuthenticationResponse } from "@simplewebauthn/server";
import { isoBase64URL } from "@simplewebauthn/server/helpers";
import { AccountError, accountJson, accountHandle, secret, secretHash, secretValue, equalHash,
  text, readAccountBody, credentialResponse, CHALLENGE_MS, SESSION_MS, RECENT_AUTH_MS,
  MAX_PASSKEYS, MAX_SESSIONS, type Ceremony, type JsonObject } from "./account-protocol";
import type { SocialProfile } from "./social-protocol";
import type { ConnectionAccount } from "./connection-account";

interface AccountRow extends Record<string, SqlStorageValue> {
  user_id: string; display_name: string; bio: string; recovery_hash: string; epoch: number;
}
interface KeyRow extends Record<string, SqlStorageValue> {
  id: string; public_key: ArrayBuffer; counter: number; label: string; created: number;
}
interface SessionRow extends Record<string, SqlStorageValue> {
  hash: string; expires: number; verified: number; label: string; epoch: number;
}
interface ChallengeRow extends Record<string, SqlStorageValue> {
  id: string; kind: Ceremony; challenge: string; user_id: string; display_name: string;
  expires: number; epoch: number; session_hash: string; recovery_hash: string;
}

export class Account extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    this.ctx.storage.sql.exec(`
      CREATE TABLE IF NOT EXISTS account (
        id INTEGER PRIMARY KEY CHECK(id=1), user_id TEXT NOT NULL, display_name TEXT NOT NULL,
        bio TEXT NOT NULL, recovery_hash TEXT NOT NULL, epoch INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS passkeys (
        id TEXT PRIMARY KEY, public_key BLOB NOT NULL, counter INTEGER NOT NULL,
        label TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS sessions (
        hash TEXT PRIMARY KEY, expires INTEGER NOT NULL, verified INTEGER NOT NULL,
        label TEXT NOT NULL, epoch INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS challenges (
        id TEXT PRIMARY KEY, kind TEXT NOT NULL, challenge TEXT NOT NULL, user_id TEXT NOT NULL,
        display_name TEXT NOT NULL, expires INTEGER NOT NULL, epoch INTEGER NOT NULL,
        session_hash TEXT NOT NULL, recovery_hash TEXT NOT NULL);
    `);
  }

  private account(): AccountRow | null {
    return this.ctx.storage.sql.exec<AccountRow>("SELECT * FROM account WHERE id=1").toArray()[0] ?? null;
  }
  private keys(): KeyRow[] { return this.ctx.storage.sql.exec<KeyRow>("SELECT * FROM passkeys ORDER BY created, id").toArray(); }
  private profile() {
    const account = this.account();
    const handle = this.ctx.id.name;
    if (!account || !handle) throw new AccountError(404, "Account not found.");
    return { id: account.user_id, handle, name: account.display_name, bio: account.bio };
  }
  lookupProfile(): SocialProfile | null { return this.account() ? this.profile() : null; }
  async openConnection(authorization: string): Promise<ConnectionAccount | null> {
    try {
      const session = await this.authorize(authorization);
      return { profile: this.profile(), session: session.hash, epoch: session.epoch, expires: session.expires };
    } catch { return null; }
  }
  checkConnection(connection: ConnectionAccount): SocialProfile | null {
    const account = this.account();
    const session = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", connection.session).toArray()[0];
    return account && session && session.expires > Date.now() && session.epoch === account.epoch &&
      connection.epoch === account.epoch && connection.profile.id === account.user_id ? this.profile() : null;
  }
  private clean(): void {
    const now = Date.now();
    this.ctx.storage.sql.exec("DELETE FROM challenges WHERE expires<=?", now);
    this.ctx.storage.sql.exec("DELETE FROM sessions WHERE expires<=?", now);
  }
  private async authorize(authorization: string, recent = false): Promise<SessionRow> {
    const header = authorization;
    if (!/^Bearer [A-Za-z0-9_-]{43}$/.test(header)) throw new AccountError(401, "Please sign in.");
    const hash = await secretHash(header.slice(7));
    const session = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", hash).toArray()[0];
    const account = this.account();
    if (!session || !account || session.expires <= Date.now() || session.epoch !== account.epoch)
      throw new AccountError(401, "Please sign in again.");
    if (recent && session.verified < Date.now() - RECENT_AUTH_MS)
      throw new AccountError(403, "Approve your passkey again before changing account security.");
    return session;
  }
  private stillAuthorized(session: SessionRow, epoch: number): void {
    const active = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", session.hash).toArray()[0];
    if (!active || active.expires <= Date.now() || active.epoch !== epoch || this.account()?.epoch !== epoch)
      throw new AccountError(401, "Your sign-in changed. Please try again.");
  }
  private takeChallenge(id: unknown, kind: Ceremony): ChallengeRow {
    const rows = this.ctx.storage.sql.exec<ChallengeRow>(
      "DELETE FROM challenges WHERE id=? AND kind=? AND expires>? RETURNING *", secretValue(id), kind, Date.now()).toArray();
    if (!rows.length) throw new AccountError(400, "That sign-in attempt expired or was already used. Try again.");
    return rows[0];
  }

  private async options(request: Request, kind: Ceremony, body: JsonObject): Promise<Response> {
    const account = this.account();
    let session: SessionRow | undefined, recoveryHash = "";
    if (kind === "register") {
      if (account) throw new AccountError(409, "That account name is already taken.");
    } else if (!account) throw new AccountError(404, "Account not found.");
    if (kind === "add" || kind === "reauth") session = await this.authorize(request.headers.get("Authorization") ?? "", kind === "add");
    if (kind === "recover") {
      recoveryHash = await secretHash(secretValue(body.recovery_code));
      if (!account || !equalHash(recoveryHash, account.recovery_hash)) throw new AccountError(401, "Recovery code was not accepted.");
    }
    const pending = this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM challenges").one().count;
    if (pending >= 16) throw new AccountError(429, "Too many unfinished sign-in attempts. Please wait a moment.");
    const userId = account?.user_id ?? secret();
    const displayName = kind === "register" ? text(body.name ?? this.ctx.id.name, 32) : account!.display_name;
    if (!displayName) throw new AccountError(400, "Enter a display name.");
    const register = kind === "register" || kind === "add" || kind === "recover";
    const keys = this.keys();
    if (kind === "add" && keys.length >= MAX_PASSKEYS) throw new AccountError(409, "Remove a passkey before adding another.");
    const options = register ? await generateRegistrationOptions({
      rpName: "Freak Melee", rpID: this.env.AUTH_RP_ID, userName: this.ctx.id.name!,
      userID: isoBase64URL.toBuffer(userId), userDisplayName: displayName, timeout: CHALLENGE_MS,
      attestationType: "none", supportedAlgorithmIDs: [-7, -257],
      authenticatorSelection: { residentKey: "preferred", userVerification: "required" },
      excludeCredentials: kind === "recover" ? [] : keys.map(key => ({ id: key.id })),
    }) : await generateAuthenticationOptions({ rpID: this.env.AUTH_RP_ID, timeout: CHALLENGE_MS,
      userVerification: "required", allowCredentials: keys.map(key => ({ id: key.id })) });
    if (kind === "register" ? this.account() !== null : this.account()?.epoch !== account!.epoch)
      throw new AccountError(409, "The account changed. Please try again.");
    if (session) this.stillAuthorized(session, account!.epoch);
    if (this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM challenges").one().count >= 16)
      throw new AccountError(429, "Too many unfinished sign-in attempts. Please wait a moment.");
    const id = secret();
    this.ctx.storage.sql.exec("INSERT INTO challenges VALUES (?,?,?,?,?,?,?,?,?)", id, kind,
      options.challenge, userId, displayName, Date.now() + CHALLENGE_MS, account?.epoch ?? 0,
      session?.hash ?? "", recoveryHash);
    return accountJson({ attempt: id, options, origin: this.env.AUTH_ORIGIN });
  }

  private async sessionValues(label: unknown) {
    const token = secret(), recoveryCode = secret();
    return { token, hash: await secretHash(token), recoveryCode, recoveryHash: await secretHash(recoveryCode),
      expires: Date.now() + SESSION_MS, label: text(label ?? "Windows PC", 64) };
  }
  private insertSession(values: { hash: string; expires: number; label: string }, epoch: number): void {
    this.ctx.storage.sql.exec(`DELETE FROM sessions WHERE hash IN
      (SELECT hash FROM sessions ORDER BY verified DESC, hash LIMIT -1 OFFSET ?)`, MAX_SESSIONS - 1);
    this.ctx.storage.sql.exec("INSERT INTO sessions VALUES (?,?,?,?,?)", values.hash, values.expires, Date.now(), values.label, epoch);
  }

  private async finishRegistration(request: Request, kind: "register" | "add" | "recover", body: JsonObject): Promise<Response> {
    const session = kind === "add" ? await this.authorize(request.headers.get("Authorization") ?? "", true) : undefined;
    const challenge = this.takeChallenge(body.attempt, kind);
    if (session && session.hash !== challenge.session_hash) throw new AccountError(401, "Use the sign-in that started this request.");
    let verification;
    try {
      verification = await verifyRegistrationResponse({ response: credentialResponse(body, true),
        expectedChallenge: challenge.challenge, expectedOrigin: this.env.AUTH_ORIGIN,
        expectedRPID: this.env.AUTH_RP_ID, requireUserPresence: true,
        requireUserVerification: true, supportedAlgorithmIDs: [-7, -257] });
    } catch { throw new AccountError(400, "Passkey registration was not accepted. Please try again."); }
    if (!verification.verified || !verification.registrationInfo) throw new AccountError(400, "Passkey registration failed.");
    const credential = verification.registrationInfo.credential;
    const values = await this.sessionValues(body.device);
    const keyLabel = text(body.passkey_name ?? "Passkey", 64) || "Passkey";
    this.ctx.storage.transactionSync(() => {
      const account = this.account();
      if (kind === "register") {
        if (account) throw new AccountError(409, "That account name is already taken.");
        this.ctx.storage.sql.exec("INSERT INTO account VALUES (1,?,?,?,?,1)", challenge.user_id,
          challenge.display_name, "", values.recoveryHash);
      } else {
        if (!account || account.epoch !== challenge.epoch) throw new AccountError(409, "The account changed. Please try again.");
        if (kind === "recover") {
          if (!equalHash(account.recovery_hash, challenge.recovery_hash)) throw new AccountError(401, "Recovery code was already used.");
          this.ctx.storage.sql.exec("DELETE FROM passkeys; DELETE FROM sessions; DELETE FROM challenges;");
          this.ctx.storage.sql.exec("UPDATE account SET epoch=epoch+1, recovery_hash=? WHERE id=1", values.recoveryHash);
        } else {
          this.stillAuthorized(session!, challenge.epoch);
          if (this.keys().length >= MAX_PASSKEYS) throw new AccountError(409, "The account already has eight passkeys.");
        }
      }
      if (this.ctx.storage.sql.exec<KeyRow>("SELECT * FROM passkeys WHERE id=?", credential.id).toArray().length)
        throw new AccountError(409, "That passkey is already registered.");
      this.ctx.storage.sql.exec("INSERT INTO passkeys VALUES (?,?,?,?,?)", credential.id,
        credential.publicKey, credential.counter, keyLabel, Date.now());
      if (kind !== "add") this.insertSession(values, this.account()!.epoch);
    });
    return accountJson(kind === "add" ? { profile: this.profile() } : { profile: this.profile(),
      token: values.token, expires: values.expires, recovery_code: values.recoveryCode });
  }

  private async finishAuthentication(request: Request, kind: "login" | "reauth", body: JsonObject): Promise<Response> {
    const session = kind === "reauth" ? await this.authorize(request.headers.get("Authorization") ?? "") : undefined;
    const challenge = this.takeChallenge(body.attempt, kind);
    if (session && session.hash !== challenge.session_hash) throw new AccountError(401, "Use the sign-in that started this request.");
    const response = credentialResponse(body, false);
    if (response.response.userHandle && response.response.userHandle !== challenge.user_id)
      throw new AccountError(401, "This passkey belongs to another account.");
    const key = this.ctx.storage.sql.exec<KeyRow>("SELECT * FROM passkeys WHERE id=?", response.id).toArray()[0];
    if (!key) throw new AccountError(401, "Passkey was not accepted.");
    let verification;
    try {
      verification = await verifyAuthenticationResponse({ response, expectedChallenge: challenge.challenge,
        expectedOrigin: this.env.AUTH_ORIGIN, expectedRPID: this.env.AUTH_RP_ID, requireUserVerification: true,
        credential: { id: key.id, publicKey: new Uint8Array(key.public_key), counter: key.counter } });
    } catch { throw new AccountError(401, "Passkey sign-in was not accepted. Please try again."); }
    if (!verification.verified) throw new AccountError(401, "Passkey sign-in failed.");
    const values = await this.sessionValues(body.device);
    this.ctx.storage.transactionSync(() => {
      if (this.account()?.epoch !== challenge.epoch) throw new AccountError(401, "The account changed. Please sign in again.");
      const updated = this.ctx.storage.sql.exec<{ id: string }>(
        "UPDATE passkeys SET counter=? WHERE id=? AND counter=? RETURNING id", verification.authenticationInfo.newCounter, key.id, key.counter).toArray();
      if (!updated.length) throw new AccountError(401, "Passkey changed. Please try signing in again.");
      if (session) {
        this.stillAuthorized(session, challenge.epoch);
        this.ctx.storage.sql.exec("UPDATE sessions SET verified=? WHERE hash=?", Date.now(), session.hash);
      } else this.insertSession(values, challenge.epoch);
    });
    return accountJson(session ? { profile: this.profile() } : { profile: this.profile(), token: values.token, expires: values.expires });
  }

  private async manage(request: Request, route: string, body: JsonObject): Promise<Response> {
    const sensitive = route === "recovery/rotate" || route === "passkeys/remove" || route === "sessions/revoke-all";
    const session = await this.authorize(request.headers.get("Authorization") ?? "", sensitive);
    if (route === "me") return accountJson({ profile: this.profile(), expires: session.expires,
      passkeys: this.keys().map(key => ({ id: key.id, name: key.label, created: key.created })),
      sessions: this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions ORDER BY verified DESC").toArray()
        .map(active => ({ id: active.hash, name: active.label, expires: active.expires, current: active.hash === session.hash })) });
    if (route === "profile") {
      const name = text(body.name, 32), bio = text(body.bio, 256, true);
      if (!name) throw new AccountError(400, "Enter a display name.");
      this.ctx.storage.sql.exec("UPDATE account SET display_name=?, bio=? WHERE id=1", name, bio);
    } else if (route === "logout") this.ctx.storage.sql.exec("DELETE FROM sessions WHERE hash=?", session.hash);
    else if (route === "sessions/revoke") this.ctx.storage.sql.exec("DELETE FROM sessions WHERE hash=?", secretValue(body.id));
    else if (route === "sessions/revoke-all") this.ctx.storage.sql.exec(
      "UPDATE account SET epoch=epoch+1 WHERE id=1; DELETE FROM sessions; DELETE FROM challenges;");
    else if (route === "passkeys/remove") {
      if (this.keys().length <= 1) throw new AccountError(409, "Keep at least one passkey. Add a replacement first.");
      this.ctx.storage.sql.exec("DELETE FROM passkeys WHERE id=?", text(body.id, 1400));
    } else if (route === "recovery/rotate") {
      const code = secret(), hash = await secretHash(code);
      this.stillAuthorized(session, session.epoch);
      this.ctx.storage.sql.exec("UPDATE account SET recovery_hash=? WHERE id=1", hash);
      this.ctx.storage.sql.exec("DELETE FROM challenges WHERE kind='recover'");
      return accountJson({ recovery_code: code });
    } else throw new AccountError(404, "Not found.");
    return accountJson({ ok: true });
  }

  async fetch(request: Request): Promise<Response> {
    try {
      const path = new URL(request.url).pathname.split("/").slice(4).join("/");
      if (!this.ctx.id.name || !accountHandle(this.ctx.id.name)) throw new AccountError(400, "Invalid account name.");
      const origin = new URL(this.env.AUTH_ORIGIN);
      const local = origin.protocol === "http:" && origin.hostname === "localhost";
      if (origin.origin !== this.env.AUTH_ORIGIN || origin.hostname !== this.env.AUTH_RP_ID ||
          (origin.protocol !== "https:" && !local))
        throw new AccountError(503, "Account sign-in is not configured for this service.");
      this.clean();
      if (request.method === "GET") {
        if (path === "profile") return accountJson({ profile: this.profile() });
        if (path === "me") return await this.manage(request, path, {});
        throw new AccountError(404, "Not found.");
      }
      if (request.method !== "POST") throw new AccountError(405, "Method not allowed.");
      const body = await readAccountBody(request);
      if (path.startsWith("social/")) {
        const session = await this.authorize(request.headers.get("Authorization") ?? "");
        const profile = this.profile();
        const result = await this.env.SOCIAL.getByName(profile.id).dispatch(profile, path.slice(7), body);
        this.stillAuthorized(session, session.epoch);
        if (typeof result.error === "string") return accountJson({ error: result.error },
          typeof result.status === "number" ? result.status : 400);
        return accountJson(result);
      }
      if (path === "cancel") {
        this.ctx.storage.sql.exec("DELETE FROM challenges WHERE id=?", secretValue(body.attempt));
        return accountJson({ ok: true });
      }
      const ceremony = /^(register|login|add|recover|reauth)\/(options|finish)$/.exec(path);
      if (ceremony) {
        const kind = ceremony[1] as Ceremony;
        if (ceremony[2] === "options") return await this.options(request, kind, body);
        if (kind === "register" || kind === "add" || kind === "recover") return await this.finishRegistration(request, kind, body);
        return await this.finishAuthentication(request, kind, body);
      }
      return await this.manage(request, path, body);
    } catch (error) {
      if (error instanceof AccountError) return accountJson({ error: error.message }, error.status);
      console.error(JSON.stringify({ message: "account request failed" }));
      return accountJson({ error: "Account service is unavailable. Please try again." }, 500);
    }
  }
}
