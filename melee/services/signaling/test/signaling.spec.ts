import { guestHeaders } from "./guest";
import { env, exports } from "cloudflare:workers";
import { evictDurableObject } from "cloudflare:test";
import { describe, expect, it } from "vitest";

import { MAX_MESSAGE_BYTES, RotationPolicy } from "../src/protocol";
import { type TurnBindings, turnCredentialsResponse } from "../src/turn";
import {
  ServerMessageType,
  type ServerMessage,
  decodeServerMessage,
  encodeRequestPresence,
  encodeAssignSeats,
  encodeSetMatch,
  encodeSetListing,
  encodeSetRotation,
  encodeReportMatch,
  encodePickLobbyStage,
  encodeSignalRequest,
  encodeUpdate,
} from "../src/wire";

const worker = exports.default as unknown as Fetcher;

interface DecodedTurnConfiguration {
  hasTurn: boolean;
  ttlSeconds: number;
  stunHost: string;
  stunPort: number;
  turnHost: string;
  turnPort: number;
  username: string;
  password: string;
}

async function decodeTurnConfiguration(response: Response): Promise<DecodedTurnConfiguration> {
  const bytes = new Uint8Array(await response.arrayBuffer());
  expect(bytes.byteLength).toBeGreaterThanOrEqual(20);
  expect(new TextDecoder().decode(bytes.subarray(0, 4))).toBe("FMT1");
  expect(bytes[4]).toBe(1);
  const lengths = [bytes[6], bytes[7], bytes[8], bytes[9]];
  expect(bytes.byteLength).toBe(20 + lengths.reduce((sum, length) => sum + length, 0));
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const decoder = new TextDecoder();
  let offset = 20;
  const fields = lengths.map((length) => {
    const value = decoder.decode(bytes.subarray(offset, offset + length));
    offset += length;
    return value;
  });
  return {
    hasTurn: (bytes[5] & 1) !== 0,
    ttlSeconds: view.getUint32(12),
    stunHost: fields[0],
    stunPort: view.getUint16(16),
    turnHost: fields[1],
    turnPort: view.getUint16(18),
    username: fields[2],
    password: fields[3],
  };
}

function nextMessage(socket: WebSocket): Promise<ServerMessage> {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error("WebSocket message timed out")), 2_000);
    socket.addEventListener(
      "message",
      (event) => {
        clearTimeout(timeout);
        if (typeof event.data === "string") {
          reject(new Error("Expected a binary signaling message"));
          return;
        }
        void (async () => {
          try {
            const data = event.data as unknown;
            let buffer: ArrayBuffer;
            if (ArrayBuffer.isView(data)) {
              buffer = new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice().buffer;
            } else if (
              typeof data === "object" &&
              data !== null &&
              "arrayBuffer" in data &&
              typeof data.arrayBuffer === "function"
            ) {
              buffer = await data.arrayBuffer() as ArrayBuffer;
            } else {
              buffer = data as ArrayBuffer;
            }
            resolve(decodeServerMessage(buffer));
          } catch (error) {
            reject(error);
          }
        })();
      },
      { once: true },
    );
  });
}

async function connect(
  room: string,
  peer: string,
  name: string,
  role: "active" | "waiting" | "spectator" | "auto",
  players: number,
  password?: string,
): Promise<WebSocket> {
  const url = new URL(`https://signal.test/v1/rooms/${room}/connect`);
  url.searchParams.set("peer", peer);
  url.searchParams.set("name", name);
  url.searchParams.set("build", "native-test-build");
  url.searchParams.set("players", String(players));
  if (role !== "auto") {
    url.searchParams.set("active", String(role === "active" ? (1 << players) - 1 : 0));
    url.searchParams.set("waiting", String(role === "waiting" ? (1 << players) - 1 : 0));
  }
  const response = await worker.fetch(url.toString(), {
    headers: { Upgrade: "websocket", ...guestHeaders(peer),
      ...(password ? { "X-Freak-Lobby-Password": password } : {}) },
  });
  expect(response.status).toBe(101);
  expect(response.webSocket).not.toBeNull();
  const socket = response.webSocket!;
  socket.accept();
  return socket;
}

