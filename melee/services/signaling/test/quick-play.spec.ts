import { env } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { quickJoin, quickConnect } from "./quick-client";
import { allocateQuickMatch, chooseQuickTickets, quickBucket, QUICK_RULES, QUICK_STAGES,
  type QuickOffer, type QuickTicket, type QuickAllocation } from "../src/quick-play";
import { ServerMessageType, encodeSetMatch, encodeQueueQuick } from "../src/wire";

function ticket(index: number, seats: number): QuickTicket {
  return { partyId: index.toString(16).padStart(64, "0"), roomCode: `PARTY${index}X`, generation: 1,
    mode: 2, build: "quick-test", members: [{peerId: `player_${index}`, name: `Player ${index}`,
      localPlayers: seats, connectionId: crypto.randomUUID()}], created: index, expires: Date.now()+30000 };
}
const room = (name: string) => env.ROOMS.getByName(name);
const queue = (mode: 1|2, build = "quick-test") => env.QUICK_PLAY.getByName(quickBucket(mode, build));
async function offer(name: string): Promise<QuickOffer> {
  return runInDurableObject(room(name), async (_instance, state) => {
    const row = state.storage.sql.exec<{roster: string}>("SELECT roster FROM quick_party WHERE id=1").one();
    return JSON.parse(row.roster) as QuickOffer;
  });
}

