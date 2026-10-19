import { DurableObject } from "cloudflare:workers";
import { AccountError, secret } from "./account-protocol";
import { pairName, type SocialProfile, type FriendUpdate, type FriendState } from "./social-protocol";

interface PairRow extends Record<string, SqlStorageValue> {
  first_id: string; second_id: string; state: number; revision: number;
}
interface UpdateRow extends Record<string, SqlStorageValue> { recipient: string; revision: number; payload: string }

// The pair is the authority for friendship. Mailboxes hold ordered copies for
// their lists; a late retry cannot resurrect a removed friendship.
export class SocialFriendship extends DurableObject<CloudflareBindings> {
  private flushing: Promise<void> | undefined;
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    this.ctx.storage.sql.exec(`
      CREATE TABLE IF NOT EXISTS friendship (
        id INTEGER PRIMARY KEY CHECK(id=1), first_id TEXT NOT NULL, second_id TEXT NOT NULL,
        state INTEGER NOT NULL, revision INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS friend_updates (
        recipient TEXT PRIMARY KEY, revision INTEGER NOT NULL, payload TEXT NOT NULL);
    `);
  }
  async change(actor: SocialProfile, peer: SocialProfile, action: string): Promise<{ error?: string; status?: number }> {
    try {
      const pair = pairName(actor.id, peer.id);
      if (pair !== this.ctx.id.name) throw new AccountError(403, "Friendship identity does not match.");
      if (!["request", "accept", "decline", "remove"].includes(action)) throw new AccountError(400, "Unknown friend action.");
      const first = actor.id < peer.id ? actor : peer, second = actor.id < peer.id ? peer : actor;
      const requester = actor.id === first.id ? 1 : 2, incoming = requester === 1 ? 2 : 1;
      if (action === "request" || action === "accept") {
        const allowed = await Promise.all([
          this.env.SOCIAL.getByName(actor.id).allows(peer.id), this.env.SOCIAL.getByName(peer.id).allows(actor.id),
        ]);
        if (allowed.some(value => !value)) throw new AccountError(403, "This player is not accepting friend requests.");
      }
      const existing = this.ctx.storage.sql.exec<PairRow>("SELECT * FROM friendship WHERE id=1").toArray()[0];
      const previous = existing?.state ?? 0;
      let state = previous;
      if (action === "request") state = previous === incoming ? 3 : previous === 0 ? requester : previous;
      else if (action === "accept") {
        if (previous !== incoming && previous !== 3) throw new AccountError(409, "That friend request is no longer pending.");
        state = 3;
      } else if (action === "decline") {
        if (previous !== incoming && previous !== 0) throw new AccountError(409, "That friend request is no longer pending.");
        state = 0;
      } else state = 0;
      if (!existing || state !== previous) {
        const revision = (existing?.revision ?? 0) + 1, created = Date.now();
        const status = (who: number): FriendState => state === 3 ? "friend" : state === 0 ? "none" : state === who ? "outgoing" : "incoming";
        this.ctx.storage.transactionSync(() => {
          this.ctx.storage.sql.exec("INSERT OR REPLACE INTO friendship VALUES (1,?,?,?,?)", first.id, second.id, state, revision);
          for (const [owner, other, index] of [[first, second, 1], [second, first, 2]] as const) {
            const update: FriendUpdate = { owner, peer: other, pair, revision, state: status(index), notice: secret(), created };
            this.ctx.storage.sql.exec("INSERT OR REPLACE INTO friend_updates VALUES (?,?,?)", owner.id, revision, JSON.stringify(update));
          }
        });
      }
      if (this.pending()) await this.ctx.storage.setAlarm(Date.now() + 1000);
      await this.flush();
      return {};
    } catch (error) {
      if (error instanceof AccountError) return { error: error.message, status: error.status };
      throw error;
    }
  }
  private pending(): boolean { return this.ctx.storage.sql.exec("SELECT recipient FROM friend_updates LIMIT 1").toArray().length > 0; }
  accepted(first: string, second: string): boolean {
    return pairName(first, second) === this.ctx.id.name &&
      this.ctx.storage.sql.exec<PairRow>("SELECT * FROM friendship WHERE id=1").toArray()[0]?.state === 3;
  }
  private async drain(): Promise<void> {
    // Revisions committed during an await get another pass, with bounded work.
    for (let pass = 0; pass < 4; ++pass) {
      const updates = this.ctx.storage.sql.exec<UpdateRow>("SELECT * FROM friend_updates").toArray();
      if (!updates.length) break;
      const delivered = await Promise.all(updates.map(async update => {
        try {
          if (!await this.env.SOCIAL.getByName(update.recipient).syncFriend(JSON.parse(update.payload) as FriendUpdate)) return false;
          this.ctx.storage.sql.exec("DELETE FROM friend_updates WHERE recipient=? AND revision=?", update.recipient, update.revision);
          return true;
        } catch { return false; }
      }));
      if (delivered.some(value => !value)) break;
    }
    if (this.pending()) await this.ctx.storage.setAlarm(Date.now() + 30_000);
    else await this.ctx.storage.deleteAlarm();
  }
  private async flush(): Promise<void> {
    if (this.flushing) return this.flushing;
    this.flushing = this.drain();
    try { await this.flushing; } finally { this.flushing = undefined; }
  }
  async alarm(): Promise<void> { await this.flush(); }
}
