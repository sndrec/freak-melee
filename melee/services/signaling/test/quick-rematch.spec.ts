import { runInDurableObject, evictDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { phase, setup, select, prepare, expire, revision } from "./quick-fixture";
import { encodeReportQuick, encodeVoteQuickStage, encodeRequestPresence, encodeStageCursor, ServerMessageType } from "../src/wire";

async function launch(game: Awaited<ReturnType<typeof setup>>) {
  const before = await revision(game);
  for (let i=0; i<game.clients.length; ++i) await select(game.clients[i], game.players[i], true, before);
  await prepare(game); await expire(game);
  await phase(game.clients[0], 2);
  return runInDurableObject(game.room, async (_i, state) =>
    state.storage.sql.exec<{epoch: number}>("SELECT epoch FROM room_match").one().epoch);
}

describe("Quick Play round results and next stage", () => {
  it("requires every PC's result, gives a non-host loser the pick, and starts a fresh match", async () => {
    const game = await setup();
    try {
      const epoch = await launch(game);
      game.clients[0].socket.send(encodeReportQuick(epoch, game.assigned[0].team));
      await phase(game.clients[0], 4);
      await runInDurableObject(game.room, async (_i, state) =>
        expect(state.storage.sql.exec<{payload: ArrayBuffer}>("SELECT payload FROM room_match").one().payload).not.toBeNull());
      game.clients[1].socket.send(encodeReportQuick(epoch, game.assigned[0].team));
      const choosing = await phase(game.clients[0], 3);
      expect(choosing.completedEpoch).toBe(epoch);
      expect(choosing.chooser).toBe(game.assigned[1].team);
      const cursor={epoch,revision:0,sequence:1,x:-2000,y:1500,tile:3,seat:0};
      // The winner cannot move the loser's cursor. The losing player's update
      // is relayed to every PC, including before anyone confirms a stage.
      game.clients[0].socket.send(encodeStageCursor({...cursor,sequence:99}));
      game.clients[1].socket.send(encodeStageCursor(cursor));
      for (const client of game.clients) expect(await client.next(ServerMessageType.StageCursor)).toEqual({
        type:ServerMessageType.StageCursor,peerId:game.clients[1].peerId,cursor});
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 5));
      await game.clients[0].next(ServerMessageType.QuickRejected);
      game.clients[1].socket.send(encodeVoteQuickStage(epoch - 1, 0, 5));
      expect((await game.clients[1].refresh()).phase).toBe(3);
      await evictDurableObject(game.room);
      game.clients[1].socket.send(encodeVoteQuickStage(epoch, 0, 5));
      const chosen = await phase(game.clients[0], 0);
      expect(chosen.stage).toBe(5);
      // Reports for the retired round cannot clear or alter its rematch.
      game.clients[0].socket.send(encodeReportQuick(epoch, 255));
      const next = await launch(game);
      expect(next).toBeGreaterThan(epoch);
      game.clients[1].socket.send(encodeReportQuick(epoch, game.assigned[1].team));
      game.clients[1].socket.send(encodeRequestPresence(16384));
      await runInDurableObject(game.room, async (_i, state) => {
        const match = state.storage.sql.exec<{epoch: number; payload: ArrayBuffer}>("SELECT epoch, payload FROM room_match").one();
        expect(match.epoch).toBe(next); expect(match.payload).not.toBeNull();
      });
    } finally { await game.close(); }
  });

  it("lets one shared-PC teammate pick, change or withdraw, and the other confirm", async () => {
    const game = await setup(2);
    try {
      const epoch = await launch(game), losing = game.assigned[0].team;
      for (const c of game.clients) c.socket.send(encodeReportQuick(epoch, 1-losing));
      await phase(game.clients[0], 3);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 0));
      let current = await phase(game.clients[0], 3, (state) => state.votes[0] === 0);
      expect(current.phase).toBe(3); expect(current.votes[0]).toBe(0);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 1, 1));
      await game.clients[0].next(ServerMessageType.QuickRejected);
      current = await game.clients[0].refresh();
      expect(current.phase).toBe(3); expect(current.votes.slice(0,2)).toEqual([0,255]);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 2));
      current = await phase(game.clients[0], 3, (state) => state.votes[0] === 2);
      expect(current.votes.slice(0,2)).toEqual([2,255]);
      // A delayed confirmation of the previous selection cannot start play.
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 1, 0));
      await game.clients[0].next(ServerMessageType.QuickRejected);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 255));
      current = await phase(game.clients[0], 3, (state) => state.votes[0] === 255);
      expect(current.votes.slice(0,2)).toEqual([255,255]);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 1, 1));
      await phase(game.clients[0], 3, (state) => state.votes[1] === 1);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 1));
      current = await phase(game.clients[0], 0);
      expect(current.stage).toBe(1);
    } finally { await game.close(); }
  });

  it("keeps conflicting results pending and resets an agreed no-contest without granting a stage pick", async () => {
    const game = await setup();
    try {
      const epoch = await launch(game);
      game.clients[0].socket.send(encodeReportQuick(epoch, 0));
      game.clients[1].socket.send(encodeReportQuick(epoch, 1));
      await game.clients[0].next(ServerMessageType.QuickRejected);
      expect((await game.clients[0].refresh()).phase).toBe(4);
      game.clients[0].socket.send(encodeReportQuick(epoch, 1));
      await game.clients[0].next(ServerMessageType.QuickRejected);
    } finally { await game.close(); }
    const aborted = await setup();
    try {
      const epoch = await launch(aborted), stage = aborted.assigned[0].stage;
      for (const c of aborted.clients) c.socket.send(encodeReportQuick(epoch, 255));
      const result = await phase(aborted.clients[0], 0);
      expect(result.chooser).toBe(255); expect(result.stage).toBe(stage);
      expect(await launch(aborted)).toBeGreaterThan(epoch);
    } finally { await aborted.close(); }
  });

  it("requires separate-PC teammates to report and agree independently", async () => {
    const game = await setup(2, [[1,1],[2]]);
    try {
      const epoch = await launch(game), winner = game.assigned[2].team;
      game.clients[0].socket.send(encodeReportQuick(epoch, winner));
      game.clients[1].socket.send(encodeReportQuick(epoch, winner));
      await phase(game.clients[0], 4);
      game.clients[2].socket.send(encodeReportQuick(epoch, winner));
      await phase(game.clients[0], 3);
      expect(game.assigned[0].team).toBe(game.assigned[1].team);
      game.clients[0].socket.send(encodeVoteQuickStage(epoch, 0, 2));
      await phase(game.clients[0], 3, (state) => state.votes[0] === 2);
      game.clients[1].socket.send(encodeVoteQuickStage(epoch, 1, 2));
      await game.clients[1].next(ServerMessageType.QuickRejected); // A PC cannot vote for a seat it does not own.
      game.clients[1].socket.send(encodeVoteQuickStage(epoch, 0, 2));
      const chosen = await phase(game.clients[0], 0);
      expect(chosen.stage).toBe(2);
      expect(await launch(game)).toBeGreaterThan(epoch);
    } finally { await game.close(); }
  });

  it("preserves a running P2P proposal on signaling loss and retires the group after its end report", async () => {
    const game = await setup();
    try {
      const epoch = await launch(game);
      await game.clients[1].close();
      await runInDurableObject(game.room, async (_i, state) => {
        const match = state.storage.sql.exec<{epoch: number; payload: ArrayBuffer}>("SELECT epoch, payload FROM room_match").one();
        expect(match.epoch).toBe(epoch); expect(match.payload).not.toBeNull();
      });
      game.clients[0].socket.send(encodeReportQuick(epoch, 255));
      expect((await game.clients[0].state(3)).state).toBe(3);
    } finally { await game.close(); }
  });
});