describe("Quick Play assembly", () => {
  it("preserves pairs for every doubles combination and includes all six stages", () => {
    expect(QUICK_STAGES).toEqual(["battlefield", "final_destination", "fountain_of_dreams", "dream_land", "pokemon_stadium", "yoshis_story"]);
    for (const sizes of [[2,2], [2,1,1], [1,2,1], [1,1,1,1]]) {
      const tickets = sizes.map((size, index) => ticket(index+1, size));
      const selected = chooseQuickTickets(tickets, 2);
      expect(selected).toHaveLength(tickets.length);
      const result = allocateQuickMatch(selected);
      expect(result.rules).toEqual(QUICK_RULES);
      expect(result.players.filter((p) => p.team === 0).reduce((sum,p) => sum+p.localPlayers, 0)).toBe(2);
      expect(result.players.filter((p) => p.team === 1).reduce((sum,p) => sum+p.localPlayers, 0)).toBe(2);
      expect(result.stage).toBeGreaterThanOrEqual(0);
      expect(result.stage).toBeLessThan(6);
    }
    expect(chooseQuickTickets([ticket(1,1), ticket(2,2), ticket(3,2)], 2).map((p) => p.created)).toEqual([2,3]);
    expect(chooseQuickTickets([ticket(1,2), ticket(2,1)], 2)).toEqual([]);
  });

  it("allocates two real solo parties once and requires private admission tokens", async () => {
    const a = await quickJoin("QSINGLEA", "single_a");
    const b = await quickJoin("QSINGLEB", "single_b");
    try {
      await a.configure(1); await b.configure(1);
      await a.search(); await a.state(1);
      await a.search(); await a.state(1);
      await b.search();
      const first = await a.state(2), second = await b.state(2);
      expect(first.matchRoom).toBe(second.matchRoom);
      expect(first.admission).not.toBe(second.admission);
      expect(first.team).not.toBe(second.team);
      expect((await quickConnect(first.matchRoom, "single_a", 1, "quick-test", second.admission)).status).toBe(403);
      expect((await quickConnect(first.matchRoom, "outsider_x", 1, "quick-test", first.admission)).status).toBe(403);
      expect((await quickConnect(first.matchRoom, "single_a", 1, "different", first.admission)).status).toBe(403);
      const joined = await quickJoin(first.matchRoom, "single_a", 1, "quick-test", first.admission);
      try {
        expect((await joined.state(2)).matchRoom).toBe(first.matchRoom);
        joined.socket.send(encodeSetMatch(new Uint8Array([1,2,3]), 16384));
        await joined.next(ServerMessageType.QuickRejected);
      } finally { await joined.close(); }
      await a.search();
      expect((await a.state(2)).matchRoom).toBe(first.matchRoom);
    } finally { await a.close(); await b.close(); }
  });

  it("combines a shared-PC pair with two solo players", async () => {
    const pair = await quickJoin("QPAIRAAA", "local_pair", 2);
    const a = await quickJoin("QPAIRBBB", "solo_aaa");
    const b = await quickJoin("QPAIRCCC", "solo_bbb");
    try {
      await pair.configure(2); await a.configure(2); await b.configure(2);
      await pair.search(); await a.search(); await b.search();
      const p = await pair.state(2), x = await a.state(2), y = await b.state(2);
      expect(p.matchRoom).toBe(x.matchRoom); expect(x.matchRoom).toBe(y.matchRoom);
      expect(x.team).toBe(y.team); expect(p.team).not.toBe(x.team);
      const game = await quickJoin(p.matchRoom, "local_pair", 2, "quick-test", p.admission);
      await game.close();
    } finally { await pair.close(); await a.close(); await b.close(); }
  });

  it("keeps teammates on separate PCs together and permits only the leader to queue", async () => {
    const leader = await quickJoin("QREMOTE1", "leader_pc");
    await leader.configure(2);
    const mate = await quickJoin("QREMOTE1", "teammate_pc");
    const other = await quickJoin("QREMOTE2", "other_pair", 2);
    try {
      await mate.state(0); await other.configure(2);
      await mate.search(); await mate.next(ServerMessageType.QuickRejected);
      await leader.search(); await leader.state(1);
      expect((await quickConnect("QREMOTE1", "third_pc")).status).toBe(409);
      await other.search();
      const a = await leader.state(2), b = await mate.state(2), c = await other.state(2);
      expect(a.matchRoom).toBe(b.matchRoom); expect(b.matchRoom).toBe(c.matchRoom);
      expect(a.team).toBe(b.team); expect(a.team).not.toBe(c.team);
      expect(a.admission).not.toBe(b.admission);
    } finally { await leader.close(); await mate.close(); await other.close(); }
  });

  it("fences cancelled searches against delayed enqueue and permits a new search", async () => {
    const a = await quickJoin("QCANCELA", "cancel_a");
    const b = await quickJoin("QCANCELB", "cancel_b");
    try {
      await a.configure(1); await b.configure(1);
      await a.search(); await a.state(1);
      const stale = await offer("QCANCELA");
      await a.cancel();
      expect(await queue(1).enqueue(stale)).toBe(false);
      await b.search(); await b.state(1);
      await a.search();
      expect((await a.state(2)).matchRoom).toBe((await b.state(2)).matchRoom);
    } finally { await a.close(); await b.close(); }
  });

  it("expires a lease after hibernation instead of matching an abandoned search", async () => {
    const a = await quickJoin("QEXPIREA", "expire_a");
    try {
      await a.configure(1); await a.search(); await a.state(1);
      const stale = await offer("QEXPIREA");
      await runInDurableObject(queue(1), async (_instance, state) => {
        state.storage.sql.exec("UPDATE queue_entries SET expires=?", Date.now()-1);
      });
      await evictDurableObject(queue(1));
      await runDurableObjectAlarm(queue(1));
      await a.state(0);
      expect(await queue(1).enqueue(stale)).toBe(false);
    } finally { await a.close(); }
  });

  it("restores queued parties and memberships after hibernation", async () => {
    const a = await quickJoin("QHIBERNA", "hibernate_a");
    const b = await quickJoin("QHIBERNB", "hibernate_b");
    try {
      await a.configure(1); await b.configure(1); await a.search(); await a.state(1);
      await evictDurableObject(room("QHIBERNA")); await evictDurableObject(queue(1));
      await b.search();
      expect((await a.state(2)).matchRoom).toBe((await b.state(2)).matchRoom);
    } finally { await a.close(); await b.close(); }
  });

  it("separates builds and prevents a peer from searching in two parties", async () => {
    const a = await quickJoin("QBUILDAA", "same_peer");
    const duplicate = await quickJoin("QDUPLIAA", "same_peer");
    const other = await quickJoin("QBUILDBB", "other_peer", 1, "other-build");
    try {
      await a.configure(1); await duplicate.configure(1); await other.configure(1);
      await a.search(); await a.state(1);
      await duplicate.search(); await duplicate.next(ServerMessageType.QuickRejected); await duplicate.state(0);
      await other.search(); await other.state(1);
      expect(await runInDurableObject(queue(1), async (_instance, state) =>
        state.storage.sql.exec("SELECT * FROM queue_entries").toArray().length)).toBe(1);
      expect(await runInDurableObject(queue(1, "other-build"), async (_instance, state) =>
        state.storage.sql.exec("SELECT * FROM queue_entries").toArray().length)).toBe(1);
      await a.cancel(); await duplicate.search(); await duplicate.state(1);
    } finally { await a.close(); await duplicate.close(); await other.close(); }
  });

  it("rejects malformed commands and incompatible party seats without losing signaling", async () => {
    const a = await quickJoin("QBADPARTY", "bad_pair", 2);
    try {
      a.socket.send(new Uint8Array([70,77,83,49,5,11,0,1,2]));
      await a.next(ServerMessageType.QuickRejected);
      a.socket.send(new Uint8Array([70,77,83,49,5,10,0,1,1]));
      await a.next(ServerMessageType.QuickRejected);
      await a.configure(2);
      expect((await a.refresh()).mode).toBe(2);
    } finally { await a.close(); }
  });

  it("replaces a queued connection without letting its stale close cancel the new search", async () => {
    const old = await quickJoin("QREPLACEA", "replace_a");
    const b = await quickJoin("QREPLACEB", "replace_b");
    let replacement;
    try {
      await old.configure(1); await b.configure(1);
      await old.search(); await old.state(1);
      // The public peer ID alone cannot replace an active guest connection.
      replacement = await quickJoin("QREPLACEA", "replace_a", 1, "quick-test", undefined, old.connectionId);
      await replacement.state(0); await replacement.search(); await replacement.state(1);
      await b.search();
      expect((await replacement.state(2)).matchRoom).toBe((await b.state(2)).matchRoom);
    } finally { await old.close(); await replacement?.close(); await b.close(); }
  });

  it("lets a handed-off party return, cancel its assignment and search again", async () => {
    const a = await quickJoin("QRETURNA", "return_a");
    const b = await quickJoin("QRETURNB", "return_b");
    let returned;
    try {
      await a.configure(1); await b.configure(1);
      await a.search(); await a.state(1); await b.search();
      const first = await a.state(2); await b.state(2);
      await a.close();
      returned = await quickJoin("QRETURNA", "return_a");
      await returned.state(2); await returned.cancel();
      await b.cancel();
      await returned.search(); await returned.state(1); await b.search();
      const next = await returned.state(2);
      expect(next.matchRoom).not.toBe(first.matchRoom);
      expect(next.matchRoom).toBe((await b.state(2)).matchRoom);
      expect((await quickConnect(first.matchRoom, "return_a", 1, "quick-test", first.admission)).status).toBe(403);
    } finally { await a.close(); await returned?.close(); await b.close(); }
  });

  it("recovers a committed allocation after partial handoff and service eviction", async () => {
    const a = await quickJoin("QRECOVERA", "recover_a");
    const b = await quickJoin("QRECOVERB", "recover_b");
    try {
      await a.configure(1); await b.configure(1);
      await a.search(); await a.state(1);
      const first = await offer("QRECOVERA");
      // Construct the persisted crash point after the second party saves its
      // search and before its coordinator enqueue call returns.
      const second = await runInDurableObject(room("QRECOVERB"), async (_instance, state) => {
        const socket = state.getWebSockets()[0];
        const p = socket.deserializeAttachment();
        const value: QuickOffer = { partyId: state.id.toString(), roomCode: "QRECOVERB", generation: 2,
          mode: 1, build: "quick-test", members: [{peerId: p.peerId, name: p.name,
            localPlayers: p.localPlayers, connectionId: p.connectionId}] };
        state.storage.sql.exec("UPDATE quick_party SET generation=2, state=1, roster=? WHERE id=1", JSON.stringify(value));
        return value;
      });
      const now = Date.now();
      const allocation = allocateQuickMatch([first, second].map((party) => ({...party, created: now, expires: now+30000})));
      expect(await room("QRECOVERA").prepareQuickAllocation(first.generation, allocation)).toBe(true);
      expect(await room("QRECOVERB").prepareQuickAllocation(second.generation, allocation)).toBe(true);
      expect(await room(allocation.roomCode).configureQuickMatch(allocation)).toBe(true);
      await runInDurableObject(queue(1), async (_instance, state) => {
        state.storage.sql.exec("DELETE FROM queue_entries");
        state.storage.sql.exec("INSERT INTO queue_jobs VALUES (?, ?, 0, 1)", allocation.id, JSON.stringify(allocation));
        for (const party of allocation.parties) {
          state.storage.sql.exec("INSERT INTO queue_job_members VALUES (?, ?, ?)", party.partyId, party.generation, allocation.id);
          state.storage.sql.exec("INSERT OR REPLACE INTO queue_fences VALUES (?, ?, 0)", party.partyId, party.generation);
          for (const member of party.members)
            state.storage.sql.exec("INSERT OR REPLACE INTO queue_peers VALUES (?, ?, ?)", member.peerId, party.partyId, party.generation);
        }
      });
      expect(await room("QRECOVERA").activateQuickAllocation(first.generation, allocation.id)).toBe(true);
      const assignment = await a.state(2);
      await a.close();
      await evictDurableObject(room("QRECOVERB"));
      await evictDurableObject(queue(1));
      await runDurableObjectAlarm(queue(1));
      expect((await b.state(2)).matchRoom).toBe(assignment.matchRoom);
      const game = await quickJoin(assignment.matchRoom, "recover_a", 1, "quick-test", assignment.admission);
      await game.close();
    } finally { await a.close(); await b.close(); }
  });
});
