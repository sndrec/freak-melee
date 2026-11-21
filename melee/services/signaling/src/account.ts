import { DurableObject } from "cloudflare:workers";
import { verifyAccountKey } from "./account-key";
import { AccountError, accountJson, accountHandle, secret, secretHash, secretValue, equalHash,
  text, readAccountBody, CHALLENGE_MS, SESSION_MS, RECENT_AUTH_MS,
  MAX_ACCOUNT_KEYS, MAX_SESSIONS, type Ceremony, type JsonObject } from "./account-protocol";
import type { SocialProfile } from "./social-protocol";
import { fighterGiftRequest } from "./fighter-gifts";
import type { ConnectionAccount } from "./connection-account";
import { isRoomId } from "./protocol";

interface AccountRow extends Record<string, SqlStorageValue> {
  user_id: string; display_name: string; bio: string; recovery_hash: string; epoch: number;
}
interface KeyRow extends Record<string, SqlStorageValue> {
  id: string; label: string; created: number;
}
interface SessionRow extends Record<string, SqlStorageValue> {
  hash: string; expires: number; verified: number; label: string; epoch: number; key_id: string;
}
interface ChallengeRow extends Record<string, SqlStorageValue> {
  id: string; kind: Ceremony; challenge: string; user_id: string; display_name: string;
  expires: number; epoch: number; session_hash: string; recovery_hash: string; key_id: string; key_label: string;
}

