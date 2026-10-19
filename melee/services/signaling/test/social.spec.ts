import { env } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject, runDurableObjectAlarm } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { player } from "./account-client";
import { secret } from "../src/account-protocol";
import { pairName, type FriendUpdate, type MailDelivery, type SocialResult, type SocialProfile } from "../src/social-protocol";

function letter(to: string, title = "Hello", body = "A letter for later.") { return { to, title, body, nonce: secret() }; }
async function ok(response: Promise<Response>): Promise<SocialResult> {
  const result = await response; const json = await result.json<SocialResult>();
  expect(result.status, json.error).toBe(200); return json;
}

describe("persistent friends and letters", () => {
  it("delivers offline letters once, preserves read state and keeps each mailbox private", async () => {
    const a = await player(), b = await player();
    const message = letter(b.handle, "For tomorrow", "Hello!\nThis arrived while you were away.");
    const sent = await ok(a.social("send", message)); expect(sent.status).toBe("sent");
    expect((await ok(a.social("send", message))).id).toBe(sent.id);
    expect((await a.social("send", { ...message, body: "Changed" })).status).toBe(409);
    const inbox = await b.state();
    expect(inbox.total).toBe(1); expect(inbox.unread).toBe(1);
    expect(inbox.messages?.[0]).toMatchObject({ id: sent.id, title: "For tomorrow", peer: a.grant.profile.id });
    expect(inbox.messages?.[0]).not.toHaveProperty("body");
    expect((await a.state()).total).toBe(0); expect((await a.state(1)).total).toBe(1);
    const read = await ok(b.social("read", { id: sent.id }));
    expect(read.message?.body).toBe(message.body); expect((await b.state()).unread).toBe(0);
    expect((await a.social("read", { id: secret() })).status).toBe(404);
    await evictDurableObject(b.mailbox);
    expect((await b.state()).unread).toBe(0);
    await ok(b.social("delete", { id: sent.id }));
    await ok(a.social("send", message));
    expect((await b.state()).total).toBe(0);
    expect((await a.state(1)).total).toBe(1);
  });
  it("requires the account's live session and rejects forged senders and other inbox IDs", async () => {
    const a = await player(), b = await player(), c = await player();
    expect((await a.request("social/state", {}, b.grant.token)).status).toBe(401);
    expect((await a.request("social/state", {})).status).toBe(401);
    const sent = await ok(a.social("send", { ...letter(b.handle), sender: c.grant.profile, from: c.handle }));
    expect((await b.state()).messages?.[0].peer).toBe(a.grant.profile.id);
    expect((await c.social("read", { id: sent.id })).status).toBe(404);
    expect((await c.social("delete", { id: sent.id })).status).toBe(404);
    await a.request("logout", {}, a.grant.token);
    expect((await a.social("state")).status).toBe(401);
  });
  it("turns a friend request into one shared friendship, then removes it on both sides", async () => {
    const a = await player(), b = await player();
    await ok(a.social("friend", { to: b.handle, action: "request" }));
    expect((await a.state()).friends?.[0].state).toBe("outgoing");
    const incoming = await b.state();
    expect(incoming.friends?.[0].state).toBe("incoming");
    expect(incoming.messages?.[0]).toMatchObject({ kind: "friend_request", status: "pending" });
    await ok(b.social("friend", { to: a.handle, action: "accept" }));
    expect((await a.state()).friends?.[0].state).toBe("friend");
    expect((await b.state()).friends?.[0].state).toBe("friend");
    expect((await b.state()).messages?.[0].status).toBe("handled");
    await evictDurableObject(env.FRIENDSHIPS.getByName(pairName(a.grant.profile.id, b.grant.profile.id)));
    await ok(a.social("friend", { to: b.handle, action: "remove" }));
    expect((await a.state()).friends_total).toBe(0); expect((await b.state()).friends_total).toBe(0);
    const stale: FriendUpdate = { owner: b.grant.profile, peer: a.grant.profile, pair: pairName(a.grant.profile.id, b.grant.profile.id),
      revision: 1, state: "incoming", notice: secret(), created: Date.now() };
    await b.mailbox.syncFriend(stale);
    expect((await b.state()).friends_total).toBe(0);
  });
  it("handles simultaneous reciprocal requests and keeps acceptance tied to an actual request", async () => {
    const a = await player(), b = await player();
    expect((await b.social("friend", { to: a.handle, action: "accept" })).status).toBe(409);
    await Promise.all([ok(a.social("friend", { to: b.handle, action: "request" })), ok(b.social("friend", { to: a.handle, action: "request" }))]);
    expect((await a.state()).friends?.[0].state).toBe("friend");
    expect((await b.state()).friends?.[0].state).toBe("friend");
    expect((await a.state()).friends_total).toBe(1);
    await ok(a.social("friend", { to: b.handle, action: "remove" }));
    await ok(a.social("friend", { to: b.handle, action: "request" }));
    await ok(b.social("friend", { to: a.handle, action: "decline" }));
    expect((await a.state()).friends_total).toBe(0);
  });
  it("blocks letters and friend requests while preserving existing correspondence", async () => {
    const a = await player(), b = await player();
    await ok(a.social("send", letter(b.handle)));
    await ok(b.social("block", { to: a.handle, blocked: true }));
    expect((await ok(a.social("send", letter(b.handle, "Blocked")))).status).toBe("failed");
    expect((await a.social("friend", { to: b.handle, action: "request" })).status).toBe(403);
    expect((await b.state()).total).toBe(1);
    await ok(b.social("block", { to: a.handle, blocked: false }));
    expect((await ok(a.social("send", letter(b.handle, "Allowed again")))).status).toBe("sent");
    expect((await b.state()).total).toBe(2);
  });
  it("retries a durable queued delivery after a full inbox and survives a lost acknowledgement", async () => {
    const a = await player(), b = await player();
    await runInDurableObject(b.mailbox, (_instance, state) => {
      state.storage.sql.exec(`WITH RECURSIVE n(x) AS (SELECT 1 UNION ALL SELECT x+1 FROM n WHERE x<2048)
        INSERT INTO mail SELECT 'fixture-'||x,0,'letter','fixture','fixture','Fixture','Full','',0,0,'delivered',0 FROM n`);
    });
    const sent = await ok(a.social("send", letter(b.handle)));
    expect(sent.status).toBe("queued");
    const delivery = await runInDurableObject(a.mailbox, (_instance, state) => {
      return JSON.parse(state.storage.sql.exec<{ payload: string }>("SELECT payload FROM outgoing").one().payload) as MailDelivery;
    });
    await runInDurableObject(b.mailbox, (_instance, state) => { state.storage.sql.exec("DELETE FROM mail WHERE id LIKE 'fixture-%'"); });
    expect(await b.mailbox.receiveMail(delivery)).toBe("delivered");
    await evictDurableObject(a.mailbox);
    await runDurableObjectAlarm(a.mailbox);
    expect((await b.state()).total).toBe(1);
    expect((await a.state(1)).messages?.[0].status).toBe("sent");
    const pending = await runInDurableObject(a.mailbox, (_instance, state) => state.storage.sql.exec("SELECT * FROM outgoing").toArray().length);
    expect(pending).toBe(0);
  });
  it("bounds message sizes and pagination and rejects invalid recipients", async () => {
    const a = await player(), b = await player();
    expect((await a.social("send", letter(a.handle))).status).toBe(400);
    expect((await a.social("send", letter("unknown_account"))).status).toBe(404);
    expect((await a.social("send", letter("../escape"))).status).toBe(400);
    expect((await a.social("send", letter(b.handle, "x".repeat(65)))).status).toBe(400);
    expect((await a.social("send", letter(b.handle, "Valid", "x".repeat(2049)))).status).toBe(400);
    expect((await a.social("state", { offset: -1 })).status).toBe(400);
    expect((await a.social("state", { friends_offset: 257 })).status).toBe(400);
    expect((await a.social("read", { id: "bad" })).status).toBe(400);
  });
});
