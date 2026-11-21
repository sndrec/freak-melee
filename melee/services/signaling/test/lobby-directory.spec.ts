import { guestHeaders } from "./guest";
import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { CONNECTION_TIMEOUT_MS } from "../src/connection-lease";
import { describe, expect, it } from "vitest";
import { ServerMessageType, decodeServerMessage, encodeSetListing, encodeRequestLobbies,
  encodeUpdate, encodeConfigureQuick, type ServerMessage } from "../src/wire";

const worker = exports.default as unknown as Fetcher;
function messages(socket: WebSocket) {
  const queue: ServerMessage[] = [];
  let received = Promise.resolve();
  socket.addEventListener("message", event => {
    received = received.then(async () => {
      const data = event.data as unknown;
      let buffer: ArrayBuffer;
      if (ArrayBuffer.isView(data)) buffer = new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice().buffer;
      else if (typeof data === "object" && data !== null && "arrayBuffer" in data && typeof data.arrayBuffer === "function") buffer = await data.arrayBuffer();
      else buffer = data as ArrayBuffer;
      queue.push(decodeServerMessage(buffer));
    });
  });
  return async (type: ServerMessageType) => {
    for (let i = 0; i < 200; ++i) {
      const index = queue.findIndex(message => message.type === type);
      if (index >= 0) return queue.splice(index, 1)[0];
      await new Promise(resolve => setTimeout(resolve, 10));
    }
    throw new Error(`Missing message ${type}`);
  };
}
async function connect(path: string) {
  const response = await worker.fetch(`https://signal.test${path}`, { headers: { Upgrade: "websocket", ...guestHeaders(new URL(path, "https://test").searchParams.get("peer") ?? "browser") } });
  expect(response.status).toBe(101);
  const socket = response.webSocket!;
  const next = messages(socket);
  socket.accept();
  return { socket, next };
}
async function closeAll(sockets: WebSocket[]) {
  await Promise.all(sockets.map(socket => new Promise<void>(resolve => {
    if (socket.readyState === WebSocket.CLOSED) { resolve(); return; }
    socket.addEventListener("close", () => resolve(), { once: true });
    socket.close();
  })));
  // Let the rooms' last-presence advertisements finish before test isolation
  // tears down the directory's RPC environment.
  await new Promise(resolve => setTimeout(resolve, 100));
}

