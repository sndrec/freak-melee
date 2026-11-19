import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject } from "cloudflare:test";
import { expect, it } from "vitest";
import { Buffer } from "node:buffer";
import { player } from "./account-client";
import { secret } from "../src/account-protocol";
import { fighterGiftKey, FIGHTER_GIFT_TTL_MS } from "../src/fighter-gifts";

it("delivers a fighter to an offline friend once and restricts downloads to its recipient", async () => {
  const sender = await player(), recipient = await player(), stranger = await player();
  const bytes = new TextEncoder().encode("FMSC".padEnd(208, "x"));
  const hash = Buffer.from(await crypto.subtle.digest("SHA-256", bytes)).toString("hex");
  const nonce = secret();
  const upload = (expected = hash, key = nonce) => exports.default.fetch(new Request(
    `https://signal.test/v1/accounts/${sender.handle}/fighter-gift/${recipient.handle}/${key}/${expected}?title=Test%20fighter`, {
      method: "POST", body: bytes, headers: {
        Authorization: `Bearer ${sender.grant.token}`, "Content-Type": "application/octet-stream",
        "Content-Length": String(bytes.length),
      },
    }), env);
  expect((await upload()).status).toBe(403);
  expect((await sender.social("friend", { to: recipient.handle, action: "request" })).status).toBe(200);
  expect((await recipient.social("friend", { to: sender.handle, action: "accept" })).status).toBe(200);
  const sent = await upload();
  expect(sent.status, sent.status === 200 ? "" : await sent.clone().text()).toBe(200);
  const { id } = await sent.json<{ id: string }>();
  const key = fighterGiftKey(recipient.grant.profile.id, id);
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
  const next = await upload(hash, nextNonce);
  expect(next.status).toBe(200);
  const { id: expiredId } = await next.json<{ id: string }>();
  await runInDurableObject(recipient.mailbox, (_instance, state) => {
    state.storage.sql.exec("UPDATE mail SET created=? WHERE id=?", Date.now() - FIGHTER_GIFT_TTL_MS - 1, expiredId);
  });
  expect((await recipient.request(`fighter-gift/${expiredId}`, undefined, recipient.grant.token)).status).toBe(404);
  expect((await recipient.state()).messages?.some(message => message.id === expiredId)).toBe(false);
});
