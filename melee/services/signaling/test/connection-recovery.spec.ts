import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject, runDurableObjectAlarm } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { CONNECTION_TIMEOUT_MS, CONNECTION_LIFETIME_MS } from "../src/connection-lease";
import type { ConnectionAttachment } from "../src/protocol";
import { encodeSetListing, encodeRequestPresence, ServerMessageType } from "../src/wire";
import { player } from "./account-client";
import { guestHeaders } from "./guest";
import { QuickClient } from "./quick-client";

const worker = exports.default as unknown as Fetcher;
const code = () => crypto.randomUUID().replaceAll("-", "").slice(0, 12).toUpperCase();
const room = (name: string) => env.ROOMS.getByName(name);

async function join(name: string, peer: string, identity = peer, account?: Awaited<ReturnType<typeof player>>) {
  const response = await worker.fetch(`https://test/v1/rooms/${name}/connect?peer=${peer}&name=Player&build=recovery&players=1`, {
    headers: { Upgrade: "websocket", ...guestHeaders(identity), ...(account ? {
      Authorization: `Bearer ${account.grant.token}`, "X-Freak-Account": account.handle,
    } : {}) },
  });
  expect(response.status).toBe(101);
  const client = new QuickClient(response.webSocket!);
  const welcome = await client.next(ServerMessageType.Welcome);
  client.peerId = welcome.peerId; client.connectionId = welcome.connectionId;
  await client.next(ServerMessageType.Presence);
  return client;
}

async function list(name: string, client: QuickClient) {
  client.socket.send(encodeSetListing("Recovery lobby", true));
  await client.next(ServerMessageType.Listing);
  expect((await room(name).directoryEntry())?.title).toBe("Recovery lobby");
}

async function age(name: string, peerId: string, milliseconds: number) {
  await runInDurableObject(room(name), (_instance, state) => {
    for (const socket of state.getWebSockets()) {
      const peer = socket.deserializeAttachment() as ConnectionAttachment;
      if (peer.peerId === peerId) socket.serializeAttachment({ ...peer, lastSeenAt: Date.now() - milliseconds });
    }
  });
}

