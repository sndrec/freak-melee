import { env } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { quickJoin, QuickClient } from "./quick-client";
import { encodePrepareQuick, encodeRequestPresence, encodeUpdate, ServerMessageType } from "../src/wire";
import { lockedChoices, buildQuickMatch } from "../src/quick-match";
import type { QuickAllocation } from "../src/quick-play";
import { publicParticipant } from "../src/protocol";

const digest = new Uint8Array(32).fill(0x42);
function choices(players: number, confirmed = true): Uint8Array {
  const bytes = new Uint8Array(7 + 50 * players);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, 0x464d534c); bytes[4] = 1; bytes[5] = players; bytes[6] = confirmed ? 1 : 0;
  for (let i = 0; i < players; ++i) {
    bytes[7 + i * 50] = 3;
    bytes.fill(i + 1, 25 + i * 50, 57 + i * 50);
  }
  return bytes;
}
async function phase(client: QuickClient, expected: number) {
  for (;;) {
    const state = await client.state(2);
    if (state.phase === expected) return state;
  }
}
async function setup(mode: 1 | 2 = 1) {
  const names = [0, 1].map(() => crypto.randomUUID().replaceAll("-", "").slice(0, 8).toUpperCase());
  const parties = [await quickJoin(names[0], "countdown_a", mode), await quickJoin(names[1], "countdown_b", mode)];
  await parties[0].configure(mode); await parties[1].configure(mode);
  await parties[0].search(); await parties[1].search();
  const assigned = [await parties[0].state(2), await parties[1].state(2)];
  const room = env.ROOMS.getByName(assigned[0].matchRoom);
  const clients = [await quickJoin(assigned[0].matchRoom, "countdown_a", mode, "quick-test", assigned[0].admission),
    await quickJoin(assigned[1].matchRoom, "countdown_b", mode, "quick-test", assigned[1].admission)];
  return { room, clients, assigned, mode, async close() { await Promise.all([...clients, ...parties].map((c) => c.close())); } };
}
async function select(client: QuickClient, players: number, confirmed = true) {
  const selection = choices(players, confirmed);
  client.socket.send(encodeUpdate({ selection }, 16384));
  client.socket.send(encodeRequestPresence(16384));
  for (;;) {
    const presence = (await client.next(ServerMessageType.Presence)).snapshot;
    if (presence.participants.some((p) => p.peerId === client.peerId &&
        p.selection.length === selection.length && p.selection[6] === selection[6])) return;
  }
}
async function revision(game: Awaited<ReturnType<typeof setup>>) {
  return runInDurableObject(game.room, async (_instance, state) =>
    state.storage.sql.exec<{revision: number}>("SELECT revision FROM room_counter").one().revision);
}
async function prepare(game: Awaited<ReturnType<typeof setup>>) {
  const current = await revision(game);
  for (const client of game.clients) client.socket.send(encodePrepareQuick(current, true, digest));
  return phase(game.clients[0], 1);
}
async function expire(game: Awaited<ReturnType<typeof setup>>) {
  await runInDurableObject(game.room, async (_instance, state) => {
    state.storage.sql.exec("UPDATE quick_flow SET deadline=? WHERE id=1", Date.now() - 1);
  });
  await evictDurableObject(game.room);
  await runDurableObjectAlarm(game.room);
}

