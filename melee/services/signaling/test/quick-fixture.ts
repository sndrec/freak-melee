import { env } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { quickJoin, QuickClient } from "./quick-client";
import { encodePrepareQuick, encodeRequestPresence, encodeUpdate, ServerMessageType } from "../src/wire";

export const digest = new Uint8Array(32).fill(0x42);
export function choices(players: number, confirmed = true): Uint8Array {
  const bytes = new Uint8Array(7 + 50 * players);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, 0x464d534c); bytes[4] = 1; bytes[5] = players; bytes[6] = confirmed ? 1 : 0;
  for (let i = 0; i < players; ++i) {
    bytes[7 + i * 50] = 3;
    bytes.fill(i + 1, 25 + i * 50, 57 + i * 50);
  }
  return bytes;
}
export async function phase(client: QuickClient, expected: number,
  matches?: (state: import("../src/quick-play").QuickState) => boolean) {
  for (;;) {
    const state = await client.state(2);
    if (state.phase === expected && (!matches || matches(state))) return state;
  }
}
export async function setup(mode: 1 | 2 = 1, partySeats: number[][] = [[mode],[mode]]) {
  const parties: QuickClient[] = [], leaders: QuickClient[] = [], players: number[] = [];
  for (const seats of partySeats) {
    const code = crypto.randomUUID().replaceAll("-", "").slice(0,8).toUpperCase();
    const first = parties.length;
    for (const count of seats) {
      const peer = `quick_${code}_${parties.length}`;
      parties.push(await quickJoin(code, peer, count)); players.push(count);
    }
    leaders.push(parties[first]);
    await parties[first].configure(mode);
  }
  for (const leader of leaders) await leader.search();
  const assigned = [];
  for (const party of parties) assigned.push(await party.state(2));
  const room = env.ROOMS.getByName(assigned[0].matchRoom);
  const clients: QuickClient[] = [];
  for (let i=0; i<parties.length; ++i)
    clients.push(await quickJoin(assigned[i].matchRoom, parties[i].peerId, players[i], "quick-test", assigned[i].admission));
  return { room, clients, assigned, mode, players, async close() { await Promise.all([...clients, ...parties].map((c) => c.close())); } };
}
export async function select(client: QuickClient, players: number, confirmed = true, minimumRevision = 0) {
  const selection = choices(players, confirmed);
  const before = Math.max(minimumRevision, client.presenceRevision);
  client.socket.send(encodeUpdate({ selection }, 16384));
  client.socket.send(encodeRequestPresence(16384));
  for (;;) {
    const presence = (await client.next(ServerMessageType.Presence)).snapshot;
    if (presence.revision > before && presence.participants.some((p) => p.peerId === client.peerId &&
        p.selection.length === selection.length && p.selection[6] === selection[6])) return;
  }
}
export async function revision(game: Awaited<ReturnType<typeof setup>>) {
  return runInDurableObject(game.room, async (_instance, state) =>
    state.storage.sql.exec<{revision: number}>("SELECT revision FROM room_counter").one().revision);
}
export async function prepare(game: Awaited<ReturnType<typeof setup>>) {
  const current = await revision(game);
  for (const client of game.clients) client.socket.send(encodePrepareQuick(current, true, digest));
  return phase(game.clients[0], 1);
}
export async function expire(game: Awaited<ReturnType<typeof setup>>) {
  await runInDurableObject(game.room, async (_instance, state) => {
    state.storage.sql.exec("UPDATE quick_flow SET deadline=? WHERE id=1", Date.now() - 1);
  });
  await evictDurableObject(game.room);
  await runDurableObjectAlarm(game.room);
}

