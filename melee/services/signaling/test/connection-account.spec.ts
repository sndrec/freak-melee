import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { player } from "./account-client";
import { QuickClient } from "./quick-client";
import { encodeChatIdentity, encodeChatPost } from "../src/chat";
import { encodeRequestPresence, encodeSetMatch, encodeUpdate, ServerMessageType } from "../src/wire";
import { secret, secretHash } from "../src/account-protocol";
import type { SocialResult } from "../src/social-protocol";

type Account = Awaited<ReturnType<typeof player>>;
const decoder = new TextDecoder();
const peerId = () => crypto.randomUUID().replaceAll("-", "");
const roomId = () => peerId().slice(0, 12).toUpperCase();
function request(path: string, account?: Account, headers: Record<string, string> = {}) {
  return exports.default.fetch(new Request(`https://signal.test${path}`, { headers: {
    Upgrade: "websocket", ...(account ? { Authorization: `Bearer ${account.grant.token}`, "X-Freak-Account": account.handle } : {}), ...headers,
  } }), env);
}
async function room(account: Account | undefined, code: string, peer = peerId()) {
  const response = await request(`/v1/rooms/${code}/connect?peer=${peer}&name=Spoofed&build=identity-test&players=1`, account);
  expect(response.status).toBe(101);
  const client = new QuickClient(response.webSocket!);
  const welcome = await client.next(ServerMessageType.Welcome);
  client.peerId = welcome.peerId;
  const presence = (await client.next(ServerMessageType.Presence)).snapshot;
  return { client, presence, peer, connection: welcome.connectionId };
}
class ChatPackets {
  private packets: Uint8Array[] = [];
  private changed?: () => void;
  private received = Promise.resolve();
  constructor(readonly socket: WebSocket) {
    socket.addEventListener("message", event => {
      this.received = this.received.then(async () => {
        const data = event.data as unknown;
        const bytes = ArrayBuffer.isView(data) ? new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice()
          : new Uint8Array(typeof data === "object" && data !== null && "arrayBuffer" in data
            ? await (data as Blob).arrayBuffer() : data as ArrayBuffer);
        this.packets.push(bytes); this.changed?.();
      });
    });
    socket.accept();
  }
  async next(type: number): Promise<Uint8Array> {
    for (;;) {
      const index = this.packets.findIndex(bytes => bytes[0] === type);
      if (index >= 0) return this.packets.splice(index, 1)[0];
      await new Promise<void>((resolve, reject) => {
        const timer = setTimeout(() => { this.changed = undefined; reject(new Error(`Missing packet ${type}`)); }, 3000);
        this.changed = () => { clearTimeout(timer); this.changed = undefined; resolve(); };
      });
    }
  }
}
function identity(bytes: Uint8Array, at: number) {
  expect(bytes.length).toBe(at + 2 + bytes[at] + bytes[at + 1]);
  return { id: decoder.decode(bytes.subarray(at + 2, at + 2 + bytes[at])),
    handle: decoder.decode(bytes.subarray(at + 2 + bytes[at])) };
}
async function chat(account?: Account, peer = peerId(), headers: Record<string, string> = {}) {
  const response = await request(`/chat/connect?peer=${peer}`, account, headers);
  expect(response.status).toBe(101);
  const client = new ChatPackets(response.webSocket!); await client.next(0x82);
  return { client, peer };
}
async function ok(result: Promise<Response>) {
  const response = await result, body = await response.json<SocialResult>();
  expect(response.status, body.error).toBe(200); return body;
}
async function friends(a: Account, b: Account) {
  await ok(a.social("friend", { to: b.handle, action: "request" }));
  await ok(b.social("friend", { to: a.handle, action: "accept" }));
}