describe("Quick Play ready countdown", () => {
  it("requires complete locked selections and parses every shared-PC choice", () => {
    const bytes = choices(2);
    expect(lockedChoices(bytes, 2)).toHaveLength(2);
    expect(lockedChoices(bytes.subarray(0, bytes.length - 1), 2)).toBeNull();
    expect(lockedChoices(bytes, 1)).toBeNull();
    expect(lockedChoices(choices(2, false), 2)).toBeNull();
    bytes[57] = 0;
    expect(lockedChoices(bytes, 2)).toBeNull();
  });

  it("waits for every prepared client, cancels on unready and publishes once after hibernation", async () => {
    const game = await setup();
    try {
      for (const c of game.clients) await select(c, 1);
      const current = await revision(game);
      game.clients[0].socket.send(encodePrepareQuick(current, true, digest));
      await expect.poll(async () => runInDurableObject(game.room, async (_i, state) =>
        state.storage.sql.exec<{phase: number}>("SELECT phase FROM quick_flow").one().phase)).toBe(0);
      game.clients[1].socket.send(encodePrepareQuick(current, true, new Uint8Array(32).fill(1)));
      expect((await game.clients[1].refresh()).phase).toBe(0);
      game.clients[1].socket.send(encodePrepareQuick(current, true, digest));
      const countdown = await phase(game.clients[0], 1);
      expect(countdown.remainingMs).toBeGreaterThan(2500);
      await runDurableObjectAlarm(game.room); // An early alarm cannot start the match.
      expect((await game.clients[0].refresh()).phase).toBe(1);
      await select(game.clients[1], 1, false);
      await phase(game.clients[0], 0);
      await runInDurableObject(game.room, async (_i, state) => expect(await state.storage.getAlarm()).toBeNull());
      game.clients[1].socket.send(encodePrepareQuick(current, true, digest));
      expect((await game.clients[1].refresh()).phase).toBe(0);
      await select(game.clients[1], 1);
      await prepare(game);
      await expire(game);
      await phase(game.clients[0], 2);
      let snapshot;
      do { snapshot = (await game.clients[0].next(ServerMessageType.Presence)).snapshot; } while (!snapshot.match);
      const bytes = snapshot.match!;
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      expect(new TextDecoder().decode(bytes.subarray(0, 4))).toBe("FMLM");
      expect(view.getBigUint64(4)).not.toBe(0n);
      expect(bytes.subarray(12, 44)).toEqual(digest);
      expect(view.getBigUint64(52)).toBe(0n);
      expect([...bytes.subarray(60, 69)]).toEqual([2, 1, 6, 6, 4, 255, 0, 10, 0]);
      expect(bytes[69]).toBe(1); expect(bytes[154]).toBe(1); expect(bytes[239]).toBe(0);
      const epoch = snapshot.matchEpoch;
      await runDurableObjectAlarm(game.room);
      game.clients[0].socket.send(encodeRequestPresence(16384));
      let repeated;
      do { repeated = (await game.clients[0].next(ServerMessageType.Presence)).snapshot; } while (!repeated.match);
      expect(repeated.matchEpoch).toBe(epoch);
      expect(repeated.match).toEqual(bytes);
    } finally { await game.close(); }
  });

  it("cancels when a player disconnects and requires preparation after replacement", async () => {
    const game = await setup();
    try {
      for (const c of game.clients) await select(c, 1);
      await prepare(game);
      await game.clients[1].close();
      await phase(game.clients[0], 0);
      const a = game.assigned[1];
      game.clients[1] = await quickJoin(a.matchRoom, "countdown_b", 1, "quick-test", a.admission);
      await select(game.clients[1], 1);
      expect((await game.clients[1].refresh()).phase).toBe(0);
      await prepare(game);
      await expire(game);
      await phase(game.clients[0], 2);
    } finally { await game.close(); }
  });

  it("assigns shared-PC seats and team rules and preserves the six-stage pool", async () => {
    const game = await setup(2);
    try {
      for (const c of game.clients) await select(c, 2);
      await prepare(game);
      await runInDurableObject(game.room, async (instance, state) => {
        const row = state.storage.sql.exec<{allocation: string}>("SELECT allocation FROM quick_match").one();
        const allocation = JSON.parse(row.allocation) as QuickAllocation;
        // The actual socket attachments remain the source of the tested roster.
        const people = state.getWebSockets().map((s) => s.deserializeAttachment() as import("../src/protocol").ConnectionAttachment)
          .sort((a,b) => a.joinOrder - b.joinOrder);
        for (const [stage, native] of [0x1f,0x20,2,0x1c,3,8].entries()) {
          const bytes = buildQuickMatch({ ...allocation, stage }, people.map(publicParticipant), digest);
          const view = new DataView(bytes.buffer);
          expect(view.getBigUint64(44)).toBe(0x464d535400000000n | BigInt(native + 1));
          expect(bytes[68]).toBe(3);
          expect([bytes[70],bytes[155],bytes[240],bytes[325]]).toEqual([0,1,0,1]);
          expect(bytes[72]).toBe(bytes[157]); expect(bytes[242]).toBe(bytes[327]);
          expect(bytes[72]).not.toBe(bytes[242]);
          expect(bytes[410]).toBe(3);
          const nextPeer = 412 + bytes[411];
          expect(bytes[nextPeer+1]).toBe(12);
        }
      });
      await expire(game);
      await phase(game.clients[0], 2);
    } finally { await game.close(); }
  });
});