describe("signaling worker", () => {
  it("enforces a lobby's configured fighter limit when players join or switch seats", async () => {
    const owner = await connect("CAPACITY", "cap_owner", "Owner", "active", 1);
    await nextMessage(owner);
    await nextMessage(owner);
    const changed = nextMessage(owner);
    owner.send(encodeSetListing("Two fighters", true, 2));
    expect(await changed).toMatchObject({ type: ServerMessageType.Listing, capacity: 2 });
    await nextMessage(owner);

    const joined = nextMessage(owner);
    const guest = await connect("CAPACITY", "cap_guest", "Guest", "auto", 2);
    await nextMessage(guest);
    const presence = await joined;
    expect(presence.type).toBe(ServerMessageType.Presence);
    if (presence.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(presence.snapshot.participants.find(p => p.peerId === "cap_guest"))
      .toMatchObject({ activeMask: 1, waitingMask: 2 });

    const rejected = nextMessage(owner);
    owner.send(encodeAssignSeats("cap_guest", 3, 0, MAX_MESSAGE_BYTES));
    expect(await rejected).toMatchObject({ type: ServerMessageType.Error,
      message: "All fighting seats are taken" });
    owner.close(1000, "test complete");
    guest.close(1000, "test complete");
  });

  it("requires a lobby password for new participants and keeps it out of the listing", async () => {
    const owner = await connect("PASSWORD", "lock_owner", "Owner", "active", 1);
    await nextMessage(owner);
    await nextMessage(owner);
    const changed = nextMessage(owner);
    owner.send(encodeSetListing("Locked lobby", true, 4, "Test-Code-83"));
    expect(await changed).toEqual({ type: ServerMessageType.Listing, title: "Locked lobby",
      capacity: 4, listed: true, locked: true });
    const guestURL = "https://signal.test/v1/rooms/PASSWORD/connect?peer=lock_guest&name=Guest&build=native-test-build&players=1";
    const headers = { Upgrade: "websocket", ...guestHeaders("lock_guest") };
    expect((await worker.fetch(guestURL, { headers })).status).toBe(423);
    expect((await worker.fetch(guestURL, { headers: { ...headers, "X-Freak-Lobby-Password": "Wrong" } })).status).toBe(423);

    const joined = nextMessage(owner);
    const guest = await connect("PASSWORD", "lock_guest", "Guest", "auto", 1, "Test-Code-83");
    await joined;
    const unlocked = nextMessage(owner);
    owner.send(encodeSetListing("Locked lobby", true, 4, ""));
    expect(await unlocked).toMatchObject({ type: ServerMessageType.Listing, locked: false });
    const unlockedGuest = await connect("PASSWORD", "lock_third", "Third", "auto", 1);
    unlockedGuest.close(1000, "test complete");
    guest.close(1000, "test complete");
    owner.close(1000, "test complete");
  });

  it("hands lobby stage choice to the previous loser, with active-seat fallback", async () => {
    const sockets: WebSocket[] = [];
    async function join(peer: string, players: number) {
      const pending = sockets.map(nextMessage);
      const socket = await connect("STAGE77", peer, peer,
        players ? "active" : "spectator", players);
      await nextMessage(socket); await nextMessage(socket); await Promise.all(pending);
      sockets.push(socket);
      return socket;
    }
    async function broadcast(sender: WebSocket, bytes: ArrayBuffer) {
      const pending = sockets.map(nextMessage);
      sender.send(bytes);
      const messages = await Promise.all(pending);
      if (messages[0].type !== ServerMessageType.Presence) throw new Error("presence expected");
      return messages[0].snapshot;
    }
    async function rejected(sender: WebSocket, bytes: ArrayBuffer) {
      const pending = nextMessage(sender); sender.send(bytes);
      expect(await pending).toMatchObject({ type: ServerMessageType.Error });
    }
    const host = await join("stage_host", 0);
    const first = await join("stage_first", 2);
    const second = await join("stage_second", 2);
    const start = new Uint8Array([0x46,0x4d,0x53,0x4c,1,0,2]);
    let room = await broadcast(host, encodeUpdate({ selection: start }, MAX_MESSAGE_BYTES));
    expect([room.stagePeerId,room.stageSeat,room.stage]).toEqual(["stage_first",0,255]);
    await rejected(host, encodePickLobbyStage(room.matchEpoch,0,32,MAX_MESSAGE_BYTES));
    await rejected(first, encodePickLobbyStage(room.matchEpoch,1,32,MAX_MESSAGE_BYTES));
    for (const invalidStage of [0,21,26,33])
      await rejected(first, encodePickLobbyStage(room.matchEpoch,0,invalidStage,MAX_MESSAGE_BYTES));
    room = await broadcast(first, encodePickLobbyStage(room.matchEpoch,0,32,MAX_MESSAGE_BYTES));
    expect(room.stage).toBe(32);
    room = await broadcast(host, encodeSetMatch(new Uint8Array([1]), MAX_MESSAGE_BYTES));
    const completedEpoch = room.matchEpoch;
    room = await broadcast(second, encodeReportMatch(completedEpoch,"",255,false,3,MAX_MESSAGE_BYTES));
    expect([room.stagePeerId,room.stageSeat,room.stage]).toEqual(["stage_second",1,255]);
    await evictDurableObject(env.ROOMS.getByName("STAGE77"));
    const persisted = nextMessage(host); host.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    expect(await persisted).toMatchObject({ snapshot: { stagePeerId:"stage_second", stageSeat:1 } });
    // The second player on a shared PC owns the next pick, not its first pad.
    await rejected(second, encodePickLobbyStage(room.matchEpoch,0,31,MAX_MESSAGE_BYTES));
    room = await broadcast(second, encodePickLobbyStage(room.matchEpoch,1,31,MAX_MESSAGE_BYTES));
    expect(room.stage).toBe(31);
    const obsoleteEpoch = room.matchEpoch;
    room = await broadcast(second, encodeUpdate({ activeMask:1, waitingMask:0 },MAX_MESSAGE_BYTES));
    expect([room.stagePeerId,room.stageSeat,room.stage]).toEqual(["stage_first",0,255]);
    await rejected(first, encodePickLobbyStage(obsoleteEpoch,0,8,MAX_MESSAGE_BYTES));
    room = await broadcast(first, encodeUpdate({ activeMask:0, waitingMask:0 },MAX_MESSAGE_BYTES));
    expect([room.stagePeerId,room.stageSeat]).toEqual(["stage_second",0]);
    // A late result cannot overwrite the chooser or clear a later proposal.
    second.send(encodeReportMatch(completedEpoch,"",255,false,0,MAX_MESSAGE_BYTES));
    const unchanged = nextMessage(second); second.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    expect(await unchanged).toMatchObject({ snapshot: { revision:room.revision, stagePeerId:"stage_second", stageSeat:0 } });
    for (const socket of sockets) socket.close(1000,"test complete");
  });

  it("rotates completed 1v1 seats in waiting order and ignores stale or canceled results", async () => {
    const sockets: WebSocket[] = [];
    async function join(peer: string, role: "active" | "waiting"): Promise<WebSocket> {
      const previous = sockets.map(nextMessage);
      const socket = await connect("ROTATE7", peer, peer, role, 1);
      await nextMessage(socket);
      await nextMessage(socket);
      await Promise.all(previous);
      sockets.push(socket);
      return socket;
    }
    async function broadcast(sender: WebSocket, bytes: ArrayBuffer) {
      const messages = sockets.map(nextMessage);
      sender.send(bytes);
      const delivered = await Promise.all(messages);
      for (const message of delivered) {
        expect(message.type).toBe(ServerMessageType.Presence);
      }
      const first = delivered[0];
      if (first.type !== ServerMessageType.Presence) throw new Error("presence expected");
      return first.snapshot;
    }
    const owner = await join("rot_owner", "active");
    const guest = await join("rot_guest", "active");
    await join("rot_queue1", "waiting");
    await join("rot_queue2", "waiting");

    let room = await broadcast(owner, encodeSetRotation(
      RotationPolicy.WinnerStays, MAX_MESSAGE_BYTES));
    expect(room.rotationPolicy).toBe(RotationPolicy.WinnerStays);
    room = await broadcast(owner, encodeSetMatch(
      new Uint8Array([1]), MAX_MESSAGE_BYTES));
    const firstEpoch = room.matchEpoch;
    const guestPolicyError = nextMessage(guest);
    guest.send(encodeSetRotation(RotationPolicy.WinnerOut, MAX_MESSAGE_BYTES));
    expect(await guestPolicyError).toMatchObject({ type: ServerMessageType.Error });
    const lockedPolicyError = nextMessage(owner);
    owner.send(encodeSetRotation(RotationPolicy.WinnerOut, MAX_MESSAGE_BYTES));
    expect(await lockedPolicyError).toMatchObject({ type: ServerMessageType.Error });
    const waitingReportError = nextMessage(sockets[2]);
    sockets[2].send(encodeReportMatch(
      firstEpoch, "rot_guest", 0, true, 255, MAX_MESSAGE_BYTES));
    expect(await waitingReportError).toMatchObject({ type: ServerMessageType.Error });
    room = await broadcast(guest, encodeReportMatch(
      firstEpoch, "rot_guest", 0, true, 255, MAX_MESSAGE_BYTES));
    expect(room.match).toBeNull();
    expect(room.participants.map((participant) => [
      participant.peerId, participant.activeMask, participant.waitingRanks[0],
    ])).toEqual([
      ["rot_owner", 0, 2], ["rot_guest", 1, 0],
      ["rot_queue1", 1, 0], ["rot_queue2", 0, 1],
    ]);

    const stalePresence = nextMessage(guest);
    guest.send(encodeReportMatch(firstEpoch, "rot_guest", 0, true, 255,
      MAX_MESSAGE_BYTES));
    guest.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    const stale = await stalePresence;
    expect(stale.type).toBe(ServerMessageType.Presence);
    if (stale.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(stale.snapshot.revision).toBe(room.revision);

    await broadcast(owner, encodeSetRotation(
      RotationPolicy.WinnerOut, MAX_MESSAGE_BYTES));
    room = await broadcast(owner, encodeSetMatch(
      new Uint8Array([2]), MAX_MESSAGE_BYTES));
    room = await broadcast(guest, encodeReportMatch(
      room.matchEpoch, "rot_guest", 0, true, 255, MAX_MESSAGE_BYTES));
    expect(room.participants.map((participant) => [
      participant.peerId, participant.activeMask, participant.waitingRanks[0],
    ])).toEqual([
      ["rot_owner", 0, 1], ["rot_guest", 0, 2],
      ["rot_queue1", 1, 0], ["rot_queue2", 1, 0],
    ]);

    room = await broadcast(owner, encodeSetMatch(
      new Uint8Array([3]), MAX_MESSAGE_BYTES));
    room = await broadcast(sockets[2], encodeReportMatch(
      room.matchEpoch, "", 255, false, 255, MAX_MESSAGE_BYTES));
    expect(room.match).toBeNull();
    expect(room.participants.map((participant) => participant.activeMask))
      .toEqual([0, 0, 1, 1]);
    for (const socket of sockets) socket.close(1000, "test complete");
  });

  it("rotates between seats on one shared PC without overwriting either mask", async () => {
    const owner = await connect("COUCH77", "couch_peer", "Couch", "active", 3);
    await nextMessage(owner);
    await nextMessage(owner);
    const changed = nextMessage(owner);
    owner.send(encodeUpdate({ activeMask: 3, waitingMask: 4 }, MAX_MESSAGE_BYTES));
    await changed;
    const policy = nextMessage(owner);
    owner.send(encodeSetRotation(RotationPolicy.WinnerOut, MAX_MESSAGE_BYTES));
    await policy;
    const proposal = nextMessage(owner);
    owner.send(encodeSetMatch(new Uint8Array([7]), MAX_MESSAGE_BYTES));
    const published = await proposal;
    expect(published.type).toBe(ServerMessageType.Presence);
    if (published.type !== ServerMessageType.Presence) throw new Error("presence expected");
    const rotated = nextMessage(owner);
    owner.send(encodeReportMatch(published.snapshot.matchEpoch,
      "couch_peer", 0, true, 255, MAX_MESSAGE_BYTES));
    const completed = await rotated;
    expect(completed.type).toBe(ServerMessageType.Presence);
    if (completed.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(completed.snapshot.participants[0]).toMatchObject({
      activeMask: 6, waitingMask: 1, waitingRanks: [1, 0, 0, 0],
    });
    owner.close(1000, "test complete");
  });

  it("orders waiting seats independently across PCs and preserves order through hibernation", async () => {
    const owner = await connect("QUEUE8", "queue_owner", "Owner", "active", 2);
    await nextMessage(owner);
    await nextMessage(owner);

    const ownerSeesSecond = nextMessage(owner);
    const second = await connect("QUEUE8", "queue_second", "Second", "waiting", 2);
    await nextMessage(second);
    await nextMessage(second);
    await ownerSeesSecond;

    const ownerSeesThird = nextMessage(owner);
    const secondSeesThird = nextMessage(second);
    const third = await connect("QUEUE8", "queue_third", "Third", "waiting", 2);
    await nextMessage(third);
    const joined = await nextMessage(third);
    await ownerSeesThird;
    await secondSeesThird;
    expect(joined.type).toBe(ServerMessageType.Presence);
    if (joined.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(joined.snapshot.participants.map((peer) => peer.waitingRanks)).toEqual([
      [0, 0, 0, 0], [1, 2, 0, 0], [3, 4, 0, 0],
    ]);

    const ownerSeesOverride = nextMessage(owner);
    const secondSeesOverride = nextMessage(second);
    const thirdSeesOverride = nextMessage(third);
    owner.send(encodeAssignSeats("queue_second", 1, 2, MAX_MESSAGE_BYTES));
    const overridden = await secondSeesOverride;
    await ownerSeesOverride;
    await thirdSeesOverride;
    expect(overridden.type).toBe(ServerMessageType.Presence);
    if (overridden.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(overridden.snapshot.participants.map((peer) => peer.waitingRanks)).toEqual([
      [0, 0, 0, 0], [0, 1, 0, 0], [2, 3, 0, 0],
    ]);

    const ownerSeesChoice = nextMessage(owner);
    const secondSeesChoice = nextMessage(second);
    const thirdSeesChoice = nextMessage(third);
    second.send(encodeUpdate({ activeMask: 0, waitingMask: 3 }, MAX_MESSAGE_BYTES));
    const chosen = await secondSeesChoice;
    await ownerSeesChoice;
    await thirdSeesChoice;
    expect(chosen.type).toBe(ServerMessageType.Presence);
    if (chosen.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(chosen.snapshot.participants.map((peer) => peer.waitingRanks)).toEqual([
      [0, 0, 0, 0], [4, 1, 0, 0], [2, 3, 0, 0],
    ]);

    await evictDurableObject(env.ROOMS.getByName("QUEUE8"));
    const restoredMessage = nextMessage(owner);
    owner.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    const restored = await restoredMessage;
    expect(restored.type).toBe(ServerMessageType.Presence);
    if (restored.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(restored.snapshot.participants.map((peer) => peer.waitingRanks)).toEqual(
      chosen.snapshot.participants.map((peer) => peer.waitingRanks),
    );

    owner.close(1000, "test complete");
    second.close(1000, "test complete");
    third.close(1000, "test complete");
  });

  it("queues individual excess couch players and caps active fighters", async () => {
    const first = await connect("SEAT77", "seat_first", "First PC", "active", 3);
    await nextMessage(first);
    await nextMessage(first);

    const firstSeesJoin = nextMessage(first);
    const second = await connect("SEAT77", "seat_second", "Second PC", "auto", 3);
    await nextMessage(second);
    const joined = await nextMessage(second);
    await firstSeesJoin;
    expect(joined.type).toBe(ServerMessageType.Presence);
    if (joined.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(joined.snapshot.participants.find(
      (participant) => participant.peerId === "seat_second",
    )).toMatchObject({ localPlayers: 3, activeMask: 1, waitingMask: 6 });

    const firstSeesSwap = nextMessage(first);
    const secondSeesSwap = nextMessage(second);
    second.send(encodeUpdate({ activeMask: 2, waitingMask: 5 }, MAX_MESSAGE_BYTES));
    const swapped = await secondSeesSwap;
    await firstSeesSwap;
    expect(swapped.type).toBe(ServerMessageType.Presence);
    if (swapped.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(swapped.snapshot.participants.find(
      (participant) => participant.peerId === "seat_second",
    )).toMatchObject({ activeMask: 2, waitingMask: 5 });

    const rejectedOverride = nextMessage(second);
    second.send(encodeAssignSeats("seat_first", 3, 4, MAX_MESSAGE_BYTES));
    expect(await rejectedOverride).toMatchObject({ type: ServerMessageType.Error,
      message: "Only the room owner may assign seats" });

    const firstSeesOverride = nextMessage(first);
    const secondSeesOverride = nextMessage(second);
    first.send(encodeAssignSeats("seat_second", 1, 6, MAX_MESSAGE_BYTES));
    const overridden = await secondSeesOverride;
    await firstSeesOverride;
    expect(overridden.type).toBe(ServerMessageType.Presence);
    if (overridden.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(overridden.snapshot.participants.find(
      (participant) => participant.peerId === "seat_second",
    )).toMatchObject({ activeMask: 1, waitingMask: 6 });

    const firstSeesChoice = nextMessage(first);
    const secondSeesChoice = nextMessage(second);
    second.send(encodeUpdate({ activeMask: 2, waitingMask: 5 }, MAX_MESSAGE_BYTES));
    const chosen = await secondSeesChoice;
    await firstSeesChoice;
    expect(chosen.type).toBe(ServerMessageType.Presence);
    if (chosen.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(chosen.snapshot.participants.find(
      (participant) => participant.peerId === "seat_second",
    )).toMatchObject({ activeMask: 2, waitingMask: 5 });

    const rejected = nextMessage(second);
    second.send(encodeUpdate({ activeMask: 3, waitingMask: 4 }, MAX_MESSAGE_BYTES));
    expect(await rejected).toMatchObject({ type: ServerMessageType.Error,
      message: "All fighting seats are taken" });

    const proposal = new Uint8Array([0x46, 0x4d, 0x4c, 0x01]);
    const firstSeesMatch = nextMessage(first);
    const secondSeesMatch = nextMessage(second);
    first.send(encodeSetMatch(proposal, MAX_MESSAGE_BYTES));
    await firstSeesMatch;
    const publishedMatch = await secondSeesMatch;
    if (publishedMatch.type !== ServerMessageType.Presence) throw new Error("presence expected");

    const firstSeesReady = nextMessage(first);
    const secondSeesReady = nextMessage(second);
    second.send(encodeUpdate({ ready: true }, MAX_MESSAGE_BYTES));
    await firstSeesReady;
    await secondSeesReady;

    const firstSeesWaiting = nextMessage(first);
    const secondSeesWaiting = nextMessage(second);
    second.send(encodeUpdate({ waitingMask: 4 }, MAX_MESSAGE_BYTES));
    const waitingChanged = await secondSeesWaiting;
    await firstSeesWaiting;
    expect(waitingChanged.type).toBe(ServerMessageType.Presence);
    if (waitingChanged.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(waitingChanged.snapshot.matchEpoch).toBe(publishedMatch.snapshot.matchEpoch);
    expect(waitingChanged.snapshot.match).toEqual(proposal);
    expect(waitingChanged.snapshot.participants.find(
      (participant) => participant.peerId === "seat_second",
    )).toMatchObject({ activeMask: 2, waitingMask: 4, ready: true });

    const lockedOverride = nextMessage(first);
    first.send(encodeAssignSeats("seat_second", 1, 4, MAX_MESSAGE_BYTES));
    expect(await lockedOverride).toMatchObject({ type: ServerMessageType.Error,
      message: "Active players are locked for this match" });

    first.close(1000, "test complete");
    second.close(1000, "test complete");
  });

  it("serves a bounded STUN-only configuration when TURN is not configured", async () => {
    const response = await worker.fetch("https://signal.test/v1/turn-credentials");
    expect(response.status).toBe(200);
    expect(response.headers.get("Cache-Control")).toBe("no-store");
    expect(await decodeTurnConfiguration(response)).toEqual({
      hasTurn: false,
      ttlSeconds: 0,
      stunHost: "stun.cloudflare.com",
      stunPort: 3478,
      turnHost: "",
      turnPort: 0,
      username: "",
      password: "",
    });

    const rejected = await worker.fetch("https://signal.test/v1/turn-credentials", {
      method: "POST",
    });
    expect(rejected.status).toBe(405);
  });

  it("exchanges the server-side key for short-lived UDP TURN credentials", async () => {
    const bindings: TurnBindings = {
      TURN_KEY_ID: "turn-key",
      TURN_KEY_API_TOKEN: "secret-token",
    };
    const fetcher: typeof fetch = async (input, init) => {
      expect(String(input)).toBe(
        "https://rtc.live.cloudflare.com/v1/turn/keys/turn-key/credentials/generate-ice-servers",
      );
      expect(init?.method).toBe("POST");
      expect(new Headers(init?.headers).get("Authorization")).toBe("Bearer secret-token");
      expect(init?.body).toBe(JSON.stringify({ ttl: 86_400 }));
      return Response.json({
        iceServers: [
          { urls: ["stun:stun.cloudflare.com:3478"] },
          {
            urls: [
              "turn:turn.cloudflare.com:3478?transport=tcp",
              "turn:turn.cloudflare.com:3478?transport=udp",
            ],
            username: "issued-user",
            credential: "issued-password",
          },
        ],
      });
    };
    const response = await turnCredentialsResponse(bindings, fetcher);
    expect(response.status).toBe(200);
    expect(await decodeTurnConfiguration(response)).toEqual({
      hasTurn: true,
      ttlSeconds: 86_400,
      stunHost: "stun.cloudflare.com",
      stunPort: 3478,
      turnHost: "turn.cloudflare.com",
      turnPort: 3478,
      username: "issued-user",
      password: "issued-password",
    });
  });

  it("rejects malformed credential-service responses", async () => {
    const bindings: TurnBindings = {
      TURN_KEY_ID: "turn-key",
      TURN_KEY_API_TOKEN: "secret-token",
    };
    const malformed: typeof fetch = async () => Response.json({ iceServers: [] });
    const response = await turnCredentialsResponse(bindings, malformed);
    expect(response.status).toBe(502);
  });

  it("reports health and rejects invalid room routes before Durable Object dispatch", async () => {
    const health = await worker.fetch("https://signal.test/health");
    expect(health.status).toBe(200);
    expect(await health.json()).toEqual({ service: "freak-melee-signaling", status: "ok" });

    const invalid = await worker.fetch(
      "https://signal.test/v1/rooms/bad/connect",
      { headers: { Upgrade: "websocket" } },
    );
    expect(invalid.status).toBe(400);

    const missingUpgrade = await worker.fetch(
      "https://signal.test/v1/rooms/ABC123/connect",
    );
    expect(missingUpgrade.status).toBe(426);
  });

  it("coordinates presence, room ownership, updates, and directed signaling", async () => {
    const first = await connect("ROOM42", "peer_one", "Alice", "active", 2);
    const firstWelcome = await nextMessage(first);
    const firstPresence = await nextMessage(first);
    expect(firstWelcome).toMatchObject({
      type: ServerMessageType.Welcome,
      peerId: "peer_one",
    });
    expect(firstPresence).toMatchObject({
      type: ServerMessageType.Presence,
      snapshot: { ownerPeerId: "peer_one" },
    });

    const firstSeesJoin = nextMessage(first);
    const second = await connect("ROOM42", "peer_two", "Bob", "spectator", 0);
    const secondWelcome = await nextMessage(second);
    const secondPresence = await nextMessage(second);
    expect(secondWelcome).toMatchObject({
      type: ServerMessageType.Welcome,
      peerId: "peer_two",
    });
    expect(secondPresence).toMatchObject({
      type: ServerMessageType.Presence,
      snapshot: { ownerPeerId: "peer_one" },
    });
    expect(secondPresence.type).toBe(ServerMessageType.Presence);
    if (secondPresence.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(secondPresence.snapshot.participants).toHaveLength(2);
    const joined = await firstSeesJoin;
    expect(joined.type).toBe(ServerMessageType.Presence);
    if (joined.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(joined.snapshot.participants).toHaveLength(2);

    const signalAtSecond = nextMessage(second);
    const signalAckAtFirst = nextMessage(first);
    const candidate = new TextEncoder().encode("candidate:192.0.2.4:47200");
    first.send(encodeSignalRequest("peer_two", candidate, MAX_MESSAGE_BYTES));
    const receivedSignal = await signalAtSecond;
    expect(receivedSignal).toMatchObject({
      type: ServerMessageType.Signal,
      from: "peer_one",
    });
    if (receivedSignal.type !== ServerMessageType.Signal) throw new Error("signal expected");
    expect(receivedSignal.payload).toEqual(candidate);
    expect(await signalAckAtFirst).toEqual({
      type: ServerMessageType.SignalSent,
      to: "peer_two",
    });

    const updatedAtFirst = nextMessage(first);
    const updatedAtSecond = nextMessage(second);
    const selection = new Uint8Array(335);
    selection.set([0x46, 0x4d, 0x53, 0x4c]);
    second.send(encodeUpdate(
      { activeMask: 0, waitingMask: 1, localPlayers: 1, ready: false, selection },
      MAX_MESSAGE_BYTES,
    ));
    const update = await updatedAtFirst;
    await updatedAtSecond;
    expect(update.type).toBe(ServerMessageType.Presence);
    if (update.type !== ServerMessageType.Presence) throw new Error("presence expected");
    const bob = update.snapshot.participants.find(
      (participant) => participant.peerId === "peer_two",
    );
    expect(bob).toMatchObject({ activeMask: 0, waitingMask: 1,
                                localPlayers: 1, ready: false });
    expect(bob?.selection).toEqual(selection);

    first.close(1000, "test complete");
    second.close(1000, "test complete");
  });

  it("restores participant attachments after Durable Object hibernation", async () => {
    const socket = await connect("SLEEP7", "peer_sleep", "Sleeper", "spectator", 0);
    await nextMessage(socket);
    await nextMessage(socket);
    const policy = nextMessage(socket);
    socket.send(encodeSetRotation(RotationPolicy.WinnerOut, MAX_MESSAGE_BYTES));
    await policy;

    await evictDurableObject(env.ROOMS.getByName("SLEEP7"));
    const presence = nextMessage(socket);
    socket.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    expect(await presence).toMatchObject({
      type: ServerMessageType.Presence,
      snapshot: {
        ownerPeerId: "peer_sleep",
        rotationPolicy: RotationPolicy.WinnerOut,
        participants: [
          {
            peerId: "peer_sleep",
            name: "Sleeper",
            activeMask: 0,
            waitingMask: 0,
            localPlayers: 0,
          },
        ],
      },
    });
    socket.close(1000, "test complete");
  });

  it("keeps owner match proposals durable and invalidates readiness on changes", async () => {
    const owner = await connect("MATCH7", "peer_owner", "Owner", "active", 1);
    await nextMessage(owner);
    await nextMessage(owner);

    const ownerSeesJoin = nextMessage(owner);
    const guest = await connect("MATCH7", "peer_guest", "Guest", "active", 1);
    await nextMessage(guest);
    await nextMessage(guest);
    await ownerSeesJoin;

    const proposal = new Uint8Array([0x46, 0x4d, 0x4c, 0x01]);
    const proposalAtOwner = nextMessage(owner);
    const proposalAtGuest = nextMessage(guest);
    owner.send(encodeSetMatch(proposal, MAX_MESSAGE_BYTES));
    const proposalPresence = await proposalAtOwner;
    await proposalAtGuest;
    expect(proposalPresence.type).toBe(ServerMessageType.Presence);
    if (proposalPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(proposalPresence.snapshot.matchEpoch).toBe(1);
    expect(proposalPresence.snapshot.match).toEqual(proposal);

    const readyAtOwner = nextMessage(owner);
    const readyAtGuest = nextMessage(guest);
    guest.send(encodeUpdate({ ready: true }, MAX_MESSAGE_BYTES));
    await readyAtOwner;
    const readyPresence = await readyAtGuest;
    expect(readyPresence.type).toBe(ServerMessageType.Presence);
    if (readyPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(readyPresence.snapshot.participants.find(
      (participant) => participant.peerId === "peer_guest",
    )?.ready).toBe(true);

    const lockedSelection = nextMessage(guest);
    guest.send(encodeUpdate(
      { selection: new Uint8Array([0x01, 0x02, 0x03]) },
      MAX_MESSAGE_BYTES,
    ));
    expect(await lockedSelection).toMatchObject({ type: ServerMessageType.Error,
      message: "Active players are locked for this match" });

    const clearAtOwner = nextMessage(owner);
    const clearAtGuest = nextMessage(guest);
    owner.send(encodeSetMatch(new Uint8Array(), MAX_MESSAGE_BYTES));
    const cleared = await clearAtOwner;
    await clearAtGuest;
    expect(cleared.type).toBe(ServerMessageType.Presence);
    if (cleared.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(cleared.snapshot.matchEpoch).toBe(2);
    expect(cleared.snapshot.match).toBeNull();
    expect(cleared.snapshot.participants.every(
      (participant) => !participant.ready,
    )).toBe(true);

    const selectionAtOwner = nextMessage(owner);
    const selectionAtGuest = nextMessage(guest);
    guest.send(encodeUpdate(
      { selection: new Uint8Array([0x01, 0x02, 0x03]) },
      MAX_MESSAGE_BYTES,
    ));
    const selectionPresence = await selectionAtOwner;
    await selectionAtGuest;
    expect(selectionPresence.type).toBe(ServerMessageType.Presence);
    if (selectionPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(selectionPresence.snapshot.matchEpoch).toBe(cleared.snapshot.matchEpoch + 1);
    expect(selectionPresence.snapshot.match).toBeNull();
    expect(selectionPresence.snapshot.participants.find(
      (participant) => participant.peerId === "peer_guest",
    )?.selection).toEqual(new Uint8Array([0x01, 0x02, 0x03]));

    const replacement = new Uint8Array([0x46, 0x4d, 0x4c, 0x02]);
    const replacementAtOwner = nextMessage(owner);
    const replacementAtGuest = nextMessage(guest);
    owner.send(encodeSetMatch(replacement, MAX_MESSAGE_BYTES));
    const replacementPresence = await replacementAtOwner;
    await replacementAtGuest;
    expect(replacementPresence.type).toBe(ServerMessageType.Presence);
    if (replacementPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(replacementPresence.snapshot.matchEpoch).toBe(selectionPresence.snapshot.matchEpoch + 1);
    expect(replacementPresence.snapshot.match).toEqual(replacement);
    expect(replacementPresence.snapshot.participants.every(
      (participant) => !participant.ready,
    )).toBe(true);

    const rejected = nextMessage(guest);
    guest.send(encodeSetMatch(proposal, MAX_MESSAGE_BYTES));
    expect(await rejected).toMatchObject({
      type: ServerMessageType.Error,
      message: "Only the room owner may set a bounded match proposal",
    });

    await evictDurableObject(env.ROOMS.getByName("MATCH7"));
    const restored = nextMessage(owner);
    owner.send(encodeRequestPresence(MAX_MESSAGE_BYTES));
    const restoredPresence = await restored;
    expect(restoredPresence.type).toBe(ServerMessageType.Presence);
    if (restoredPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(restoredPresence.snapshot.matchEpoch).toBe(replacementPresence.snapshot.matchEpoch);
    expect(restoredPresence.snapshot.match).toEqual(replacement);

    const transferred = nextMessage(guest);
    owner.close(1000, "owner leaves");
    const transferredPresence = await transferred;
    expect(transferredPresence.type).toBe(ServerMessageType.Presence);
    if (transferredPresence.type !== ServerMessageType.Presence) {
      throw new Error("presence expected");
    }
    expect(transferredPresence.snapshot.ownerPeerId).toBe("peer_guest");
    expect(transferredPresence.snapshot.matchEpoch).toBe(replacementPresence.snapshot.matchEpoch + 1);
    expect(transferredPresence.snapshot.match).toBeNull();
    guest.close(1000, "test complete");
  });

  it("keeps an active match when its spectator owner leaves", async () => {
    const owner = await connect("WATCH7", "peer_watch", "Watcher", "spectator", 0);
    await nextMessage(owner);
    await nextMessage(owner);

    const ownerSeesFirst = nextMessage(owner);
    const first = await connect("WATCH7", "peer_first", "First", "active", 1);
    await nextMessage(first);
    await nextMessage(first);
    await ownerSeesFirst;

    const ownerSeesSecond = nextMessage(owner);
    const firstSeesSecond = nextMessage(first);
    const second = await connect("WATCH7", "peer_second", "Second", "active", 1);
    await nextMessage(second);
    await nextMessage(second);
    await ownerSeesSecond;
    await firstSeesSecond;

    const proposal = new Uint8Array([0x46, 0x4d, 0x4c, 0x01]);
    const ownerSeesMatch = nextMessage(owner);
    const firstSeesMatch = nextMessage(first);
    const secondSeesMatch = nextMessage(second);
    owner.send(encodeSetMatch(proposal, MAX_MESSAGE_BYTES));
    await ownerSeesMatch;
    await firstSeesMatch;
    await secondSeesMatch;

    const ownerSeesWaiting = nextMessage(owner);
    const firstSeesWaiting = nextMessage(first);
    const secondSeesWaiting = nextMessage(second);
    const waiting = await connect("WATCH7", "peer_waiting", "Waiting", "waiting", 1);
    await nextMessage(waiting);
    await nextMessage(waiting);
    await ownerSeesWaiting;
    await firstSeesWaiting;
    await secondSeesWaiting;

    const ownerSeesSelection = nextMessage(owner);
    const firstSeesSelection = nextMessage(first);
    const secondSeesSelection = nextMessage(second);
    const waitingSeesSelection = nextMessage(waiting);
    waiting.send(encodeUpdate({ selection: new Uint8Array([1, 2, 3]) }, MAX_MESSAGE_BYTES));
    await ownerSeesSelection;
    const selection = await firstSeesSelection;
    await secondSeesSelection;
    await waitingSeesSelection;
    expect(selection.type).toBe(ServerMessageType.Presence);
    if (selection.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(selection.snapshot.matchEpoch).toBe(1);
    expect(selection.snapshot.match).toEqual(proposal);

    const firstSeesTransfer = nextMessage(first);
    const secondSeesTransfer = nextMessage(second);
    owner.close(1000, "spectator leaves");
    const transfer = await firstSeesTransfer;
    await secondSeesTransfer;
    expect(transfer.type).toBe(ServerMessageType.Presence);
    if (transfer.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(transfer.snapshot.ownerPeerId).toBe("peer_first");
    expect(transfer.snapshot.matchEpoch).toBe(1);
    expect(transfer.snapshot.match).toEqual(proposal);

    const firstSeesAbort = nextMessage(first);
    const waitingSeesAbort = nextMessage(waiting);
    second.close(1000, "active guest leaves");
    const aborted = await firstSeesAbort;
    await waitingSeesAbort;
    expect(aborted.type).toBe(ServerMessageType.Presence);
    if (aborted.type !== ServerMessageType.Presence) throw new Error("presence expected");
    expect(aborted.snapshot.matchEpoch).toBe(2);
    expect(aborted.snapshot.match).toBeNull();
    first.close(1000, "test complete");
    waiting.close(1000, "test complete");
  });
});
