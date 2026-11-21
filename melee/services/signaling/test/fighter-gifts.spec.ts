import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject } from "cloudflare:test";
import { expect, it } from "vitest";
import { Buffer } from "node:buffer";
import { player } from "./account-client";
import { secret } from "../src/account-protocol";
import { fighterGiftKey, FIGHTER_GIFT_TTL_MS } from "../src/fighter-gifts";
import { pairName } from "../src/social-protocol";

async function giftPair() {
  const sender = await player(), recipient = await player();
  const bytes = new TextEncoder().encode("FMSC".padEnd(208, "x"));
  const hash = Buffer.from(await crypto.subtle.digest("SHA-256", bytes)).toString("hex");
  const nonce = secret();
  const upload = (key = nonce) => exports.default.fetch(new Request(
    `https://signal.test/v1/accounts/${sender.handle}/fighter-gift/${recipient.handle}/${key}/${hash}?title=Test%20fighter`, {
      method: "POST", body: bytes, headers: {
        Authorization: `Bearer ${sender.grant.token}`, "Content-Type": "application/octet-stream",
        "Content-Length": String(bytes.length),
      },
    }), env);
  async function befriend() {
    expect((await sender.social("friend", { to: recipient.handle, action: "request" })).status).toBe(200);
    expect((await recipient.social("friend", { to: sender.handle, action: "accept" })).status).toBe(200);
  }
  return { sender, recipient, upload, befriend, bytes, hash };
}

it("delivers concurrent retries once and restricts the fighter download to its recipient", async () => {
  const { sender, recipient, upload, befriend, bytes, hash } = await giftPair();
  const stranger = await player();
  expect((await upload()).status).toBe(403);
  await befriend();
  const [sent, retry] = await Promise.all([upload(), upload()]);
  expect(sent.status, sent.status === 200 ? "" : await sent.clone().text()).toBe(200);
  const { id } = await sent.json<{ id: string }>();
  expect(await retry.json()).toEqual({ id });
  const object = await recipient.mailbox.fighterGift(recipient.grant.profile, id);
  expect(object?.hash).toBe(hash);
  const key = object!.key;
  const prefix = fighterGiftKey(recipient.grant.profile.id, id, "");
  expect((await env.FIGHTER_GIFTS.list({ prefix })).objects.map(o => o.key)).toEqual([key]);
  const commit = (attempt: string) => recipient.mailbox.receiveFighter(
    recipient.grant.profile, sender.grant.profile, id, "Test fighter", hash, attempt);
  expect(await commit(key.slice(prefix.length + 1))).toBe("stored");
  expect(await commit(crypto.randomUUID())).toBe("delivered");
  expect((await env.FIGHTER_GIFTS.head(key))?.size).toBe(bytes.length);
  expect(await (await upload()).json()).toEqual({ id });
  await evictDurableObject(recipient.mailbox);
  const received = (await recipient.state()).messages?.filter(message => message.kind === "fighter_gift");
  expect(received).toEqual([expect.objectContaining({ id, title: "Test fighter", unread: 1 })]);
  expect((await stranger.request(`fighter-gift/${id}`, undefined, stranger.grant.token)).status).toBe(404);
  const file = await recipient.request(`fighter-gift/${id}`, undefined, recipient.grant.token);
  expect(file.status).toBe(200);
  expect(new Uint8Array(await file.arrayBuffer())).toEqual(bytes);
  expect((await recipient.social("delete", { id })).status).toBe(200);
  expect(await env.FIGHTER_GIFTS.head(key)).toBeNull();
  expect((await recipient.social("delete", { id })).status).toBe(200);
  expect((await recipient.request(`fighter-gift/${id}`, undefined, recipient.grant.token)).status).toBe(404);
  expect(await (await upload()).json()).toEqual({ id });
  expect(await env.FIGHTER_GIFTS.head(key)).toBeNull();
  expect((await recipient.state()).messages?.some(message => message.id === id)).toBe(false);

  const nextNonce = secret();
  const next = await upload(nextNonce);
  expect(next.status).toBe(200);
  const { id: expiredId } = await next.json<{ id: string }>();
  await runInDurableObject(recipient.mailbox, (_instance, state) => {
    state.storage.sql.exec("UPDATE mail SET created=? WHERE id=?", Date.now() - FIGHTER_GIFT_TTL_MS - 1, expiredId);
  });
  expect((await recipient.request(`fighter-gift/${expiredId}`, undefined, recipient.grant.token)).status).toBe(404);
  expect((await recipient.state()).messages?.some(message => message.id === expiredId)).toBe(false);
});

