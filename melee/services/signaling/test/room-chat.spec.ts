import { guestHeaders } from "./guest";
import { env, exports } from "cloudflare:workers";
import { evictDurableObject, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { decodeServerMessage, encodePostChat, encodeRequestChat, encodeRequestPresence,
  encodeUpdate, ServerMessageType, type ServerMessage } from "../src/wire";

const worker = exports.default as unknown as Fetcher;
class Client {
  private messages: ServerMessage[] = [];
  private changed?: () => void;
  private receiving = Promise.resolve();
  private error?: unknown;
  constructor(readonly socket: WebSocket) {
    socket.addEventListener("message", (event) => {
      const data: unknown = event.data;
      this.receiving = this.receiving.then(async () => {
        let bytes: ArrayBuffer;
        if (ArrayBuffer.isView(data)) bytes = new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice().buffer;
        else if (typeof data === "object" && data !== null && "arrayBuffer" in data && typeof data.arrayBuffer === "function")
          bytes = await data.arrayBuffer() as ArrayBuffer;
        else bytes = data as ArrayBuffer;
        this.messages.push(decodeServerMessage(bytes));
        this.changed?.();
      }).catch((error: unknown) => { this.error = error; this.changed?.(); });
    });
    socket.accept();
  }
  async next<T extends ServerMessageType>(type: T): Promise<Extract<ServerMessage, { type: T }>> {
    for (;;) {
      if (this.error !== undefined) throw this.error;
      const index = this.messages.findIndex((message) => message.type === type);
      if (index >= 0) return this.messages.splice(index, 1)[0] as Extract<ServerMessage, { type: T }>;
      await new Promise<void>((resolve, reject) => {
        const timeout = setTimeout(() => { this.changed = undefined; reject(new Error(`Chat message ${type} timed out`)); }, 2000);
        this.changed = () => { clearTimeout(timeout); this.changed = undefined; resolve(); };
      });
    }
  }
  async history() {
    this.socket.send(encodeRequestChat());
    const begin = await this.next(ServerMessageType.ChatBegin);
    const entries = [];
    for (let i = 0; i < begin.count; ++i) entries.push((await this.next(ServerMessageType.ChatEntry)).entry);
    const end = await this.next(ServerMessageType.ChatEnd);
    expect(end.sequence).toBe(entries.at(-1)?.sequence ?? 0);
    return entries;
  }
  send(nonce: number, text: string) { this.socket.send(encodePostChat(nonce, text)); }
  close() { this.socket.close(1000, "Test finished"); }
}
async function join(room: string, peer: string, name: string): Promise<Client> {
  const query = new URLSearchParams({ peer, name, players: "0", build: "room-chat-test", active: "0", waiting: "0" });
  const response = await worker.fetch(`https://test/v1/rooms/${room}/connect?${query}`, { headers: { Upgrade: "websocket", ...guestHeaders(peer) } });
  expect(response.status).toBe(101);
  const client = new Client(response.webSocket!);
  await client.next(ServerMessageType.Welcome);
  await client.next(ServerMessageType.Presence);
  return client;
}

describe("private lobby chat", () => {
  it("shares authoritative names with spectators, deduplicates retry, isolates rooms and survives hibernation", async () => {
    const a = await join("CHATROOM1", "player_one", "First");
    const b = await join("CHATROOM1", "spectator_two", "Spectator");
    const other = await join("CHATROOM2", "other_room", "Other");
    try {
      expect(await a.history()).toEqual([]);
      expect(await b.history()).toEqual([]);
      expect(await other.history()).toEqual([]);
      a.send(1, "Hello 🌙");
      const first = (await a.next(ServerMessageType.ChatEntry)).entry;
      expect(first).toMatchObject({ peerId: "player_one", name: "First", nonce: 1, text: "Hello 🌙" });
      expect((await b.next(ServerMessageType.ChatEntry)).entry).toEqual(first);
      a.send(1, "Hello 🌙");
      expect((await a.next(ServerMessageType.ChatEntry)).entry).toEqual(first);
      b.send(1, "Spectators can talk");
      expect((await a.next(ServerMessageType.ChatEntry)).entry.name).toBe("Spectator");
      await b.next(ServerMessageType.ChatEntry);
      a.socket.send(encodeUpdate({ name: "Renamed" }, 16384));
      await a.next(ServerMessageType.Presence);
      await evictDurableObject(env.ROOMS.get(env.ROOMS.idFromName("CHATROOM1")));
      const c = await join("CHATROOM1", "later_reader", "Later");
      try {
        const history = await c.history();
        expect(history.map((entry) => entry.text)).toEqual(["Hello 🌙", "Spectators can talk"]);
        expect(history[0].name).toBe("First");
      } finally { c.close(); }
      const separate = await join("CHATROOM2", "other_reader", "Separate");
      try { expect(await separate.history()).toEqual([]); } finally { separate.close(); }
    } finally { a.close(); b.close(); other.close(); }
  });

  it("rejects invalid/rate-limited chat without breaking room signaling", async () => {
    const a = await join("CHATERROR", "invalid_peer", "Tester");
    try {
      await a.history();
      a.send(1, "Good message");
      await a.next(ServerMessageType.ChatEntry);
      a.send(2, "Too soon");
      expect((await a.next(ServerMessageType.ChatRejected)).nonce).toBe(2);
      a.send(3, "bad\u0000text");
      expect((await a.next(ServerMessageType.ChatRejected)).nonce).toBe(3);
      a.send(4, "x".repeat(513));
      expect((await a.next(ServerMessageType.ChatRejected)).nonce).toBe(4);
      a.send(5, "   ");
      expect((await a.next(ServerMessageType.ChatRejected)).nonce).toBe(5);
      a.send(1, "Different retry body");
      expect((await a.next(ServerMessageType.ChatRejected)).nonce).toBe(1);
      const malformed = new Uint8Array(encodePostChat(6, "abc"));
      malformed[malformed.length - 1] = 0xff;
      a.socket.send(malformed);
      await a.next(ServerMessageType.ChatRejected);
      a.socket.send(encodeRequestPresence(16384));
      expect((await a.next(ServerMessageType.Presence)).snapshot.participants).toHaveLength(1);
    } finally { a.close(); }
  });

  it("retains only 64 entries and clears history when the lobby becomes empty", async () => {
    const room = "CHATBOUND";
    const a = await join(room, "bounded_peer", "Tester");
    try {
      await a.history();
      const stub = env.ROOMS.get(env.ROOMS.idFromName(room));
      await runInDurableObject(stub, async (_instance, state) => {
        for (let i = 1; i <= 70; ++i) state.storage.sql.exec(
          "INSERT INTO room_chat (connection, nonce, peer, name, text) VALUES (?, ?, ?, ?, ?)",
          "seed", i, "seed_peer", "Seed", `Message ${i}`);
      });
      a.send(1, "Newest");
      await a.next(ServerMessageType.ChatEntry);
      const b = await join(room, "bounded_reader", "Reader");
      try {
        const history = await b.history();
        expect(history).toHaveLength(64);
        expect(history[0].text).toBe("Message 8");
        expect(history.at(-1)?.text).toBe("Newest");
      } finally { b.close(); }
    } finally { a.close(); }
    const stub = env.ROOMS.get(env.ROOMS.idFromName(room));
    // A local RPC can arrive before the socket close event reaches the object.
    await expect.poll(() => runInDurableObject(stub, async (_instance, state) =>
      state.storage.sql.exec("SELECT * FROM room_chat").toArray().length)).toBe(0);
    const fresh = await join(room, "fresh_reader", "Fresh");
    try { expect(await fresh.history()).toEqual([]); } finally { fresh.close(); }
  });
});

