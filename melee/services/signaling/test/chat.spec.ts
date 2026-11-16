import { env, exports } from "cloudflare:workers";
import { evictDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { CHAT_HISTORY, encodeChatPost, encodeChatIdentity } from "../src/chat";

const worker = exports.default as unknown as Fetcher;
const firstPeer = "11111111111111111111111111111111";
const secondPeer = "22222222222222222222222222222222";
const decoder = new TextDecoder();

async function connect(target: Fetcher, peer: string) {
  const response = await target.fetch(`https://chat.test/chat/connect?peer=${peer}`, {
    headers: { Upgrade: "websocket" },
  });
  expect(response.status).toBe(101);
  const socket = response.webSocket!;
  const queued: Uint8Array[] = [];
  let wake: (() => void) | undefined;
  let received = Promise.resolve();
  socket.addEventListener("message", (event) => {
    received = received.then(async () => {
      const data = event.data as unknown;
      let buffer: ArrayBuffer;
      if (ArrayBuffer.isView(data)) buffer = new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice().buffer;
      else if (typeof data === "object" && data !== null && "arrayBuffer" in data && typeof data.arrayBuffer === "function") buffer = await data.arrayBuffer();
      else buffer = data as ArrayBuffer;
      queued.push(new Uint8Array(buffer));
      wake?.();
    });
  });
  socket.accept();
  return {
    socket,
    async next(type: number): Promise<Uint8Array> {
      for (;;) {
        while (queued.length) {
          const bytes = queued.shift()!;
          if (bytes[0] === type) return bytes;
        }
        await new Promise<void>((resolve, reject) => {
          const timer = setTimeout(() => reject(new Error(`Missing chat packet ${type}`)), 2000);
          wake = () => { clearTimeout(timer); wake = undefined; resolve(); };
        });
      }
    },
  };
}

function entry(bytes: Uint8Array) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    id: view.getBigUint64(1), nonce: view.getUint32(9),
    peer: decoder.decode(bytes.subarray(13, 45)),
    name: decoder.decode(bytes.subarray(48, 48 + bytes[45])),
    text: decoder.decode(bytes.subarray(48 + bytes[45], 48 + bytes[45] + view.getUint16(46))),
  };
}

async function players(connection: Awaited<ReturnType<typeof connect>>) {
  const first = await connection.next(0x85);
  const count = new DataView(first.buffer, first.byteOffset, first.byteLength).getUint16(1);
  const result = [];
  for (let i = 0; i < count; ++i) {
    const record = await connection.next(0x86);
    result.push({ peer: decoder.decode(record.subarray(1, 33)),
                  name: decoder.decode(record.subarray(34, 34 + record[33])) });
    expect(record[33] + 2).toBe(record.length - 34);
  }
  expect(Array.from(await connection.next(0x87))).toEqual([0x87]);
  return result;
}