describe("connection leases and membership", () => {
  it("hides an unreachable lobby, preserves it through hibernation, and restores it on heartbeat", async () => {
    const name = code(), client = await join(name, "recovery_host");
    try {
      await list(name, client);
      await age(name, client.peerId, CONNECTION_TIMEOUT_MS + 1);
      await runDurableObjectAlarm(room(name));
      expect(await room(name).directoryEntry()).toBeNull();
      await evictDurableObject(room(name));
      expect(await room(name).directoryEntry()).toBeNull();
      client.socket.send("ping");
      await expect.poll(() => client.pongs).toBe(1);
      expect((await room(name).directoryEntry())?.participants).toBe(1);
      expect(client.socket.readyState).toBe(WebSocket.OPEN);
    } finally { await client.close(); }
  });

  it("removes a timed-out host while keeping the other participant and lobby alive", async () => {
    const name = code(), host = await join(name, "timeout_host"), other = await join(name, "timeout_other");
    try {
      await list(name, host);
      await age(name, host.peerId, CONNECTION_LIFETIME_MS + 1);
      await runDurableObjectAlarm(room(name));
      expect(host.socket.readyState).not.toBe(WebSocket.OPEN);
      expect(other.socket.readyState).toBe(WebSocket.OPEN);
      expect((await room(name).directoryEntry())?.participants).toBe(1);
      other.socket.send(encodeRequestPresence(16384));
      let presence;
      do { presence = (await other.next(ServerMessageType.Presence)).snapshot; }
      while (presence.participants.length !== 1);
      expect(presence.ownerPeerId).toBe(other.peerId);
    } finally { await host.close(); await other.close(); }
  });

  it("reserves a seat after a transport error and reconnects with the original join order", async () => {
    const name = code(), host = await join(name, "reserve_host"), other = await join(name, "reserve_other");
    let replacement: QuickClient | undefined;
    try {
      await list(name, host);
      const original = await runInDurableObject(room(name), (_instance, state) =>
        (state.getWebSockets(`peer:${host.peerId}`)[0].deserializeAttachment() as ConnectionAttachment).joinOrder);
      await runInDurableObject(room(name), async (instance, state) =>
        instance.webSocketError(state.getWebSockets(`peer:${host.peerId}`)[0]));
      expect((await room(name).directoryEntry())?.participants).toBe(2);
      await evictDurableObject(room(name));
      replacement = await join(name, "returned_host", "reserve_host");
      const restored = await runInDurableObject(room(name), (_instance, state) =>
        state.getWebSockets(`peer:${replacement!.peerId}`)[0].deserializeAttachment() as ConnectionAttachment);
      expect(restored.joinOrder).toBe(original);
      expect(restored.ready).toBe(false);
      expect((await room(name).directoryEntry())?.participants).toBe(2);
    } finally { await host.close(); await other.close(); await replacement?.close(); }
  });

  it("keeps the empty lobby recoverable until the grace period ends", async () => {
    const name = code(), host = await join(name, "last_host");
    await list(name, host);
    await runInDurableObject(room(name), async (instance, state) => instance.webSocketError(state.getWebSockets()[0]));
    await runInDurableObject(room(name), (_instance, state) => {
      expect(state.storage.sql.exec<{listed: number}>("SELECT listed FROM room_listing").one().listed).toBe(1);
      expect(state.storage.sql.exec("SELECT * FROM room_recovery").toArray()).toHaveLength(1);
      state.storage.sql.exec("UPDATE room_recovery SET expires=?", Date.now() - 1);
    });
    await runDurableObjectAlarm(room(name));
    await runInDurableObject(room(name), async (_instance, state) => {
      expect(state.storage.sql.exec<{listed: number}>("SELECT listed FROM room_listing").one().listed).toBe(0);
      expect(await state.storage.getAlarm()).toBeNull();
    });
    await host.close();
  });

  it("moves an account between lobbies and ignores late cleanup from its old socket", async () => {
    const account = await player(), first = code(), second = code();
    const old = await join(first, "account_old", "unused", account);
    await list(first, old);
    const current = await join(second, "account_new", "unused", account);
    try {
      expect(old.socket.readyState).not.toBe(WebSocket.OPEN);
      expect(await room(first).directoryEntry()).toBeNull();
      await env.PLAYER_SESSIONS.getByName(`account:${account.grant.profile.id}`).leave(old.connectionId);
      current.socket.send("ping"); await expect.poll(() => current.pongs).toBe(1);
      expect(current.socket.readyState).toBe(WebSocket.OPEN);
    } finally { await old.close(); await current.close(); }
  });

  it("gives a guest one membership without merging different guests named Player", async () => {
    const name = code(), first = await join(name, "guest_first", "installation_a");
    const friend = await join(name, "guest_friend", "installation_b");
    await list(name, first);
    const replacement = await join(name, "guest_return", "installation_a");
    try {
      expect(first.socket.readyState).not.toBe(WebSocket.OPEN);
      expect(friend.socket.readyState).toBe(WebSocket.OPEN);
      expect((await room(name).directoryEntry())?.participants).toBe(2);
      replacement.socket.send("ping"); await expect.poll(() => replacement.pongs).toBe(1);
    } finally { await first.close(); await friend.close(); await replacement.close(); }
  });

  it("fences simultaneous joins so only one lobby retains the player", async () => {
    const names = [code(), code(), code()];
    const responses = await Promise.all(names.map((name, index) => worker.fetch(
      `https://test/v1/rooms/${name}/connect?peer=concurrent_${index}&name=Player&build=recovery&players=1`,
      { headers: { Upgrade: "websocket", ...guestHeaders("one_installation") } })));
    const sockets = responses.filter(response => response.status === 101).map(response => response.webSocket!);
    for (const socket of sockets) socket.accept();
    try {
      expect(responses.every(response => response.status === 101 || response.status === 409)).toBe(true);
      const memberships = await Promise.all(names.map(name => runInDurableObject(room(name), (_instance, state) =>
        state.getWebSockets().filter(socket => socket.readyState === WebSocket.OPEN).length)));
      expect(memberships.reduce((sum, count) => sum + count, 0)).toBe(1);
    } finally { for (const socket of sockets) socket.close(); }
  });

  it("replaces the old chat connection for the same account", async () => {
    const account = await player();
    const connect = async () => {
      const peer = crypto.randomUUID().replaceAll("-", "");
      const response = await worker.fetch(`https://test/chat/connect?peer=${peer}`, { headers: {
        Upgrade: "websocket", Authorization: `Bearer ${account.grant.token}`, "X-Freak-Account": account.handle,
      } });
      expect(response.status).toBe(101);
      response.webSocket!.accept();
      return { peer, socket: response.webSocket! };
    };
    const first = await connect(), latest = await connect();
    try {
      expect(first.socket.readyState).not.toBe(WebSocket.OPEN);
      expect(latest.socket.readyState).toBe(WebSocket.OPEN);
      expect((await env.GLOBAL_CHAT.getByName("friends").connectedPlayers()).map(peer => peer.peer)).toEqual([latest.peer]);
    } finally { first.socket.close(); latest.socket.close(); }
  });

  it("keeps an account membership when the same socket attaches it again", async () => {
    const account = await player(), name = code(), guest = await join(name, "attach_twice");
    try {
      const identity = await env.ACCOUNTS.getByName(account.handle).openConnection(`Bearer ${account.grant.token}`);
      expect(identity).not.toBeNull();
      expect(await room(name).attachAccount(guest.connectionId, identity!)).toBe(true);
      expect(await room(name).attachAccount(guest.connectionId, identity!)).toBe(true);
      expect(await env.PLAYER_SESSIONS.getByName(`account:${account.grant.profile.id}`).owns(guest.connectionId)).toBe(true);
    } finally { await guest.close(); }
  });

  it("retires an obsolete socket without stranding the replacement in an old match", async () => {
    const name = code(), old = await join(name, "obsolete_host");
    await runInDurableObject(room(name), (_instance, state) => {
      const socket = state.getWebSockets()[0];
      const attachment = socket.deserializeAttachment() as Partial<ConnectionAttachment>;
      delete attachment.playerIdentity;
      socket.serializeAttachment(attachment);
      state.storage.sql.exec("UPDATE room_match SET payload=?", new Uint8Array([1, 2, 3]));
    });
    const current = await join(name, "updated_host");
    try {
      expect(old.socket.readyState).not.toBe(WebSocket.OPEN);
      await list(name, current);
      expect((await room(name).directoryEntry())?.fighting).toBe(1);
      await runInDurableObject(room(name), (_instance, state) =>
        expect(state.storage.sql.exec<{payload: ArrayBuffer | null}>("SELECT payload FROM room_match").one().payload).toBeNull());
    } finally { await old.close(); await current.close(); }
  });
});