it("charges upload attempts against a rolling budget and acknowledges completed retries for free", async () => {
  const { sender, recipient, upload, befriend } = await giftPair();
  await befriend();
  await runInDurableObject(sender.mailbox, (_mailbox, state) => {
    const now = Date.now();
    state.storage.sql.exec("INSERT INTO fighter_upload_attempts VALUES(?)", now - 86_400_001);
    for (let i = 0; i < 35; ++i)
      state.storage.sql.exec("INSERT INTO fighter_upload_attempts VALUES(?)", now);
  });
  const result = await upload();
  expect(result.status).toBe(200);
  const { id } = await result.json<{ id: string }>();
  expect(await (await upload()).json()).toEqual({ id });
  expect((await upload(secret())).status).toBe(429);
  expect((await recipient.state()).messages?.filter(m => m.kind === "fighter_gift")).toHaveLength(1);
  expect(await runInDurableObject(sender.mailbox, (_mailbox, state) =>
    state.storage.sql.exec("SELECT COUNT(*) n FROM fighter_upload_attempts").one().n)).toBe(36);
});

it("rechecks the authoritative friendship when uploads start and finish", async () => {
  const { sender, recipient, upload, befriend, hash } = await giftPair();
  await befriend();
  const from = sender.grant.profile, to = recipient.grant.profile;
  // A removal has committed but the two mailbox friend lists have not received it yet.
  await runInDurableObject(env.FRIENDSHIPS.getByName(pairName(from.id, to.id)), (_pair, state) => {
    state.storage.sql.exec("UPDATE friendship SET state=0 WHERE id=1");
  });
  const id = secret();
  expect(await recipient.mailbox.fighterDeliveryStatus(to, from.id, id, hash)).toBe("ready");
  expect((await upload()).status).toBe(403);
  expect(await recipient.mailbox.receiveFighter(to, from, id, "Test fighter", hash, crypto.randomUUID())).toBeNull();
  expect((await recipient.state()).messages?.some(m => m.kind === "fighter_gift")).toBe(false);
});

it("downloads and deletes a gift already in the inbox before upload attempts had separate object keys", async () => {
  const { recipient, upload, befriend, bytes, hash } = await giftPair();
  await befriend();
  const sent = await upload();
  const { id } = await sent.json<{ id: string }>();
  const to = recipient.grant.profile;
  const oldKey = fighterGiftKey(to.id, id, "");
  const newGift = await recipient.mailbox.fighterGift(to, id);
  await env.FIGHTER_GIFTS.put(oldKey, bytes, { sha256: hash, customMetadata: { hash } });
  await env.FIGHTER_GIFTS.delete(newGift!.key);
  await runInDurableObject(recipient.mailbox, (_mailbox, state) => {
    state.storage.sql.exec("DELETE FROM fighter_files WHERE id=?", id);
  });
  const downloaded = await recipient.request(`fighter-gift/${id}`, undefined, recipient.grant.token);
  expect(downloaded.status).toBe(200);
  expect(new Uint8Array(await downloaded.arrayBuffer())).toEqual(bytes);
  expect((await recipient.social("delete", { id })).status).toBe(200);
  expect(await env.FIGHTER_GIFTS.head(oldKey)).toBeNull();
});