describe("native lobby browser", () => {
  it("closes only the lobby revision inspected by an operator", async () => {
    const host = await connect("/v1/rooms/ADMIN777/connect?peer=host0000&name=Host&build=admin-build&players=1");
    const room = env.ROOMS.getByName("ADMIN777");
    try {
      await host.next(ServerMessageType.Welcome);
      host.socket.send(encodeSetListing("Stale lobby", true));
      await host.next(ServerMessageType.Listing);
      const state = await room.maintenanceState();
      expect(state.lobby?.title).toBe("Stale lobby");
      expect(await room.closeLobby(state.revision - 1)).toBe(false);
      expect(await room.directoryEntry()).not.toBeNull();
      expect(await room.closeLobby(state.revision)).toBe(true);
      expect(await room.directoryEntry()).toBeNull();
    } finally { await closeAll([host.socket]); }
  });

  it("lists real seats, protects host settings, filters builds and removes empty lobbies", async () => {
    const host = await connect("/v1/rooms/BROWSE77/connect?peer=host0000&name=Host&build=browser-build&players=2");
    const guest = await connect("/v1/rooms/BROWSE77/connect?peer=guest000&name=Guest&build=browser-build&players=1");
    const browser = await connect("/v1/lobbies/connect?build=browser-build");
    const sockets = [host.socket, guest.socket, browser.socket];
    async function list() {
      await new Promise(resolve => setTimeout(resolve, 270));
      browser.socket.send(encodeRequestLobbies(0));
      const result = await browser.next(ServerMessageType.Lobbies);
      if (result.type !== ServerMessageType.Lobbies) throw new Error("page expected");
      return result.entries;
    }
    try {
      await host.next(ServerMessageType.Welcome); await guest.next(ServerMessageType.Welcome);
      expect(await list()).toHaveLength(0);
      guest.socket.send(encodeSetListing("Forged lobby", true));
      expect(await guest.next(ServerMessageType.Error)).toMatchObject({ message: expect.stringContaining("host") });
      host.socket.send(encodeSetListing("Twilight's lobby", true));
      await host.next(ServerMessageType.Listing);
      expect(await list()).toEqual([expect.objectContaining({ code: "BROWSE77", title: "Twilight's lobby",
        host: "Host", fighting: 3, waiting: 0, spectating: 0, participants: 2 })]);
      guest.socket.send(encodeUpdate({ activeMask: 0, waitingMask: 0 }, 1024));
      await guest.next(ServerMessageType.Presence);
      expect(await list()).toEqual([expect.objectContaining({ fighting: 2, spectating: 1 })]);
      const other = await connect("/v1/lobbies/connect?build=other-build"); sockets.push(other.socket);
      other.socket.send(encodeRequestLobbies(0));
      expect(await other.next(ServerMessageType.Lobbies)).toMatchObject({ total: 0, entries: [] });
      host.socket.send(encodeSetListing("Friends only", false)); await host.next(ServerMessageType.Listing);
      expect(await list()).toHaveLength(0);
      host.socket.send(encodeSetListing("Listed again", true)); await host.next(ServerMessageType.Listing);
      expect(await list()).toHaveLength(1);
      host.socket.close(); guest.socket.close();
      expect(await list()).toHaveLength(0);
    } finally { await closeAll(sockets); }
  });

  it("paginates, survives object eviction, and rejects stale advertisement updates", async () => {
    const directory = env.LOBBY_DIRECTORY.getByName("public");
    const browser = await connect("/v1/lobbies/connect?build=page-build");
    const sockets = [browser.socket];
    try {
      for (let i = 0; i < 10; ++i) {
        const room = await connect(`/v1/rooms/PAGE000${i}/connect?peer=host000${i}&name=Host&build=page-build&players=1`);
        sockets.push(room.socket);
        room.socket.send(encodeSetListing(`Lobby ${i}`, true));
        await room.next(ServerMessageType.Listing);
      }
      await evictDurableObject(env.ROOMS.getByName("PAGE0000"));
      await evictDurableObject(directory);
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 10, offset: 0, entries: { length: 8 } });
      await new Promise(resolve => setTimeout(resolve, 270));
      browser.socket.send(encodeRequestLobbies(8));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 10, offset: 8,
        entries: [expect.objectContaining({ code: "PAGE0008" }), expect.objectContaining({ code: "PAGE0009" })] });
      const oldEntry = await env.ROOMS.getByName("PAGE0000").directoryEntry();
      expect(oldEntry).not.toBeNull();
      const issued = Date.now();
      await directory.advertise("PAGE0000", 100, issued, null, 0);
      await directory.advertise("PAGE0000", 99, issued, oldEntry, issued + CONNECTION_TIMEOUT_MS);
      await new Promise(resolve => setTimeout(resolve, 270));
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 9,
        entries: [expect.objectContaining({ code: "PAGE0001" }), ...Array(7).fill(expect.anything())] });
    } finally { await closeAll(sockets); }
  });

  it("reads compatible summaries locally and retires expired listings and withdrawals", async () => {
    const directory = env.LOBBY_DIRECTORY.getByName("public"), now = Date.now();
    const entry = { code: "ZLEASE77", title: "Fresh", host: "Host", build: "lease-build", fighting: 1,
      capacity: 4, locked: false, waiting: 0, spectating: 0, participants: 1, playing: false };
    await directory.advertise(entry.code, 1, now, entry, now + CONNECTION_TIMEOUT_MS);
    await directory.advertise("EXPIRE77", 2, now, { ...entry, code: "EXPIRE77" }, now);
    await directory.advertise("HIDDEN77", 3, now, null, 0);
    await runInDurableObject(directory, (_instance, state) => {
      state.storage.sql.exec(`WITH RECURSIVE n(i) AS (VALUES(1) UNION ALL SELECT i+1 FROM n WHERE i<513)
        INSERT INTO lobby_summaries SELECT printf('A%07d',i), revision, issued, 'other-build', entry, visible_until, retire_at
        FROM n, lobby_summaries WHERE code='ZLEASE77'`);
    });
    const browser = await connect("/v1/lobbies/connect?build=lease-build");
    try {
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 1, entries: [{ code: entry.code, title: entry.title }] });
      await runInDurableObject(directory, (_instance, state) => {
        state.storage.sql.exec("UPDATE lobby_summaries SET retire_at=0 WHERE code IN ('EXPIRE77','HIDDEN77')");
      });
      await runDurableObjectAlarm(directory);
      await directory.advertise("HIDDEN77", 2, now - CONNECTION_TIMEOUT_MS, { ...entry, code: "HIDDEN77" }, now);
      const retained = await runInDurableObject(directory, (_instance, state) =>
        state.storage.sql.exec("SELECT code FROM lobby_summaries WHERE code IN ('EXPIRE77','HIDDEN77','ZLEASE77')").toArray());
      expect(retained).toEqual([{ code: entry.code }]);
    } finally { await closeAll([browser.socket]); }
  });

  it("rebuilds a live lobby summary on the room heartbeat without a settings change", async () => {
    const host = await connect("/v1/rooms/RENEW777/connect?peer=host0000&name=Host&build=renew-build&players=1");
    const browser = await connect("/v1/lobbies/connect?build=renew-build");
    const room = env.ROOMS.getByName("RENEW777"), directory = env.LOBBY_DIRECTORY.getByName("public");
    try {
      host.socket.send(encodeSetListing("Renewed", true)); await host.next(ServerMessageType.Listing);
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 1 });
      await runInDurableObject(directory, (_instance, state) => {
        state.storage.sql.exec("DELETE FROM lobby_summaries WHERE code='RENEW777'");
      });
      await evictDurableObject(room);
      await runDurableObjectAlarm(room);
      await new Promise(resolve => setTimeout(resolve, 270));
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 1, entries: [{ title: "Renewed" }] });
    } finally { await closeAll([host.socket, browser.socket]); }
  });

  it("keeps Quick Play parties out of the directory", async () => {
    const host = await connect("/v1/rooms/QUICKD77/connect?peer=host0000&name=Host&build=quick-directory&players=1");
    const browser = await connect("/v1/lobbies/connect?build=quick-directory");
    try {
      host.socket.send(encodeSetListing("Play", true)); await host.next(ServerMessageType.Listing);
      host.socket.send(encodeConfigureQuick(1)); await host.next(ServerMessageType.QuickState);
      browser.socket.send(encodeRequestLobbies(0));
      expect(await browser.next(ServerMessageType.Lobbies)).toMatchObject({ total: 0 });
      host.socket.send(encodeSetListing("Try to expose party", true));
      expect(await host.next(ServerMessageType.Error)).toMatchObject({ message: expect.stringContaining("regular lobby") });
    } finally { await closeAll([host.socket, browser.socket]); }
  });
});