describe("global chat", () => {
  it("removes a ghost peer without removing a player who sent a newer heartbeat", async () => {
    const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
    const a = await connect(stub, firstPeer), b = await connect(stub, secondPeer);
    try {
      await a.next(0x82); await b.next(0x82);
      a.socket.send(new Uint8Array([2])); await a.next(0x84);
      expect(await stub.disconnectPlayer(firstPeer, 0)).toBe(false);
      expect(await stub.disconnectPlayer(secondPeer, 0)).toBe(true);
      expect(await stub.connectedPlayers()).toEqual([
        expect.objectContaining({ peer: firstPeer, lastMessageAt: expect.any(Number) }),
      ]);
    } finally { a.socket.close(); b.socket.close(); }
  });

  it("routes everyone without a room or build requirement", async () => {
    const a = await connect(worker, firstPeer);
    await a.next(0x82);
    const b = await connect(worker, secondPeer);
    await b.next(0x82);
    a.socket.send(encodeChatPost(1, "Renée", "Hello <friends> & everyone!"));
    const mine = entry(await a.next(0x81));
    expect(mine).toMatchObject({ nonce: 1, peer: firstPeer, name: "Renée", text: "Hello <friends> & everyone!" });
    expect(entry(await b.next(0x81))).toEqual(mine);
    a.socket.close(); b.socket.close();
  });

  it("persists history through hibernation and acknowledges retries only once", async () => {
    const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
    const a = await connect(stub, firstPeer);
    await a.next(0x82);
    const post = encodeChatPost(77, "Player", "Keep this message");
    a.socket.send(post);
    const original = entry(await a.next(0x81));
    await evictDurableObject(stub);
    a.socket.send(post);
    expect(entry(await a.next(0x81))).toEqual(original);
    const b = await connect(stub, secondPeer);
    expect(entry(await b.next(0x81))).toEqual(original);
    await b.next(0x82);
    b.socket.send(encodeChatPost(1, "Friend", "After hibernation"));
    expect(entry(await b.next(0x81)).id).toBe(original.id + 1n);
    a.socket.close(); b.socket.close();
  });

  it("keeps only the last 64 messages, with monotonic IDs", async () => {
    const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
    const a = await connect(stub, firstPeer);
    await a.next(0x82);
    for (let n = 1; n <= 70; ++n) {
      a.socket.send(encodeChatPost(n, "Player", String(n)));
      expect(entry(await a.next(0x81)).nonce).toBe(n);
    }
    const b = await connect(stub, secondPeer);
    for (let n = 71 - CHAT_HISTORY; n <= 70; ++n) {
      expect(entry(await b.next(0x81)).nonce).toBe(n);
    }
    await b.next(0x82);
    a.socket.close(); b.socket.close();
  });

  it("reports presence and responds to the liveness probe", async () => {
    const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
    const a = await connect(stub, firstPeer);
    expect(Array.from(await a.next(0x82))).toEqual([0x82, 0, 1]);
    expect(await players(a)).toEqual([{ peer: firstPeer, name: "Player" }]);
    a.socket.send(encodeChatIdentity("Renée"));
    expect(await players(a)).toEqual([{ peer: firstPeer, name: "Renée" }]);
    await evictDurableObject(stub);
    const b = await connect(stub, secondPeer);
    expect(Array.from(await b.next(0x82))).toEqual([0x82, 0, 2]);
    expect(await players(a)).toEqual(expect.arrayContaining([
      { peer: firstPeer, name: "Renée" }, { peer: secondPeer, name: "Player" },
    ]));
    b.socket.close();
    expect(await players(a)).toEqual([{ peer: firstPeer, name: "Renée" }]);
    a.socket.send(new Uint8Array([2]));
    expect(Array.from(await a.next(0x84))).toEqual([0x84]);
    a.socket.close();
  });

  it("announces players before their first message and updates their names", async () => {
    const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
    const a = await connect(stub, firstPeer);
    await a.next(0x82); await players(a);
    a.socket.send(encodeChatIdentity("Quiet player"));
    expect(await players(a)).toEqual([{ peer: firstPeer, name: "Quiet player" }]);
    a.socket.send(encodeChatIdentity("New name"));
    expect(await players(a)).toEqual([{ peer: firstPeer, name: "New name" }]);
    a.socket.send(new Uint8Array([2]));
    expect(Array.from(await a.next(0x84))).toEqual([0x84]);
    a.socket.close();
  });

  it("rejects oversized text and invalid UTF-8 instead of corrupting history", async () => {
    for (const post of [encodeChatPost(1, "Player", "x".repeat(513)),
      encodeChatIdentity("x".repeat(33)), encodeChatIdentity(" "),
      new Uint8Array([3, 1, 255]),
      new Uint8Array([1, 0, 0, 0, 1, 1, 0, 1, 65, 255])]) {
      const stub = env.GLOBAL_CHAT.getByName(crypto.randomUUID());
      const a = await connect(stub, firstPeer);
      await a.next(0x82);
      const closed = new Promise<number>((resolve) => a.socket.addEventListener("close", e => resolve(e.code)));
      a.socket.send(post);
      expect(await closed).toBe(1008);
    }
  });
});
