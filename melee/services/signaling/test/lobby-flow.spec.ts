import { env } from "cloudflare:workers";
import { evictDurableObject, runDurableObjectAlarm, runInDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import { quickJoin, QuickClient } from "./quick-client";
import { choices, select } from "./quick-fixture";
import { automaticLobbyStage, emptyLobbyStageHistory, lobbyStageChooser, lobbyReadySignature, LOBBY_STAGES } from "../src/lobby-stage";
import { decodeLobbyRules, encodeLobbyRules, type LobbyRules } from "../src/lobby-rules";
import { encodeRequestLobbyState, encodeSetLobbyRules, encodePickLobbyStage, encodeSetMatch, encodeStartLobbyMatch,
  encodeReportMatch, encodeUpdate, encodeStageCursor, decodeClientMessage,
  decodeServerMessage, encodeStageCursorUpdate, ClientMessageType, ServerMessageType } from "../src/wire";
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
  return { code, room:env.ROOMS.getByName(code), clients, async close() { await Promise.all(clients.map(c=>c.close())); } };
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
  it("encodes signed stage cursor coordinates and rejects malformed cursor packets",()=>{
    const cursor={epoch:7,revision:13,sequence:41,x:-27*256,y:19*256,tile:29,seat:1};
    const packet=encodeStageCursor(cursor);
    expect(decodeClientMessage(packet)).toEqual({type:ClientMessageType.StageCursor,cursor});
    expect(decodeServerMessage(encodeStageCursorUpdate("chooser",cursor))).toEqual({
      type:ServerMessageType.StageCursor,peerId:"chooser",cursor});
    for (const bad of [{x:27*256+1},{y:-19*256-1},{tile:30},{seat:4},{sequence:0}])
      expect(()=>decodeClientMessage(encodeStageCursor({...cursor,...bad}))).toThrow();
    for(let cut=0;cut<packet.byteLength;++cut)expect(()=>decodeClientMessage(packet.slice(0,cut))).toThrow();
  });

  it("relays only the authorized stage chooser's current round without changing readiness",async()=>{
    const game=await setup();
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);await expire(game);
      const room=await presence(game), revision=(await stored(game)).revision;
      const cursor={epoch:room.matchEpoch,revision,sequence:1,x:-2048,y:512,tile:7,seat:0};
      game.clients[0].socket.send(encodeStageCursor(cursor));
      for(const client of game.clients) expect(await client.next(ServerMessageType.StageCursor)).toEqual({
        type:ServerMessageType.StageCursor,peerId:game.clients[0].peerId,cursor});
      // Network jitter can deliver two 20 Hz samples together. Keep the final
      // position instead of dropping it because it arrived soon after another.
      game.clients[0].socket.send(encodeStageCursor({...cursor,sequence:2,x:1024}));
      for(const client of game.clients)expect((await client.next(ServerMessageType.StageCursor)).cursor.x).toBe(1024);
      await evictDurableObject(game.room);
      // Invalid senders, rounds, seats and duplicate packets are ignored.
      game.clients[1].socket.send(encodeStageCursor({...cursor,sequence:2}));
      for(const bad of [{epoch:cursor.epoch+1},{revision:revision+1},{seat:1},{sequence:1}])
        game.clients[0].socket.send(encodeStageCursor({...cursor,sequence:2,...bad}));
      game.clients[0].socket.send(encodeUpdate({name:"cursor barrier"},16384));
      await expect.poll(async()=>(await presence(game)).participants[0].name).toBe("cursor barrier");
      const attachments=await runInDurableObject(game.room,(_instance,ctx)=>ctx.getWebSockets().map(s=>s.deserializeAttachment()));
      expect(attachments.find(p=>p.peerId===game.clients[0].peerId).stageCursor.sequence).toBe(2);
      expect(attachments.find(p=>p.peerId===game.clients[1].peerId).stageCursor).toBeUndefined();
      expect((await stored(game)).revision).toBe(revision);
      expect((await presence(game)).stage).toBe(255);
      await select(game.clients[1],1,false);await state(game.clients[0],0);
      game.clients[0].socket.send(encodeStageCursor({...cursor,sequence:3}));
      game.clients[0].socket.send(encodeUpdate({name:"cancel barrier"},16384));
      await expect.poll(async()=>(await presence(game)).participants[0].name).toBe("cancel barrier");
      expect((await stored(game)).phase).toBe(0);
    } finally {await game.close();}
  });

  it("lets either active player move the shared cursor with stage policy On",async()=>{
    const game=await setup(0);
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);await expire(game);
      const room=await presence(game),revision=(await stored(game)).revision;
      for(const sender of game.clients) {
        const cursor={epoch:room.matchEpoch,revision,sequence:1,x:1536,y:-1024,tile:14,seat:0};
        sender.socket.send(encodeStageCursor(cursor));
        for(const client of game.clients)expect((await client.next(ServerMessageType.StageCursor)).peerId).toBe(sender.peerId);
      }
      expect((await presence(game)).stage).toBe(255);
    }finally{await game.close();}
  });

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
      await select(game.clients[1],1);await state(game.clients[0],1);await expire(game);
      const reopened=await presence(game);
      expect(reopened.matchEpoch).toBeGreaterThan(room.matchEpoch);
      game.clients[0].socket.send(encodePickLobbyStage(room.matchEpoch,0,32,16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/selected/);
      expect((await presence(game)).stage).toBe(255);
      game.clients[0].socket.send(encodePickLobbyStage(reopened.matchEpoch,0,31,16384));
      await expect.poll(async()=>(await presence(game)).stage).toBe(31);
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

  it("starts the two fighters' match when the owner is spectating", async () => {
    const game = await setup(1);
    try {
      const [host, first] = game.clients;
      host.socket.send(encodeUpdate({ activeMask: 0, waitingMask: 0 }, 16384));
      await expect.poll(async () => (await presence(game)).participants[0].activeMask).toBe(0);
      const second = await quickJoin(game.code, `second_${game.code}`);
      game.clients.push(second);
      second.socket.send(encodeRequestLobbyState(16384));
      await state(second, 0);
      await select(first, 1);
      await select(second, 1);
      await state(host, 1);
      await expire(game);
      const ready = await presence(game);
      host.socket.send(encodeStartLobbyMatch(ready.matchEpoch, new Uint8Array(32).fill(0x42)));
      await expect.poll(async () => (await presence(game)).match?.[60]).toBe(2);
      const match = (await presence(game)).match!;
      const peerNames = new TextDecoder().decode(match.subarray(69 + 4 * 85));
      expect(peerNames).toContain(first.peerId);
      expect(peerNames).toContain(second.peerId);
      expect(peerNames).not.toContain(host.peerId);
    } finally { await game.close(); }
  });

  it.each([true,false])("checks the locked roster and waits for both results (agreement %s)",async(agree)=>{
    const game=await setup();
    try {
      for(const client of game.clients)await select(client,1);await state(game.clients[0],1);
      game.clients[0].socket.send(encodeSetMatch(new Uint8Array([1]),16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/proposal/);
      await expire(game);let room=await presence(game);
      game.clients[0].socket.send(encodePickLobbyStage(room.matchEpoch,0,31,16384));
      await expect.poll(async()=>(await presence(game)).stage).toBe(31);room=await presence(game);
      const allocation={mode:1,stage:0,build:"quick-test",players:room.participants.map(p=>({peerId:p.peerId,localPlayers:1,team:0}))} as QuickAllocation;
      const proposal=buildQuickMatch(allocation,room.participants,new Uint8Array(32).fill(0x42));
      proposal[68]=rules.pause<<2|rules.sdPenalty<<4;
      const wrong=proposal.slice();wrong[64]=9;
      game.clients[0].socket.send(encodeSetMatch(wrong,16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/proposal/);
      // A valid proposal from the previous roster cannot override the
      // fighters and input ports that are locked for this round.
      const previousRoster=room.participants.map(p=>({...p, selection:p.selection.slice()}));
      previousRoster[0].selection[25]=8;
      const previous=buildQuickMatch(allocation,previousRoster,new Uint8Array(32).fill(0x42));
      previous[68]=proposal[68];
      game.clients[0].socket.send(encodeSetMatch(previous,16384));
      expect((await game.clients[0].next(ServerMessageType.Error)).message).toMatch(/proposal/);
      expect((await presence(game)).match).toBeNull();
      const digest = new Uint8Array(32).fill(0x42);
      const start = encodeStartLobbyMatch(room.matchEpoch, digest);
      for (const [index, request] of [[1, start], [0, encodeStartLobbyMatch(room.matchEpoch - 1, digest)]] as const) {
        game.clients[index].socket.send(request);
        await game.clients[index].next(ServerMessageType.RoomActionRejected);
        expect((await presence(game)).match).toBeNull();
      }
      game.clients[0].socket.send(start);
      await expect.poll(async()=>(await presence(game)).match?.length).toBe(proposal.length);
      room=await presence(game);
      const accepted = room.match!;
      expect(accepted.slice(0, 4)).toEqual(proposal.slice(0, 4));
      expect(accepted.slice(12)).toEqual(proposal.slice(12));
      expect(accepted.slice(4, 12).some(byte => byte !== 0)).toBe(true);
      game.clients[0].socket.send(start);
      await game.clients[0].next(ServerMessageType.RoomActionRejected);
      expect((await presence(game)).matchEpoch).toBe(room.matchEpoch);
      game.clients[0].socket.send(encodeReportMatch(room.matchEpoch,1,1,16384));
      game.clients[0].socket.send(encodeSetMatch(new Uint8Array(),16384));
      await expect.poll(()=>runInDurableObject(game.room,(_room,ctx)=>
        ctx.getWebSockets().find(s=>s.deserializeAttachment().peerId===game.clients[0].peerId)
          ?.deserializeAttachment().lobbyResult?.epoch)).toBe(room.matchEpoch);
      await evictDurableObject(game.room);
      expect((await presence(game)).match).toEqual(accepted);
      // The first report for a PC remains final, including after hibernation.
      game.clients[0].socket.send(encodeReportMatch(room.matchEpoch,2,0,16384));
      game.clients[1].socket.send(encodeReportMatch(room.matchEpoch,agree?1:2,agree?1:0,16384));
      await expect.poll(async()=>(await presence(game)).match).toBeNull();
      expect((await presence(game)).stagePeerId).toBe(game.clients[agree?1:0].peerId);
      expect((await presence(game)).participants.every(p=>!(p.selection[6]&1))).toBe(true);
      expect((await stored(game)).phase).toBe(0);
      for (const client of game.clients) await select(client, 1);
      await state(game.clients[0], 1);
      await expire(game);
      const next = await presence(game);
      game.clients[agree ? 1 : 0].socket.send(encodePickLobbyStage(next.matchEpoch, 0, 31, 16384));
      await expect.poll(async () => (await presence(game)).stage).toBe(31);
      game.clients[0].socket.send(start);
      await game.clients[0].next(ServerMessageType.RoomActionRejected);
      expect((await presence(game)).match).toBeNull();
      game.clients[0].socket.send(encodeStartLobbyMatch(next.matchEpoch, digest));
      await expect.poll(async () => (await presence(game)).match?.length).toBe(accepted.length);
      const rematch = (await presence(game)).match!;
      expect(rematch.slice(4, 12)).not.toEqual(accepted.slice(4, 12));
      expect(rematch.slice(12)).toEqual(accepted.slice(12));
    }finally{await game.close();}
  });

  it("ignores waiting/spectating selections and rotates Turns in canonical order",()=>{
    const people: Participant[]=[0,1,2].map(i=>({peerId:`player_${i}`,accountId:"",accountHandle:"",name:"Player",build:"test",
      localPlayers:2,activeMask:i===2?0:3,cardSlots:i===2?[255,255,255,255]:[i*2,i*2+1,255,255],
      waitingMask:i===2?3:0,ready:false,selection:choices(2),joinOrder:i,waitingRanks:[0,0,0,0]}));
    const initial=lobbyReadySignature(people,rules);expect(initial).toBeTruthy();
    people[2].selection=new Uint8Array();expect(lobbyReadySignature(people,rules)).toBe(initial);
    expect(lobbyStageChooser(people,{peerId:"player_1",seat:1})).toEqual({peerId:"player_1",seat:1});
    expect(lobbyStageChooser(people,{peerId:"player_2",seat:0})).toEqual({peerId:"player_0",seat:0});
    expect(lobbyStageChooser(people,{peerId:"",seat:255},3,{peerId:"player_0",seat:1})).toEqual({peerId:"player_1",seat:0});
    expect(lobbyStageChooser(people,{peerId:"",seat:255},3,{peerId:"player_1",seat:1})).toEqual({peerId:"player_0",seat:0});
    expect(lobbyReadySignature(people,{...rules,teams:1})).toBeNull();
  });
});
