import { env } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { quickJoin, QuickClient } from "./quick-client";
import { choices, select } from "./quick-fixture";
import { automaticLobbyStage, emptyLobbyStageHistory, lobbyStageChooser, lobbyReadySignature, LOBBY_STAGES } from "../src/lobby-stage";
import { decodeLobbyRules, encodeLobbyRules, type LobbyRules } from "../src/lobby-rules";
import { encodeRequestLobbyState, encodeSetLobbyRules, encodePickLobbyStage, encodeSetMatch,
  encodeReportMatch, encodeUpdate, ServerMessageType } from "../src/wire";
import type { Participant, RoomSnapshot } from "../src/protocol";
import { buildQuickMatch } from "../src/quick-match";
import type { QuickAllocation } from "../src/quick-play";

const rules: LobbyRules = { mode:1,time:6,stocks:4,handicap:0,damage:10,stagePolicy:4,stockTime:6,
  friendlyFire:0,pause:1,score:0,sdPenalty:1,itemFrequency:255,teams:0,itemMask:0n,stageMask:0xffffffff };

async function state(client: QuickClient, phase: number) {
  for (;;) { const { lobby } = await client.next(ServerMessageType.LobbyState); if (lobby.phase === phase) return lobby; }
}
async function setup(policy = 4) {
  const code = crypto.randomUUID().replaceAll("-", "").slice(0,8).toUpperCase();
  const clients = [await quickJoin(code, `host_${code}`), await quickJoin(code, `guest_${code}`)];
  for (const client of clients) {
    client.socket.send(encodeRequestLobbyState(16384)); await state(client,0);
  }
  clients[0].socket.send(encodeSetLobbyRules({ ...rules,stagePolicy:policy },16384));
  expect((await state(clients[1],0)).rules?.stagePolicy).toBe(policy);
  return { room:env.ROOMS.getByName(code), clients, async close() { await Promise.all(clients.map(c=>c.close())); } };
}
async function stored(game: Awaited<ReturnType<typeof setup>>) {
  return runInDurableObject(game.room, (_instance,ctx) => ctx.storage.sql.exec<{ phase:number; deadline:number; signature:string; revision:number }>(
    "SELECT phase,deadline,signature,revision FROM lobby_flow").one());
}
async function expire(game: Awaited<ReturnType<typeof setup>>) {
  await runInDurableObject(game.room, (_instance,ctx) => ctx.storage.sql.exec("UPDATE lobby_flow SET deadline=? WHERE id=1",Date.now()-1));
  await evictDurableObject(game.room); await runDurableObjectAlarm(game.room);
  await state(game.clients[0],2);
}
async function presence(game: Awaited<ReturnType<typeof setup>>): Promise<RoomSnapshot> {
  return runInDurableObject(game.room, (instance) => (instance as unknown as { snapshot(revision:number):RoomSnapshot }).snapshot(0));
}

