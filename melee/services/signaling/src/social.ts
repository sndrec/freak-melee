import { DurableObject } from "cloudflare:workers";
import { AccountError, secret, secretHash, text, type JsonObject } from "./account-protocol";
import { isRoomId } from "./protocol";
import { DELIVERY_MS, RECEIPT_MS, SOCIAL_FRIENDS, SOCIAL_MAIL, SOCIAL_PAGE, messageId, pairName,
  requireOwner, socialHandle, socialOffset, type FriendUpdate, type MailDelivery, type SocialProfile, type SocialResult } from "./social-protocol";

interface MailRow extends Record<string, SqlStorageValue> {
  id: string; folder: number; kind: string; peer: string; handle: string; name: string;
  title: string; body: string; created: number; unread: number; status: string; deleted: number;
}
interface FriendRow extends Record<string, SqlStorageValue> {
  peer: string; handle: string; name: string; state: string; revision: number;
}
interface OutgoingRow extends Record<string, SqlStorageValue> { id: string; payload: string; created: number }
interface SendRow extends Record<string, SqlStorageValue> { nonce: string; id: string; fingerprint: string; created: number }

export class SocialMailbox extends DurableObject<CloudflareBindings> {
  private flushing: Promise<void> | undefined;
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    this.ctx.storage.sql.exec(`
      CREATE TABLE IF NOT EXISTS mail (
        id TEXT PRIMARY KEY, folder INTEGER NOT NULL, kind TEXT NOT NULL, peer TEXT NOT NULL,
        handle TEXT NOT NULL, name TEXT NOT NULL, title TEXT NOT NULL, body TEXT NOT NULL,
        created INTEGER NOT NULL, unread INTEGER NOT NULL, status TEXT NOT NULL, deleted INTEGER NOT NULL DEFAULT 0);
      CREATE INDEX IF NOT EXISTS mail_folder ON mail(folder,deleted,created DESC,id DESC);
      CREATE TABLE IF NOT EXISTS outgoing (id TEXT PRIMARY KEY, payload TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS send_keys (nonce TEXT PRIMARY KEY, id TEXT NOT NULL, fingerprint TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS received_keys (id TEXT PRIMARY KEY, peer TEXT NOT NULL, created INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS friends (
        peer TEXT PRIMARY KEY, handle TEXT NOT NULL, name TEXT NOT NULL, state TEXT NOT NULL, revision INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS blocked (peer TEXT PRIMARY KEY, handle TEXT NOT NULL);
      CREATE TABLE IF NOT EXISTS send_budget (day INTEGER PRIMARY KEY, count INTEGER NOT NULL);
      CREATE TABLE IF NOT EXISTS invitations (id TEXT PRIMARY KEY, room TEXT NOT NULL, expires INTEGER NOT NULL);
    `);
  }
  allows(peer: string): boolean {
    return !this.ctx.storage.sql.exec("SELECT peer FROM blocked WHERE peer=?", peer).toArray().length;
  }
  private count(folder: number): number {
    return this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM mail WHERE folder=? AND deleted=0", folder).one().count;
  }
  private summary(message: MailRow) {
    const invite = this.ctx.storage.sql.exec<{ room: string; expires: number }>("SELECT room,expires FROM invitations WHERE id=?", message.id).toArray()[0];
    return { id: message.id, folder: message.folder, kind: message.kind, peer: message.peer, handle: message.handle,
      name: message.name, title: message.title, created: message.created, unread: message.unread, status: message.status,
      room: invite?.room ?? "", expires: invite?.expires ?? 0 };
  }
  private insertMail(id: string, folder: number, kind: string, peer: SocialProfile, title: string, body: string,
    created: number, unread: boolean, status: string): void {
    this.ctx.storage.sql.exec("INSERT INTO mail VALUES (?,?,?,?,?,?,?,?,?,?,?,0)", id, folder, kind, peer.id,
      peer.handle, peer.name, title, body, created, unread ? 1 : 0, status);
  }
  syncFriend(update: FriendUpdate): boolean {
    requireOwner(update.owner, this.ctx.id.name);
    if (pairName(update.owner.id, update.peer.id) !== update.pair || !Number.isSafeInteger(update.revision) || update.revision < 1)
      throw new Error("Invalid friend update");
    return this.ctx.storage.transactionSync(() => {
      const previous = this.ctx.storage.sql.exec<FriendRow>("SELECT * FROM friends WHERE peer=?", update.peer.id).toArray()[0];
      if (previous && previous.revision > update.revision) return true;
      if ((!previous || previous.state === "none") && update.state !== "none" &&
          this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM friends WHERE state!='none'").one().count >= SOCIAL_FRIENDS)
        return false;
      const state = this.allows(update.peer.id) ? update.state : "none";
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO friends VALUES (?,?,?,?,?)", update.peer.id, update.peer.handle, update.peer.name, state, update.revision);
      if (state !== "incoming") this.ctx.storage.sql.exec(
        "UPDATE mail SET status='handled' WHERE kind='friend_request' AND peer=?", update.peer.id);
      if (state !== "incoming" || this.ctx.storage.sql.exec("SELECT id FROM received_keys WHERE id=?", update.notice).toArray().length) return true;
      if (this.count(0) >= SOCIAL_MAIL) return false;
      this.ctx.storage.sql.exec("INSERT INTO received_keys VALUES (?,?,?)", update.notice, update.peer.id, update.created);
      this.insertMail(update.notice, 0, "friend_request", update.peer, "Friend request", "Would you like to be friends?", update.created, true, "pending");
      return true;
    });
  }
  receiveMail(delivery: MailDelivery): "delivered" | "blocked" | "full" | "expired" {
    requireOwner(delivery.recipient, this.ctx.id.name);
    messageId(delivery.id);
    if (delivery.created < Date.now() - DELIVERY_MS || delivery.created > Date.now() + 60_000 ||
        (delivery.invitation && delivery.invitation.expires <= Date.now())) return "expired";
    if (!this.allows(delivery.sender.id)) return "blocked";
    const previous = this.ctx.storage.sql.exec<{ peer: string }>("SELECT peer FROM received_keys WHERE id=?", delivery.id).toArray()[0];
    if (previous) {
      if (previous.peer !== delivery.sender.id) throw new Error("Mail identity collision");
      return "delivered";
    }
    if (this.count(0) >= SOCIAL_MAIL) return "full";
    this.ctx.storage.transactionSync(() => {
      this.insertMail(delivery.id, 0, delivery.invitation ? "room_invite" : "letter", delivery.sender, delivery.title, delivery.body, delivery.created, true, "delivered");
      if (delivery.invitation) this.ctx.storage.sql.exec("INSERT INTO invitations VALUES (?,?,?)", delivery.id, delivery.invitation.room, delivery.invitation.expires);
      this.ctx.storage.sql.exec("INSERT INTO received_keys VALUES (?,?,?)", delivery.id, delivery.sender.id, delivery.created);
    });
    return "delivered";
  }
  private clean(): void {
    const cutoff = Date.now() - RECEIPT_MS;
    this.ctx.storage.sql.exec("DELETE FROM mail WHERE deleted=1 AND created<?", cutoff);
    this.ctx.storage.sql.exec("DELETE FROM invitations WHERE id NOT IN (SELECT id FROM mail)");
    this.ctx.storage.sql.exec("DELETE FROM received_keys WHERE created<? AND id NOT IN (SELECT id FROM mail)", cutoff);
    this.ctx.storage.sql.exec("DELETE FROM send_keys WHERE created<? AND id NOT IN (SELECT id FROM mail)", cutoff);
    this.ctx.storage.sql.exec("DELETE FROM send_budget WHERE day<?", Math.floor(Date.now() / 86_400_000) - 1);
  }
  private async state(body: JsonObject) {
    const folder = body.folder === 1 ? 1 : 0, offset = socialOffset(body.offset, SOCIAL_MAIL);
    const friendOffset = socialOffset(body.friends_offset, SOCIAL_FRIENDS);
    const messages = this.ctx.storage.sql.exec<MailRow>(
      "SELECT * FROM mail WHERE folder=? AND deleted=0 ORDER BY created DESC,id DESC LIMIT ? OFFSET ?", folder, SOCIAL_PAGE, offset).toArray();
    const friends = this.ctx.storage.sql.exec<FriendRow>(
      "SELECT * FROM friends WHERE state!='none' ORDER BY name COLLATE NOCASE,peer LIMIT ? OFFSET ?", SOCIAL_PAGE, friendOffset).toArray();
    const current = await Promise.all(friends.map(async friend => {
      const profile = await this.env.ACCOUNTS.getByName(friend.handle).lookupProfile();
      return { peer: friend.peer, handle: friend.handle, state: friend.state, revision: friend.revision,
        name: profile && profile.id === friend.peer ? profile.name : friend.name };
    }));
    return { folder, offset, friends_offset: friendOffset, total: this.count(folder),
      unread: this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM mail WHERE folder=0 AND deleted=0 AND unread=1").one().count,
      friends_total: this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM friends WHERE state!='none'").one().count,
      messages: messages.map(message => this.summary(message)), friends: current };
  }
  private async send(actor: SocialProfile, body: JsonObject, invite = false) {
    const handle = socialHandle(body.to), nonce = messageId(body.nonce);
    let title = invite ? "Lobby invitation" : text(body.title, 64), content = invite ? "Join my lobby." : text(body.body, 2048, true);
    const room = invite ? text(body.room, 12) : "";
    if (invite && !isRoomId(room)) throw new AccountError(400, "Enter a valid lobby code.");
    if (!title || !content) throw new AccountError(400, "Enter a subject and a message.");
    const recipient = await this.env.ACCOUNTS.getByName(handle).lookupProfile();
    if (!recipient) throw new AccountError(404, "Account not found.");
    if (recipient.id === actor.id) throw new AccountError(400, "Choose another player.");
    const fingerprint = await secretHash(JSON.stringify({ to: recipient.id, title, body: content, room }));
    const previous = this.ctx.storage.sql.exec<SendRow>("SELECT * FROM send_keys WHERE nonce=?", nonce).toArray()[0];
    if (previous) {
      if (previous.fingerprint !== fingerprint) throw new AccountError(409, "This message changed after sending. Start a new letter.");
      await this.flush();
      return { id: previous.id, status: this.ctx.storage.sql.exec<{ status: string }>("SELECT status FROM mail WHERE id=?", previous.id).toArray()[0]?.status ?? "sent" };
    }
    let invitation: { room: string; expires: number } | undefined;
    if (invite) {
      if (!await this.env.FRIENDSHIPS.getByName(pairName(actor.id, recipient.id)).accepted(actor.id, recipient.id))
        throw new AccountError(403, "Add this player as a friend before inviting them.");
      const details = await this.env.ROOMS.getByName(room).invitationFor(actor.id);
      if (!details) throw new AccountError(409, "Join a lobby with your account before sending an invitation.");
      const letters = Array.from(`Join ${details.title}`);
      while (new TextEncoder().encode(letters.join("")).length > 64) letters.pop();
      title = letters.join("");
      content = `Come play in ${details.title}.`;
      invitation = { room, expires: details.expires };
    }
    const id = this.ctx.storage.transactionSync(() => {
      const previous = this.ctx.storage.sql.exec<SendRow>("SELECT * FROM send_keys WHERE nonce=?", nonce).toArray()[0];
      if (previous) {
        if (previous.fingerprint !== fingerprint) throw new AccountError(409, "This message changed after sending. Start a new letter.");
        return previous.id;
      }
      if (this.count(1) >= SOCIAL_MAIL) throw new AccountError(409, "Your Sent folder is full. Delete some old letters first.");
      const day = Math.floor(Date.now() / 86_400_000);
      const count = this.ctx.storage.sql.exec<{ count: number }>("SELECT count FROM send_budget WHERE day=?", day).toArray()[0]?.count ?? 0;
      if (count >= 96) throw new AccountError(429, "You have reached today's letter limit. Please try again tomorrow.");
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO send_budget VALUES (?,?)", day, count + 1);
      const id = secret(), created = Date.now();
      const delivery: MailDelivery = { id, sender: actor, recipient, title, body: content, created, invitation };
      this.insertMail(id, 1, invite ? "room_invite" : "letter", recipient, title, content, created, false, "queued");
      if (invitation) this.ctx.storage.sql.exec("INSERT INTO invitations VALUES (?,?,?)", id, invitation.room, invitation.expires);
      this.ctx.storage.sql.exec("INSERT INTO send_keys VALUES (?,?,?,?)", nonce, id, fingerprint, created);
      this.ctx.storage.sql.exec("INSERT INTO outgoing VALUES (?,?,?)", id, JSON.stringify(delivery), created);
      return id;
    });
    await this.ctx.storage.setAlarm(Date.now() + 1000);
    await this.flush();
    return { id, status: this.ctx.storage.sql.exec<{ status: string }>("SELECT status FROM mail WHERE id=?", id).toArray()[0]?.status ?? "sent" };
  }
  private async drain(): Promise<void> {
    const rows = this.ctx.storage.sql.exec<OutgoingRow>("SELECT * FROM outgoing ORDER BY created,id LIMIT 8").toArray();
    await Promise.all(rows.map(async row => {
      try {
        const delivery = JSON.parse(row.payload) as MailDelivery;
        const result = await this.env.SOCIAL.getByName(delivery.recipient.id).receiveMail(delivery);
        if (result === "full" && row.created >= Date.now() - DELIVERY_MS) return;
        this.ctx.storage.transactionSync(() => {
          this.ctx.storage.sql.exec("UPDATE mail SET status=? WHERE id=?", result === "delivered" ? "sent" : "failed", row.id);
          this.ctx.storage.sql.exec("DELETE FROM outgoing WHERE id=?", row.id);
        });
      } catch { /* A durable alarm retries the same delivery ID. */ }
    }));
    if (this.ctx.storage.sql.exec("SELECT id FROM outgoing LIMIT 1").toArray().length) await this.ctx.storage.setAlarm(Date.now() + 30_000);
    else await this.ctx.storage.deleteAlarm();
  }
  private async flush(): Promise<void> {
    if (this.flushing) return this.flushing;
    this.flushing = this.drain();
    try { await this.flushing; } finally { this.flushing = undefined; }
  }
  async alarm(): Promise<void> { this.clean(); await this.flush(); }
  async dispatch(actor: SocialProfile, route: string, body: JsonObject): Promise<SocialResult> {
    try {
      requireOwner(actor, this.ctx.id.name); this.clean();
      if (route === "state") return await this.state(body);
      if (route === "send") return await this.send(actor, body);
      if (route === "invite") return await this.send(actor, body, true);
      if (route === "join-invite") {
        const id = messageId(body.id);
        const message = this.ctx.storage.sql.exec<MailRow>("SELECT * FROM mail WHERE id=? AND folder=0 AND kind='room_invite' AND deleted=0", id).toArray()[0];
        const invite = this.ctx.storage.sql.exec<{ room: string; expires: number }>("SELECT room,expires FROM invitations WHERE id=?", id).toArray()[0];
        if (!message || !invite) throw new AccountError(404, "This invitation is no longer in your inbox.");
        if (invite.expires <= Date.now()) throw new AccountError(410, "This lobby invitation has expired.");
        if (!this.allows(message.peer) || !await this.env.FRIENDSHIPS.getByName(pairName(actor.id, message.peer)).accepted(actor.id, message.peer))
          throw new AccountError(403, "This friend invitation is no longer active.");
        if (!await this.env.ROOMS.getByName(invite.room).invitationFor(message.peer)) throw new AccountError(409, "That lobby is no longer available.");
        return { room: invite.room };
      }
      if (route === "profile") {
        const profile = await this.env.ACCOUNTS.getByName(socialHandle(body.to)).lookupProfile();
        if (!profile) throw new AccountError(404, "Account not found.");
        const relationship = this.ctx.storage.sql.exec<FriendRow>("SELECT * FROM friends WHERE peer=?", profile.id).toArray()[0]?.state ?? "none";
        return { profile, blocked: !this.allows(profile.id), relationship };
      }
      if (route === "read" || route === "delete") {
        const id = messageId(body.id);
        const message = this.ctx.storage.sql.exec<MailRow>("SELECT * FROM mail WHERE id=? AND deleted=0", id).toArray()[0];
        if (!message) throw new AccountError(404, "This letter is no longer in your mailbox.");
        if (route === "delete") {
          this.ctx.storage.sql.exec("UPDATE mail SET deleted=1,body='',unread=0 WHERE id=?", id);
          return { ok: true };
        }
        this.ctx.storage.sql.exec("UPDATE mail SET unread=0 WHERE id=?", id);
        return { message: { ...this.summary(message), body: message.body, unread: 0 } };
      }
      if (route === "friend" || route === "block") {
        const peer = await this.env.ACCOUNTS.getByName(socialHandle(body.to)).lookupProfile();
        if (!peer) throw new AccountError(404, "Account not found.");
        const pair = pairName(actor.id, peer.id);
        let action = text(body.action ?? "remove", 16);
        if (route === "block") {
          if (typeof body.blocked !== "boolean") throw new AccountError(400, "Choose whether to block this player.");
          if (body.blocked) {
            if (!this.ctx.storage.sql.exec("SELECT peer FROM blocked WHERE peer=?", peer.id).toArray().length &&
                this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM blocked").one().count >= SOCIAL_FRIENDS)
              throw new AccountError(409, "Your blocked-player list is full.");
            this.ctx.storage.sql.exec("INSERT OR REPLACE INTO blocked VALUES (?,?)", peer.id, peer.handle);
            this.ctx.storage.sql.exec("UPDATE friends SET state='none' WHERE peer=?", peer.id);
          } else this.ctx.storage.sql.exec("DELETE FROM blocked WHERE peer=?", peer.id);
          action = "remove";
        } else if (!this.ctx.storage.sql.exec("SELECT peer FROM friends WHERE peer=? AND state!='none'", peer.id).toArray().length &&
            this.ctx.storage.sql.exec<{ count: number }>("SELECT COUNT(*) AS count FROM friends WHERE state!='none'").one().count >= SOCIAL_FRIENDS)
          throw new AccountError(409, "Your friend list is full. Remove an old contact first.");
        const result = await this.env.FRIENDSHIPS.getByName(pair).change(actor, peer, action);
        if (result.error) throw new AccountError(result.status ?? 400, result.error);
        return { ok: true };
      }
      throw new AccountError(404, "Unknown mailbox action.");
    } catch (error) {
      if (error instanceof AccountError) return { error: error.message, status: error.status };
      throw error;
    }
  }
}