describe("verified connection identities and invitations", () => {
  it("keeps an active guest connection when another client presents its public peer ID", async () => {
    const a = await player(), code = roomId(), guest = await room(undefined, code);
    try {
      const path = `/v1/rooms/${code}/connect?peer=${guest.peer}&name=Other&players=1&build=identity-test`;
      expect((await request(path)).status).toBe(409);
      expect((await request(path, a)).status).toBe(409);
      guest.client.socket.send(encodeRequestPresence(8192));
      expect((await guest.client.next(ServerMessageType.Presence)).snapshot).toEqual(guest.presence);
      expect(guest.client.socket.readyState).toBe(WebSocket.OPEN);
      await guest.client.close();
      const returning = await room(undefined, code, guest.peer);
      await returning.client.close();
    } finally { if (guest.client.socket.readyState === WebSocket.OPEN) await guest.client.close(); }
  });
  it("attaches a signed-in account to the existing guest connection without changing lobby ownership or readiness", async () => {
    const code = roomId(), guest = await room(undefined, code), observer = await room(undefined, code);
    const a = await player();
    try {
      guest.client.socket.send(encodeUpdate({ selection: new Uint8Array([1, 2, 3]), activeMask: 1 }, 8192));
      let before = guest.presence;
      do { before = (await guest.client.next(ServerMessageType.Presence)).snapshot; }
      while (!before.participants.find(p => p.peerId === guest.peer)?.selection.length);
      guest.client.socket.send(encodeSetMatch(new Uint8Array([0x46, 0x4d, 0x4c, 0x01]), 8192));
      do { before = (await guest.client.next(ServerMessageType.Presence)).snapshot; } while (!before.match?.length);
      guest.client.socket.send(encodeUpdate({ ready: true }, 8192));
      do { before = (await guest.client.next(ServerMessageType.Presence)).snapshot; }
      while (!before.participants.find(p => p.peerId === guest.peer)?.ready);
      expect(before.participants).toHaveLength(2);
      await ok(a.request("lobby/attach", { room: code, connection: guest.connection }, a.grant.token));
      const after = (await guest.client.next(ServerMessageType.Presence)).snapshot;
      expect(after.ownerPeerId).toBe(before.ownerPeerId);
      expect(after.participants).toHaveLength(2);
      expect(after.participants.find(p => p.peerId === guest.peer)).toEqual({
        ...before.participants.find(p => p.peerId === guest.peer),
        accountId: a.grant.profile.id, accountHandle: a.handle, name: a.grant.profile.name,
      });
      expect(after.matchEpoch).toBe(before.matchEpoch);
      expect(after.match).toEqual(before.match);
      expect(guest.client.socket.readyState).toBe(WebSocket.OPEN);
      await evictDurableObject(env.ROOMS.getByName(code));
      guest.client.socket.send(encodeRequestPresence(8192));
      const restored = (await guest.client.next(ServerMessageType.Presence)).snapshot;
      expect(restored.participants.find(p => p.peerId === guest.peer)?.accountId).toBe(a.grant.profile.id);
      await ok(a.request("lobby/attach", { room: code, connection: guest.connection }, a.grant.token));
    } finally {
      await guest.client.close(); await observer.client.close();
    }
  });
  it("rejects public peer IDs, missing credentials, stale connections and attempts to replace an attached account", async () => {
    const a = await player(), b = await player(), code = roomId(), guest = await room(undefined, code);
    try {
      const body = { room: code, connection: guest.connection };
      expect((await a.request("lobby/attach", body)).status).toBe(401);
      expect((await a.request("lobby/attach", { ...body, connection: guest.peer }, a.grant.token)).status).toBe(400);
      expect((await a.request("lobby/attach", { ...body, connection: crypto.randomUUID() }, a.grant.token)).status).toBe(409);
      expect((await a.request("lobby/attach", { ...body, room: "INVALID!" }, a.grant.token)).status).toBe(400);
      await ok(a.request("lobby/attach", { ...body, profile: b.grant.profile }, a.grant.token));
      expect((await b.request("lobby/attach", body, b.grant.token)).status).toBe(409);
      await guest.client.close();
      expect((await a.request("lobby/attach", body, a.grant.token)).status).toBe(409);
      await ok(a.request("logout", {}, a.grant.token));
      expect((await a.request("lobby/attach", body, a.grant.token)).status).toBe(401);
    } finally { if (guest.client.socket.readyState === WebSocket.OPEN) await guest.client.close(); }
  });
  it("strips forged internal identities and rejects invalid or mismatched credentials", async () => {
    const a = await player(), b = await player();
    for (const path of [`/chat/connect?peer=${peerId()}`, `/v1/rooms/${roomId()}/connect?peer=${peerId()}&players=1&build=test`]) {
      expect((await request(path, a, { Authorization: `Bearer ${b.grant.token}` })).status).toBe(401);
      expect((await request(path, undefined, { "X-Freak-Account": a.handle })).status).toBe(401);
      expect((await request(path, undefined, { Authorization: `Bearer ${a.grant.token}` })).status).toBe(401);
    }
    const guest = await chat(undefined, peerId(), { "X-Freak-Verified-Account": JSON.stringify({ profile: a.grant.profile }) });
    try {
      const record = await guest.client.next(0x86);
      expect(identity(record, 34 + record[33])).toEqual({ id: "", handle: "" });
    } finally { guest.client.socket.close(); }
  });
  it("uses account names in chat presence and history, survives hibernation, and revokes live sockets", async () => {
    const a = await player(); const signed = await chat(a);
    try {
      const person = await signed.client.next(0x86);
      expect(decoder.decode(person.subarray(34, 34 + person[33]))).toBe(a.grant.profile.name);
      expect(identity(person, 34 + person[33])).toEqual({ id: a.grant.profile.id, handle: a.handle });
      signed.client.socket.send(encodeChatIdentity("Someone else"));
      signed.client.socket.send(encodeChatPost(1, "Someone else", "Verified hello"));
      const message = await signed.client.next(0x81);
      const textSize = new DataView(message.buffer, message.byteOffset).getUint16(46);
      expect(decoder.decode(message.subarray(48, 48 + message[45]))).toBe(a.grant.profile.name);
      expect(identity(message, 48 + message[45] + textSize)).toEqual({ id: a.grant.profile.id, handle: a.handle });
      expect(decoder.decode(message)).not.toContain(a.grant.token);
      expect(decoder.decode(message)).not.toContain(await secretHash(a.grant.token));
      await evictDurableObject(env.GLOBAL_CHAT.getByName("friends"));
      const guest = await chat();
      try { expect(await guest.client.next(0x81)).toEqual(message); } finally { guest.client.socket.close(); }
      expect((await request(`/chat/connect?peer=${signed.peer}`)).status).toBe(403);
      await ok(a.request("logout", {}, a.grant.token));
      signed.client.socket.send(new Uint8Array([2]));
      await expect.poll(() => signed.client.socket.readyState).not.toBe(WebSocket.OPEN);
      expect((await request(`/chat/connect?peer=${signed.peer}`, a)).status).toBe(401);
    } finally { if (signed.client.socket.readyState === WebSocket.OPEN) signed.client.socket.close(); }
  });
  it("binds room peers to accounts and preserves seats while rejecting name spoofing", async () => {
    const a = await player(), b = await player(), code = roomId();
    const signed = await room(a, code);
    try {
      expect(signed.presence.participants[0]).toMatchObject({ accountId: a.grant.profile.id, accountHandle: a.handle, name: a.grant.profile.name, activeMask: 1 });
      signed.client.socket.send(encodeUpdate({ name: "Other player", activeMask: 1 }, 8192));
      const updated = (await signed.client.next(ServerMessageType.Presence)).snapshot;
      expect(updated.participants[0].name).toBe(a.grant.profile.name);
      await evictDurableObject(env.ROOMS.getByName(code));
      signed.client.socket.send(encodeRequestPresence(8192));
      expect((await signed.client.next(ServerMessageType.Presence)).snapshot.participants[0].accountId).toBe(a.grant.profile.id);
      const path = `/v1/rooms/${code}/connect?peer=${signed.peer}&name=Guest&players=1&build=identity-test`;
      expect((await request(path)).status).toBe(403); expect((await request(path, b)).status).toBe(403);
      const replacement = await room(a, code, signed.peer);
      try {
        expect(replacement.presence.participants).toHaveLength(1);
        expect(replacement.presence.participants[0].accountId).toBe(a.grant.profile.id);
        await ok(a.request("logout", {}, a.grant.token));
        replacement.client.socket.send(encodeRequestPresence(8192));
        await expect.poll(() => replacement.client.socket.readyState).not.toBe(WebSocket.OPEN);
      } finally { if (replacement.client.socket.readyState === WebSocket.OPEN) await replacement.client.close(); }
    } finally { if (signed.client.socket.readyState === WebSocket.OPEN) await signed.client.close(); }
  });
  it("delivers an invitation offline once and rechecks membership, friendship and expiry before joining", async () => {
    const a = await player(), b = await player(), code = roomId();
    const signed = await room(a, code);
    const invite = { to: b.handle, room: code, nonce: secret(), title: "Forged", body: "Forged" };
    try {
      expect((await a.social("invite", invite)).status).toBe(403);
      await friends(a, b);
      expect((await b.social("invite", { to: a.handle, room: code, nonce: secret() })).status).toBe(409);
      const sent = await ok(a.social("invite", invite));
      expect((await ok(a.social("invite", invite))).id).toBe(sent.id);
      const inbox = await b.state();
      const received = inbox.messages?.find(message => message.id === sent.id)!;
      expect(received).toMatchObject({ kind: "room_invite", room: code, peer: a.grant.profile.id });
      expect(received.title).not.toBe("Forged"); expect(received.expires).toBeGreaterThan(Date.now());
      await evictDurableObject(b.mailbox);
      expect((await ok(b.social("join-invite", { id: sent.id }))).room).toBe(code);
      expect((await a.social("join-invite", { id: sent.id })).status).toBe(404);
      await runInDurableObject(b.mailbox, (_instance, state) => { state.storage.sql.exec("UPDATE invitations SET expires=? WHERE id=?", Date.now() - 1, sent.id!); });
      expect((await b.social("join-invite", { id: sent.id })).status).toBe(410);
      await runInDurableObject(b.mailbox, (_instance, state) => { state.storage.sql.exec("UPDATE invitations SET expires=? WHERE id=?", Date.now() + 60_000, sent.id!); });
      await ok(b.social("block", { to: a.handle, blocked: true }));
      expect((await b.social("join-invite", { id: sent.id })).status).toBe(403);
      await ok(b.social("block", { to: a.handle, blocked: false })); await friends(a, b);
      await signed.client.close();
      expect((await b.social("join-invite", { id: sent.id })).status).toBe(409);
      expect((await ok(a.social("invite", invite))).id).toBe(sent.id);
    } finally { if (signed.client.socket.readyState === WebSocket.OPEN) await signed.client.close(); }
  });
});