describe("shared lobby rules and stage flow",()=>{
  it("validates rules and implements the full stock ordered/random catalogs",()=>{
    expect(decodeLobbyRules(encodeLobbyRules(rules))).toEqual(rules);
    const bad=encodeLobbyRules(rules);bad[5]=5;expect(()=>decodeLobbyRules(bad)).toThrow();
    const history=emptyLobbyStageHistory();
    expect(Array.from({length:29},()=>automaticLobbyStage({...rules,stagePolicy:2},history,0))).toEqual(LOBBY_STAGES);
    expect(automaticLobbyStage({...rules,stagePolicy:2},history,0)).toBe(LOBBY_STAGES[0]);
    const random=emptyLobbyStageHistory();
    expect(new Set(Array.from({length:29},()=>automaticLobbyStage({...rules,stagePolicy:1},random,0))).size).toBe(29);
    for(let i=0;i<10;++i) expect(automaticLobbyStage({...rules,stagePolicy:1,stageMask:1<<25},random,0)).toBe(32);
  });

  it("starts once all active fighters are ready, cancels on unready, and survives hibernation",async()=>{
    const game=await setup();
    try {
      game.clients[1].socket.send(encodeSetLobbyRules({...rules,stocks:99},16384));
      expect((await game.clients[1].next(ServerMessageType.Error)).message).toMatch(/host/);
      await select(game.clients[0],1); expect((await stored(game)).phase).toBe(0);
      await select(game.clients[1],1); const countdown=await state(game.clients[0],1);
      expect(countdown.remainingMs).toBeGreaterThan(2900);expect(countdown.remainingMs).toBeLessThanOrEqual(3000);
      const deadline=(await stored(game)).deadline;
      // A name update is not a new readiness event.
      game.clients[1].socket.send(encodeUpdate({name:"Renamed player"},16384));
      await expect.poll(async()=>(await presence(game)).participants[1].name).toBe("Renamed player");
      expect((await stored(game)).deadline).toBe(deadline);
      await select(game.clients[1],1,false);await state(game.clients[0],0);expect((await stored(game)).deadline).toBe(0);
      await select(game.clients[1],1);await state(game.clients[0],1);await expire(game);
      const revision=(await stored(game)).revision;
      await runInDurableObject(game.room,instance=>instance.alarm());
      expect((await stored(game)).revision).toBe(revision);
      const room=await presence(game);expect(room.stage).toBe(255);expect(room.stagePeerId).toBe(game.clients[0].peerId);
      game.clients[1].socket.send(encodePickLobbyStage(room.matchEpoch,0,32,16384));
      expect((await game.clients[1].next(ServerMessageType.Error)).message).toMatch(/selected/);
      game.clients[0].socket.send(encodePickLobbyStage(room.matchEpoch,0,32,16384));
      await expect.poll(async()=>(await presence(game)).stage).toBe(32);
      await select(game.clients[1],1,false);await state(game.clients[0],0);expect((await presence(game)).stage).toBe(255);
    } finally {await game.close();}
  });

  it("restarts a countdown on shared rule changes and lets any fighter choose with On",async()=>{
    const game=await setup(0);
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);
      const before=(await stored(game)).revision;
      game.clients[0].socket.send(encodeSetLobbyRules({...rules,stagePolicy:0,stocks:3},16384));
      for(;;){const s=await state(game.clients[1],1);if(s.rules?.stocks===3)break;}
      expect((await stored(game)).revision).toBeGreaterThan(before);await expire(game);
      const room=await presence(game);expect(room.stagePeerId).toBe("");
      game.clients[1].socket.send(encodePickLobbyStage(room.matchEpoch,0,31,16384));
      await expect.poll(async()=>(await presence(game)).stage).toBe(31);
      game.clients[0].socket.send(encodePickLobbyStage(room.matchEpoch,0,32,16384));
      expect((await presence(game)).stage).toBe(31);
    }finally{await game.close();}
  });

  it.each([1,2])("automatically chooses a stage for policy %i",async(policy)=>{
    const game=await setup(policy);
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);await expire(game);
      expect(LOBBY_STAGES).toContain((await presence(game)).stage);
      if(policy===2)expect((await presence(game)).stage).toBe(4);
    }finally{await game.close();}
  });

  it("checks the agreed stage/rules before commit and clears readiness after a result",async()=>{
    const game=await setup();
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);
      game.clients[0].socket.send(encodeSetMatch(new Uint8Array([1]),16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/proposal/);
      await expire(game);let room=await presence(game);
      game.clients[0].socket.send(encodePickLobbyStage(room.matchEpoch,0,31,16384));
      await expect.poll(async()=>(await presence(game)).stage).toBe(31);room=await presence(game);
      const allocation={mode:1,stage:0,build:"quick-test",players:room.participants.map(p=>({peerId:p.peerId,localPlayers:1}))} as QuickAllocation;
      const proposal=buildQuickMatch(allocation,room.participants,new Uint8Array(32).fill(0x42));
      proposal[68]=rules.pause<<2|rules.sdPenalty<<4;
      const wrong=proposal.slice();wrong[64]=9;
      game.clients[0].socket.send(encodeSetMatch(wrong,16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/proposal/);
      game.clients[0].socket.send(encodeSetMatch(proposal,16384));
      await expect.poll(async()=>(await presence(game)).match?.length).toBe(proposal.length);
      room=await presence(game);
      game.clients[0].socket.send(encodeReportMatch(room.matchEpoch,game.clients[0].peerId,0,true,1,16384));
      await expect.poll(async()=>(await presence(game)).match).toBeNull();
      expect((await presence(game)).participants.every(p=>!(p.selection[6]&1))).toBe(true);
      expect((await stored(game)).phase).toBe(0);
    }finally{await game.close();}
  });

  it("ignores waiting/spectating selections and rotates Turns in canonical order",()=>{
    const people: Participant[]=[0,1,2].map(i=>({peerId:`player_${i}`,accountId:"",accountHandle:"",name:"Player",build:"test",
      localPlayers:2,activeMask:i===2?0:3,waitingMask:i===2?3:0,ready:false,selection:choices(2),joinOrder:i,waitingRanks:[0,0,0,0]}));
    const initial=lobbyReadySignature(people,rules);expect(initial).toBeTruthy();
    people[2].selection=new Uint8Array();expect(lobbyReadySignature(people,rules)).toBe(initial);
    expect(lobbyStageChooser(people,{peerId:"player_1",seat:1})).toEqual({peerId:"player_1",seat:1});
    expect(lobbyStageChooser(people,{peerId:"player_2",seat:0})).toEqual({peerId:"player_0",seat:0});
    expect(lobbyStageChooser(people,{peerId:"",seat:255},3,{peerId:"player_0",seat:1})).toEqual({peerId:"player_1",seat:0});
    expect(lobbyStageChooser(people,{peerId:"",seat:255},3,{peerId:"player_1",seat:1})).toEqual({peerId:"player_0",seat:0});
    expect(lobbyReadySignature(people,{...rules,teams:1})).toBeNull();
  });
});