export class Account extends DurableObject<CloudflareBindings> {
  private initialized = false;
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    if (ctx.storage.sql.exec("SELECT 1 FROM sqlite_schema WHERE type='table' AND name='account'").toArray().length)
      this.initialize();
  }
  private initialize(): void {
    this.ctx.storage.sql.exec(`
      CREATE TABLE IF NOT EXISTS account (
        id INTEGER PRIMARY KEY CHECK(id=1), user_id TEXT NOT NULL, display_name TEXT NOT NULL,
        bio TEXT NOT NULL, recovery_hash TEXT NOT NULL, epoch INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS account_keys (
        id TEXT PRIMARY KEY, label TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS sessions (
        hash TEXT PRIMARY KEY, expires INTEGER NOT NULL, verified INTEGER NOT NULL,
        label TEXT NOT NULL, epoch INTEGER NOT NULL, key_id TEXT NOT NULL DEFAULT '');
      CREATE TABLE IF NOT EXISTS key_challenges (
        id TEXT PRIMARY KEY, kind TEXT NOT NULL, challenge TEXT NOT NULL, user_id TEXT NOT NULL,
        display_name TEXT NOT NULL, expires INTEGER NOT NULL, epoch INTEGER NOT NULL,
        session_hash TEXT NOT NULL, recovery_hash TEXT NOT NULL, key_id TEXT NOT NULL, key_label TEXT NOT NULL);
    `);
    const columns = this.ctx.storage.sql.exec<{ name: string }>("PRAGMA table_info(sessions)").toArray();
    if (!columns.some(column => column.name === "key_id"))
      this.ctx.storage.sql.exec("ALTER TABLE sessions ADD COLUMN key_id TEXT NOT NULL DEFAULT ''");
    // Retain account IDs, mail and friendships; retire the abandoned WebAuthn credentials.
    this.ctx.storage.sql.exec("DROP TABLE IF EXISTS passkeys; DROP TABLE IF EXISTS challenges;");
    this.initialized = true;
  }

  private account(): AccountRow | null {
    if (!this.initialized) return null;
    return this.ctx.storage.sql.exec<AccountRow>("SELECT * FROM account WHERE id=1").toArray()[0] ?? null;
  }
  private keys(): KeyRow[] { return this.ctx.storage.sql.exec<KeyRow>("SELECT * FROM account_keys ORDER BY created, id").toArray(); }
  private key(id: string): KeyRow | null {
    return this.ctx.storage.sql.exec<KeyRow>("SELECT * FROM account_keys WHERE id=?", id).toArray()[0] ?? null;
  }
  private profile() {
    const account = this.account();
    const handle = this.ctx.id.name;
    if (!account || !handle) throw new AccountError(404, "Account not found.");
    return { id: account.user_id, handle, name: account.display_name, bio: account.bio };
  }
  lookupProfile(): SocialProfile | null { return this.account() ? this.profile() : null; }
  openConnection(authorization: string): ConnectionAccount | null {
    try {
      const session = this.authorize(authorization);
      return { profile: this.profile(), session: session.hash, epoch: session.epoch, expires: session.expires };
    } catch { return null; }
  }
  checkConnection(connection: ConnectionAccount): SocialProfile | null {
    const account = this.account();
    if (!account) return null;
    const session = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", connection.session).toArray()[0];
    return session && session.expires > Date.now() && session.epoch === account.epoch &&
      connection.epoch === account.epoch && connection.profile.id === account.user_id && this.key(session.key_id) ? this.profile() : null;
  }
  private clean(): void {
    if (!this.initialized) return;
    const now = Date.now();
    this.ctx.storage.sql.exec("DELETE FROM key_challenges WHERE expires<=?", now);
    this.ctx.storage.sql.exec("DELETE FROM sessions WHERE expires<=? OR key_id NOT IN (SELECT id FROM account_keys)", now);
  }
  private authorize(authorization: string, recent = false): SessionRow {
    const header = authorization;
    if (!this.initialized || !/^Bearer [A-Za-z0-9_-]{43}$/.test(header)) throw new AccountError(401, "Please sign in.");
    const hash = secretHash(header.slice(7));
    const session = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", hash).toArray()[0];
    const account = this.account();
    if (!session || !account || session.expires <= Date.now() || session.epoch !== account.epoch || !this.key(session.key_id))
      throw new AccountError(401, "Please sign in again.");
    if (recent && session.verified < Date.now() - RECENT_AUTH_MS)
      throw new AccountError(403, "Verify your account key before changing account security.");
    return session;
  }
  private stillAuthorized(session: SessionRow, epoch: number): void {
    const active = this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions WHERE hash=?", session.hash).toArray()[0];
    if (!active || active.expires <= Date.now() || active.epoch !== epoch || this.account()?.epoch !== epoch || !this.key(active.key_id))
      throw new AccountError(401, "Your sign-in changed. Please try again.");
  }
  private takeChallenge(id: unknown, kind: Ceremony): ChallengeRow {
    if (!this.initialized) throw new AccountError(400, "That sign-in attempt expired or was already used. Try again.");
    const rows = this.ctx.storage.sql.exec<ChallengeRow>(
      "DELETE FROM key_challenges WHERE id=? AND kind=? AND expires>? RETURNING *", secretValue(id), kind, Date.now()).toArray();
    if (!rows.length) throw new AccountError(400, "That sign-in attempt expired or was already used. Try again.");
    return rows[0];
  }

  private async options(request: Request, kind: Ceremony, body: JsonObject): Promise<Response> {
    const account = this.account();
    let session: SessionRow | undefined, recoveryHash = "";
    if (kind === "register") {
      if (account) throw new AccountError(409, "That account name is already taken.");
    } else if (!account) throw new AccountError(404, "Account not found.");
    if (kind === "add" || kind === "reauth") session = this.authorize(request.headers.get("Authorization") ?? "", kind === "add");
    if (kind === "recover") {
      recoveryHash = secretHash(secretValue(body.recovery_code));
      if (!account || !equalHash(recoveryHash, account.recovery_hash)) throw new AccountError(401, "Recovery code was not accepted.");
    }
    const displayName = kind === "register" ? text(body.name ?? this.ctx.id.name, 32) : account!.display_name;
    if (!displayName) throw new AccountError(400, "Enter a display name.");
    const publicKey = secretValue(body.public_key);
    const keyLabel = text(body.key_name ?? "Account key", 64) || "Account key";
    if (!this.initialized) this.initialize();
    // Public sign-in attempts cannot use the slots for authenticated account changes or recovery.
    let kinds = ["register", "login"], limit = 16;
    if (kind === "recover") {
      kinds = ["recover"];
      limit = 2;
    } else if (session) {
      kinds = ["add", "reauth"];
      limit = 4;
    }
    const pending = this.ctx.storage.sql.exec<{ count: number }>(
      "SELECT COUNT(*) AS count FROM key_challenges WHERE kind IN (?,?)", kinds[0], kinds[1] ?? kinds[0]).one().count;
    if (pending >= limit) throw new AccountError(429, "Too many unfinished sign-in attempts. Please wait a moment.");
    const userId = account?.user_id ?? secret();
    if (kind === "add" && this.key(publicKey)) throw new AccountError(409, "That account key is already registered.");
    if (kind === "add" && this.keys().length >= MAX_ACCOUNT_KEYS) throw new AccountError(409, "Remove an account key before adding another.");
    if ((kind === "login" || kind === "reauth") && !this.key(publicKey))
      throw new AccountError(401, "This account key was not accepted.");
    const id = secret(), challenge = secret(), expires = Date.now() + CHALLENGE_MS;
    this.ctx.storage.sql.exec("INSERT INTO key_challenges VALUES (?,?,?,?,?,?,?,?,?,?,?)", id, kind,
      challenge, userId, displayName, expires, account?.epoch ?? 0,
      session?.hash ?? "", recoveryHash, publicKey, keyLabel);
    if (kind === "register") await this.ctx.storage.setAlarm(expires);
    return accountJson({ attempt: id, challenge, user_id: userId, public_key: publicKey,
      purpose: kind, origin: this.env.AUTH_ORIGIN });
  }

  async alarm(): Promise<void> {
    // Only an unfinished registration schedules this alarm. Keep established
    // accounts and any registration that still has a valid challenge.
    await this.ctx.blockConcurrencyWhile(async () => {
      if (!this.initialized || this.account()) return;
      const expires = this.ctx.storage.sql.exec<{ expires: number | null }>(
        "SELECT max(expires) AS expires FROM key_challenges WHERE kind='register'").one().expires;
      if (expires && expires > Date.now()) {
        await this.ctx.storage.setAlarm(expires);
        return;
      }
      await this.ctx.storage.deleteAll();
      this.initialized = false;
    });
  }

  private sessionValues(label: unknown) {
    const token = secret();
    return { token, hash: secretHash(token),
      expires: Date.now() + SESSION_MS, label: text(label ?? "PC", 64) };
  }
  private insertSession(values: { hash: string; expires: number; label: string }, epoch: number, key: string): void {
    this.ctx.storage.sql.exec(`DELETE FROM sessions WHERE hash IN
      (SELECT hash FROM sessions ORDER BY verified DESC, hash LIMIT -1 OFFSET ?)`, MAX_SESSIONS - 1);
    this.ctx.storage.sql.exec("INSERT INTO sessions VALUES (?,?,?,?,?,?)", values.hash, values.expires, Date.now(), values.label, epoch, key);
  }

  private finishRegistration(request: Request, kind: "register" | "add" | "recover", body: JsonObject): Response {
    const session = kind === "add" ? this.authorize(request.headers.get("Authorization") ?? "", true) : undefined;
    const challenge = this.takeChallenge(body.attempt, kind);
    if (session && session.hash !== challenge.session_hash) throw new AccountError(401, "Use the sign-in that started this request.");
    this.verifyProof(challenge, body);
    return this.ctx.storage.transactionSync(() => {
      const account = this.account();
      const recoveryCode = kind === "add" ? "" : secret();
      if (kind === "register") {
        if (account) throw new AccountError(409, "That account name is already taken.");
        this.ctx.storage.sql.exec("INSERT INTO account VALUES (1,?,?,?,?,1)", challenge.user_id,
          challenge.display_name, "", secretHash(recoveryCode));
      } else {
        if (!account || account.epoch !== challenge.epoch) throw new AccountError(409, "The account changed. Please try again.");
        if (kind === "recover") {
          if (!equalHash(account.recovery_hash, challenge.recovery_hash)) throw new AccountError(401, "Recovery code was already used.");
          this.ctx.storage.sql.exec("DELETE FROM account_keys; DELETE FROM sessions; DELETE FROM key_challenges;");
          this.ctx.storage.sql.exec("UPDATE account SET epoch=epoch+1, recovery_hash=? WHERE id=1", secretHash(recoveryCode));
        } else {
          if (this.keys().length >= MAX_ACCOUNT_KEYS) throw new AccountError(409, "The account already has eight account keys.");
        }
      }
      if (this.key(challenge.key_id)) throw new AccountError(409, "That account key is already registered.");
      this.ctx.storage.sql.exec("INSERT INTO account_keys VALUES (?,?,?)", challenge.key_id, challenge.key_label, Date.now());
      if (kind === "add") return accountJson({ profile: this.profile() });
      const values = this.sessionValues(body.device);
      this.insertSession(values, this.account()!.epoch, challenge.key_id);
      return accountJson({ profile: this.profile(), token: values.token, expires: values.expires, recovery_code: recoveryCode });
    });
  }

  private finishAuthentication(request: Request, kind: "login" | "reauth", body: JsonObject): Response {
    const session = kind === "reauth" ? this.authorize(request.headers.get("Authorization") ?? "") : undefined;
    const challenge = this.takeChallenge(body.attempt, kind);
    if (session && session.hash !== challenge.session_hash) throw new AccountError(401, "Use the sign-in that started this request.");
    this.verifyProof(challenge, body);
    return this.ctx.storage.transactionSync(() => {
      if (this.account()?.epoch !== challenge.epoch) throw new AccountError(401, "The account changed. Please sign in again.");
      if (!this.key(challenge.key_id)) throw new AccountError(401, "This account key was removed. Import a current key to sign in.");
      if (session) {
        this.ctx.storage.sql.exec("UPDATE sessions SET verified=? WHERE hash=?", Date.now(), session.hash);
        return accountJson({ profile: this.profile() });
      }
      const values = this.sessionValues(body.device);
      this.insertSession(values, challenge.epoch, challenge.key_id);
      return accountJson({ profile: this.profile(), token: values.token, expires: values.expires });
    });
  }

  private verifyProof(challenge: ChallengeRow, body: JsonObject): void {
    if (!verifyAccountKey({ origin: this.env.AUTH_ORIGIN, handle: this.ctx.id.name!, userId: challenge.user_id,
      purpose: challenge.kind, publicKey: challenge.key_id, attempt: String(body.attempt), challenge: challenge.challenge }, body.signature))
      throw new AccountError(401, "Account-key proof was not accepted. Please try again.");
  }

  private manage(request: Request, route: string, body: JsonObject): Response {
    const sensitive = route === "recovery/rotate" || route === "keys/remove" ||
      route === "sessions/revoke" || route === "sessions/revoke-all";
    const session = this.authorize(request.headers.get("Authorization") ?? "", sensitive);
    if (route === "me") return accountJson({ profile: this.profile(), expires: session.expires,
      keys: this.keys().map(key => ({ id: key.id, name: key.label, created: key.created })),
      sessions: this.ctx.storage.sql.exec<SessionRow>("SELECT * FROM sessions ORDER BY verified DESC").toArray()
        .map(active => ({ id: active.hash, name: active.label, expires: active.expires, current: active.hash === session.hash })) });
    if (route === "profile") {
      const name = text(body.name, 32), bio = text(body.bio, 256, true);
      if (!name) throw new AccountError(400, "Enter a display name.");
      this.ctx.storage.sql.exec("UPDATE account SET display_name=?, bio=? WHERE id=1", name, bio);
    } else if (route === "logout") this.ctx.storage.sql.exec("DELETE FROM sessions WHERE hash=?", session.hash);
    else if (route === "sessions/revoke") this.ctx.storage.sql.exec("DELETE FROM sessions WHERE hash=?", secretValue(body.id));
    else if (route === "sessions/revoke-all") this.ctx.storage.sql.exec(
      "UPDATE account SET epoch=epoch+1 WHERE id=1; DELETE FROM sessions; DELETE FROM key_challenges;");
    else if (route === "keys/remove") {
      if (this.keys().length <= 1) throw new AccountError(409, "Keep at least one account key. Add a replacement first.");
      const id = secretValue(body.id);
      this.ctx.storage.transactionSync(() => {
        this.ctx.storage.sql.exec(`DELETE FROM key_challenges WHERE key_id=? OR session_hash IN
          (SELECT hash FROM sessions WHERE key_id=?)`, id, id);
        this.ctx.storage.sql.exec("DELETE FROM sessions WHERE key_id=?", id);
        this.ctx.storage.sql.exec("DELETE FROM account_keys WHERE id=?", id);
      });
    } else if (route === "recovery/rotate") {
      const code = secret(), hash = secretHash(code);
      this.ctx.storage.sql.exec("UPDATE account SET recovery_hash=? WHERE id=1", hash);
      this.ctx.storage.sql.exec("DELETE FROM key_challenges WHERE kind='recover'");
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
      if (origin.origin !== this.env.AUTH_ORIGIN ||
          (origin.protocol !== "https:" && !local))
        throw new AccountError(503, "Account sign-in is not configured for this service.");
      this.clean();
      if (path.startsWith("fighter-gift/")) {
        this.authorize(request.headers.get("Authorization") ?? "");
        return await fighterGiftRequest(request, this.env, this.profile(), path);
      }
      if (request.method === "GET") {
        if (path === "profile") return accountJson({ profile: this.profile() });
        if (path === "me") return this.manage(request, path, {});
        throw new AccountError(404, "Not found.");
      }
      if (request.method !== "POST") throw new AccountError(405, "Method not allowed.");
      const body = await readAccountBody(request);
      if (path === "lobby/attach") {
        const session = this.authorize(request.headers.get("Authorization") ?? "");
        if (typeof body.room !== "string" || !isRoomId(body.room) ||
            typeof body.connection !== "string" ||
            !/^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/.test(body.connection))
          throw new AccountError(400, "The lobby connection could not be identified.");
        const account = { profile: this.profile(), session: session.hash, epoch: session.epoch, expires: session.expires };
        const attached = await this.env.ROOMS.getByName(body.room).attachAccount(body.connection, account);
        this.stillAuthorized(session, session.epoch);
        if (!attached) throw new AccountError(409, "That lobby connection is no longer available for this account.");
        return accountJson({ ok: true });
      }
      if (path.startsWith("social/")) {
        const session = this.authorize(request.headers.get("Authorization") ?? "");
        const profile = this.profile();
        const result = await this.env.SOCIAL.getByName(profile.id).dispatch(profile, path.slice(7), body);
        this.stillAuthorized(session, session.epoch);
        if (typeof result.error === "string") return accountJson({ error: result.error },
          typeof result.status === "number" ? result.status : 400);
        return accountJson(result);
      }
      if (path === "cancel") {
        const attempt = secretValue(body.attempt);
        if (this.initialized) this.ctx.storage.sql.exec("DELETE FROM key_challenges WHERE id=?", attempt);
        return accountJson({ ok: true });
      }
      const ceremony = /^(register|login|add|recover|reauth)\/(options|finish)$/.exec(path);
      if (ceremony) {
        const kind = ceremony[1] as Ceremony;
        if (ceremony[2] === "options") return await this.options(request, kind, body);
        if (kind === "register" || kind === "add" || kind === "recover") return this.finishRegistration(request, kind, body);
        return this.finishAuthentication(request, kind, body);
      }
      return this.manage(request, path, body);
    } catch (error) {
      if (error instanceof AccountError) return accountJson({ error: error.message }, error.status);
      console.error(JSON.stringify({ message: "account request failed" }));
      return accountJson({ error: "Account service is unavailable. Please try again." }, 500);
    }
  }
}
