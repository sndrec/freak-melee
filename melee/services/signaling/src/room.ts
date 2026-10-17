import { DurableObject } from "cloudflare:workers";
import { buildQuickMatch, quickRosterComplete, lockedChoices, QUICK_COUNTDOWN_MS } from "./quick-match";
import { quickPorts, quickStageVotes, agreedQuickStage, type QuickPort, type QuickStageVote } from "./quick-round";
import { quickBucket, quickSeats, sameQuickMembers, QUICK_RULES,
  type QuickOffer, type QuickAllocation, type QuickMember } from "./quick-play";

interface QuickPartyRow extends Record<string, SqlStorageValue> {
  mode: number; generation: number; state: number; room_code: string;
  roster: string; allocation: string | null;
}
interface QuickMatchRow extends Record<string, SqlStorageValue> {
  allocation: string; aborted: number;
}
interface QuickFlowRow extends Record<string, SqlStorageValue> {
  phase: number; deadline: number; revision: number;
}
interface QuickRoundRow extends Record<string, SqlStorageValue> {
  completed_epoch: number; chooser: number; ports: string;
}

import {
  type ConnectionAttachment,
  MAX_ACTIVE_PLAYERS,
  MAX_BUILD_LENGTH,
  MAX_MATCH_BYTES,
  MAX_MESSAGE_BYTES,
  MAX_NAME_LENGTH,
  MAX_PARTICIPANTS,
  MAX_SELECTION_BYTES,
  RotationPolicy,
  type Participant,
  type RoomSnapshot,
  isPeerId,
  isBoundedUtf8,
  countActive,
  parseBoundedInteger,
  publicParticipant,
  validSeatMasks,
} from "./protocol";
import {
  ClientMessageType,
  type ClientMessage,
  decodeClientMessage,
  encodeError,
  encodePresence,
  encodeSignal,
  encodeSignalSent,
  encodeWelcome,
  encodeQuickState, encodeQuickRejected,
  encodeChatEntry,
  encodeChatBegin,
  encodeChatEnd,
  encodeChatRejected,
  type RoomChatEntry,
} from "./wire";

interface RoomChatRow extends RoomChatEntry, Record<string, SqlStorageValue> {}

interface RoomCounter extends Record<string, SqlStorageValue> {
  revision: number;
  join_order: number;
}

interface RoomMatchRow extends Record<string, SqlStorageValue> {
  epoch: number;
  payload: ArrayBuffer | null;
}

interface RoomPolicyRow extends Record<string, SqlStorageValue> {
  rotation: number;
}

function sameBytes(left: Uint8Array, right: Uint8Array): boolean {
  if (left.byteLength !== right.byteLength) return false;
  for (let index = 0; index < left.byteLength; ++index) {
    if (left[index] !== right[index]) return false;
  }
  return true;
}

function waitingOrderForChange(
  previous: ConnectionAttachment | null, waitingMask: number, revision: number,
): [number, number, number, number] {
  const order: [number, number, number, number] = [0, 0, 0, 0];
  for (let seat = 0; seat < 4; ++seat) {
    const bit = 1 << seat;
    if ((waitingMask & bit) !== 0) {
      order[seat] = previous !== null && (previous.waitingMask & bit) !== 0
        ? previous.waitingOrder[seat] : revision * 4 + seat + 1;
    }
  }
  return order;
}

export class Room extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    ctx.blockConcurrencyWhile(async () => {
      this.ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS quick_party (
          id INTEGER PRIMARY KEY CHECK(id=1), mode INTEGER NOT NULL, generation INTEGER NOT NULL,
          state INTEGER NOT NULL, room_code TEXT NOT NULL, roster TEXT NOT NULL, allocation TEXT
        );
        CREATE TABLE IF NOT EXISTS quick_match (
          id INTEGER PRIMARY KEY CHECK(id=1), allocation TEXT NOT NULL, aborted INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS quick_flow (
          id INTEGER PRIMARY KEY CHECK(id=1), phase INTEGER NOT NULL,
          deadline INTEGER NOT NULL, revision INTEGER NOT NULL
        );
        INSERT OR IGNORE INTO quick_flow VALUES (1,0,0,0);
        CREATE TABLE IF NOT EXISTS quick_round (
          id INTEGER PRIMARY KEY CHECK(id=1), completed_epoch INTEGER NOT NULL,
          chooser INTEGER NOT NULL, ports TEXT NOT NULL
        );
        INSERT OR IGNORE INTO quick_round VALUES (1,0,255,'[]');
        CREATE TABLE IF NOT EXISTS quick_results (
          peer TEXT PRIMARY KEY, epoch INTEGER NOT NULL, winner INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS quick_stage_votes (
          peer TEXT NOT NULL, seat INTEGER NOT NULL, stage INTEGER NOT NULL,
          PRIMARY KEY(peer, seat)
        );
        CREATE TABLE IF NOT EXISTS room_chat (
          sequence INTEGER PRIMARY KEY AUTOINCREMENT,
          connection TEXT NOT NULL, nonce INTEGER NOT NULL,
          peer TEXT NOT NULL, name TEXT NOT NULL, text TEXT NOT NULL,
          UNIQUE(connection, nonce)
        );
        CREATE TABLE IF NOT EXISTS room_counter (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          revision INTEGER NOT NULL,
          join_order INTEGER NOT NULL
        );
        INSERT OR IGNORE INTO room_counter (id, revision, join_order)
        VALUES (1, 0, 0);
        CREATE TABLE IF NOT EXISTS room_match (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          epoch INTEGER NOT NULL,
          payload BLOB
        );
        INSERT OR IGNORE INTO room_match (id, epoch, payload)
        VALUES (1, 0, NULL);
        CREATE TABLE IF NOT EXISTS room_policy (
          id INTEGER PRIMARY KEY CHECK (id = 1),
          rotation INTEGER NOT NULL
        );
        INSERT OR IGNORE INTO room_policy (id, rotation)
        VALUES (1, 0);
      `);
    });
    this.ctx.setWebSocketAutoResponse(
      new WebSocketRequestResponsePair("ping", "pong"),
    );
  }

  async fetch(request: Request): Promise<Response> {
    if (request.method !== "GET" || request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("Expected a WebSocket upgrade", { status: 426 });
    }

    const url = new URL(request.url);
    const peerId = url.searchParams.get("peer") ?? "";
    const name = url.searchParams.get("name") ?? "";
    const build = url.searchParams.get("build") ?? "";
    const localPlayers = parseBoundedInteger(url.searchParams.get("players"), 0, 4);
    const activeQuery = url.searchParams.get("active");
    const waitingQuery = url.searchParams.get("waiting");
    if (
      !isPeerId(peerId) ||
      !isBoundedUtf8(name, 1, MAX_NAME_LENGTH) ||
      !isBoundedUtf8(build, 1, MAX_BUILD_LENGTH) ||
      localPlayers === null ||
      (activeQuery === null) !== (waitingQuery === null)
    ) {
      return Response.json({ error: "Invalid room participant" }, { status: 400 });
    }

    const admission = this.quickMatch();
    if (admission) {
      const allocation = JSON.parse(admission.allocation) as QuickAllocation;
      const player = allocation.players.find((entry) => entry.peerId === peerId);
      if (admission.aborted || !player || url.searchParams.get("admission") !== player.admission ||
          build !== allocation.build || localPlayers !== player.localPlayers)
        return new Response("Quick Play admission rejected", { status: 403 });
    }
    const quick = this.quickParty();
    const current = this.snapshot(this.currentRevision());
    const replacement = current.participants.find((peer) => peer.peerId === peerId);
    const returning = quick?.state === 3 && (JSON.parse(quick.roster) as QuickOffer).build === build &&
      (JSON.parse(quick.roster) as QuickOffer).members.some((member) =>
        member.peerId === peerId && member.localPlayers === localPlayers);
    if (quick?.mode && !replacement && !returning) {
      const occupied = current.participants.reduce((sum, peer) => sum + peer.localPlayers, 0);
      if (quick.state !== 0 || localPlayers === 0 || occupied + localPlayers > quick.mode ||
          current.participants.some((peer) => peer.build !== build))
        return new Response("Quick Play party is unavailable or full", { status: 409 });
    }
    const previousConnections = this.ctx.getWebSockets(`peer:${peerId}`);
    if (
      previousConnections.length === 0 &&
      this.ctx.getWebSockets().length >= MAX_PARTICIPANTS
    ) {
      return new Response("Room is full", { status: 409 });
    }
    let occupied = 0;
    for (const connected of this.ctx.getWebSockets()) {
      if (previousConnections.includes(connected) || connected.readyState !== WebSocket.OPEN) continue;
      occupied += countActive(this.attachment(connected)?.activeMask ?? 0);
    }
    const previous = previousConnections.length !== 0
      ? this.attachment(previousConnections[0]) : null;
    const free = Math.max(0, MAX_ACTIVE_PLAYERS - occupied);
    const requestedActive = activeQuery !== null
      ? parseBoundedInteger(activeQuery, 0, 15) : null;
    const requestedWaiting = waitingQuery !== null
      ? parseBoundedInteger(waitingQuery, 0, 15) : null;
    const assignedCount = this.currentMatch().payload === null
      ? Math.min(localPlayers, free) : 0;
    const defaultActive = previous?.localPlayers === localPlayers
      ? previous.activeMask : (1 << assignedCount) - 1;
    const activeMask = requestedActive ?? defaultActive;
    const waitingMask = requestedWaiting ?? (
      previous?.localPlayers === localPlayers
        ? previous.waitingMask : ((1 << localPlayers) - 1) & ~activeMask
    );
    if ((activeQuery !== null && requestedActive === null) ||
        (waitingQuery !== null && requestedWaiting === null) ||
        !validSeatMasks(localPlayers, activeMask, waitingMask) ||
        countActive(activeMask) > free ||
        (this.currentMatch().payload !== null && previous === null && activeMask !== 0)) {
      return Response.json({ error: "Invalid local player assignment" }, { status: 400 });
    }

    if ((quick?.mode || admission) && (localPlayers === 0 || activeMask !== (1 << localPlayers) - 1 || waitingMask !== 0))
      return new Response("Quick Play uses active human seats", { status: 400 });

    if (quick?.mode && replacement) this.invalidateQuickSearch();
    const counters = this.ctx.storage.sql
      .exec<RoomCounter>(`
        UPDATE room_counter
        SET revision = revision + 1, join_order = join_order + 1
        WHERE id = 1
        RETURNING revision, join_order
      `)
      .one();
    const attachment: ConnectionAttachment = {
      connectionId: crypto.randomUUID(),
      peerId,
      name,
      build,
      localPlayers,
      activeMask,
      waitingMask,
      ready: false,
      selection: new Uint8Array(),
      joinOrder: counters.join_order,
      waitingOrder: waitingOrderForChange(
        previous?.localPlayers === localPlayers ? previous : null,
        waitingMask, counters.revision,
      ),
    };

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];
    server.serializeAttachment(attachment);
    this.ctx.acceptWebSocket(server, [`peer:${peerId}`]);
    for (const oldSocket of previousConnections) {
      oldSocket.close(4001, "Replaced by a newer connection");
    }
    if (previousConnections.length === 0 && activeMask !== 0) {
      this.clearReady();
    }
    this.send(server, encodeWelcome(
      attachment.connectionId,
      peerId,
      counters.revision,
      MAX_MESSAGE_BYTES,
    ));
    this.broadcastPresence(counters.revision);
    await this.refreshQuickCountdown();
    if (quick?.mode || admission) this.sendQuick(server);
    return new Response(null, { status: 101, webSocket: client });
  }

  async webSocketMessage(socket: WebSocket, message: string | ArrayBuffer): Promise<void> {
    if (typeof message === "string" || message.byteLength > MAX_MESSAGE_BYTES) {
      this.error(socket, "Messages must use the bounded binary room protocol");
      return;
    }

    let parsed: ClientMessage;
    try {
      parsed = decodeClientMessage(message);
    } catch {
      const bytes = new Uint8Array(message);
      if (bytes[5] === ClientMessageType.PostChat || bytes[5] === ClientMessageType.RequestChat) {
        this.send(socket, encodeChatRejected(0, "Malformed chat message."));
        return;
      }
      if (bytes[5] >= ClientMessageType.ConfigureQuick && bytes[5] <= ClientMessageType.UnreadyQuick) {
        this.send(socket, encodeQuickRejected("Malformed Quick Play command.")); return;
      }
      this.error(socket, "Malformed room message");
      return;
    }

    switch (parsed.type) {
    case ClientMessageType.UnreadyQuick:
      await this.unreadyQuick(socket, parsed.completedEpoch);
      return;
    case ClientMessageType.ReportQuick:
      await this.reportQuick(socket, parsed.epoch, parsed.winner);
      return;
    case ClientMessageType.VoteQuickStage:
      await this.voteQuickStage(socket, parsed.epoch, parsed.seat, parsed.stage);
      return;
    case ClientMessageType.PrepareQuick:
      await this.prepareQuick(socket, parsed);
      return;
    case ClientMessageType.ConfigureQuick:
      this.configureQuickParty(socket, parsed.mode);
      return;
    case ClientMessageType.QueueQuick:
      try { await this.queueQuick(socket, parsed.searching); }
      catch (error) {
        this.invalidateQuickSearch();
        console.error(JSON.stringify({ message: "quick play search failed", error: String(error) }));
        this.send(socket, encodeQuickRejected("Quick Play is unavailable. Please try again."));
      }
      return;
    case ClientMessageType.RequestQuick:
      this.sendQuick(socket);
      return;
    case ClientMessageType.PostChat:
      this.postChat(socket, parsed.nonce, parsed.text);
      return;
    case ClientMessageType.RequestChat:
      this.chatHistory(socket);
      return;
    case ClientMessageType.Update:
      this.updateParticipant(socket, parsed);
      await this.refreshQuickCountdown();
      return;
    case ClientMessageType.Signal:
      this.forwardSignal(socket, parsed);
      return;
    case ClientMessageType.RequestPresence:
      this.send(socket, encodePresence(this.snapshot(this.currentRevision()), MAX_MESSAGE_BYTES));
      return;
    case ClientMessageType.SetMatch:
      this.setMatch(socket, parsed.payload);
      return;
    case ClientMessageType.AssignSeats:
      this.assignSeats(socket, parsed);
      return;
    case ClientMessageType.SetRotation:
      this.setRotation(socket, parsed.policy);
      return;
    case ClientMessageType.ReportMatch:
      this.reportMatch(socket, parsed);
      return;
    }
  }

  async webSocketClose(socket: WebSocket): Promise<void> {
    this.removeParticipant(socket);
    await this.refreshQuickCountdown();
    socket.close(1000, "Lobby connection closed");
  }

  async webSocketError(socket: WebSocket): Promise<void> {
    this.removeParticipant(socket);
    await this.refreshQuickCountdown();
    socket.close(1011, "Lobby connection failed");
  }

  private quickFlow(): QuickFlowRow {
    return this.ctx.storage.sql.exec<QuickFlowRow>("SELECT phase, deadline, revision FROM quick_flow WHERE id=1").one();
  }

  private async unreadyQuick(socket: WebSocket, completedEpoch: number): Promise<void> {
    const match = this.quickMatch(), peer = this.attachment(socket);
    if (!match || match.aborted || !peer) return;
    if (this.quickFlow().phase < 2 && this.quickRound().completed_epoch === completedEpoch) {
      const selection = peer.selection.slice();
      if (selection.length >= 7) selection[6] = 0;
      socket.serializeAttachment({ ...peer, ready: false, selection,
        quickRevision: undefined, quickDigest: undefined } satisfies ConnectionAttachment);
      this.clearReady();
      this.broadcastPresence(this.nextRevision());
      await this.refreshQuickCountdown();
    } else {
      // The committed proposal wins a late cancellation. Return its authoritative
      // selection and phase so the sender continues through the shared launch.
      this.send(socket, encodePresence(this.snapshot(this.currentRevision()), MAX_MESSAGE_BYTES));
    }
    this.sendQuick(socket);
  }

  private async prepareQuick(socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.PrepareQuick }>): Promise<void> {
    const match = this.quickMatch(), peer = this.attachment(socket);
    if (!match || match.aborted || !peer || this.quickFlow().phase >= 2) return;
    if (message.prepared && (message.revision !== this.currentRevision() ||
        !message.digest.some((byte) => byte !== 0) || !lockedChoices(peer.selection, peer.localPlayers))) {
      this.send(socket, encodePresence(this.snapshot(this.currentRevision()), MAX_MESSAGE_BYTES));
      this.sendQuick(socket);
      return;
    }
    socket.serializeAttachment({ ...peer,
      quickRevision: message.prepared ? message.revision : undefined,
      quickDigest: message.prepared ? message.digest : undefined,
    } satisfies ConnectionAttachment);
    await this.refreshQuickCountdown();
  }

  private preparedQuickMatch(): { allocation: QuickAllocation; people: Participant[]; digest: Uint8Array; revision: number } | null {
    const match = this.quickMatch();
    if (!match || match.aborted) return null;
    const allocation = JSON.parse(match.allocation) as QuickAllocation;
    const revision = this.currentRevision();
    const people = this.snapshot(revision).participants;
    if (!quickRosterComplete(allocation, people)) return null;
    let digest: Uint8Array | undefined;
    for (const person of people) {
      const socket = this.ctx.getWebSockets(`peer:${person.peerId}`).find((peer) => peer.readyState === WebSocket.OPEN);
      const peer = socket ? this.attachment(socket) : null;
      if (!peer || peer.quickRevision !== revision || !peer.quickDigest || peer.quickDigest.length !== 32) return null;
      if (digest && !sameBytes(digest, peer.quickDigest)) return null;
      digest = peer.quickDigest;
    }
    return digest ? { allocation, people, digest, revision } : null;
  }

  private async refreshQuickCountdown(): Promise<void> {
    if (!this.quickMatch()) return;
    const flow = this.quickFlow();
    if (flow.phase >= 2) return;
    const prepared = this.preparedQuickMatch();
    if (!prepared) {
      if (flow.phase === 1) {
        this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=0, deadline=0, revision=0 WHERE id=1");
        await this.ctx.storage.deleteAlarm();
        this.sendQuick();
      }
      return;
    }
    if (flow.phase === 1 && flow.revision === prepared.revision) return;
    const deadline = Date.now() + QUICK_COUNTDOWN_MS;
    this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=1, deadline=?, revision=? WHERE id=1", deadline, prepared.revision);
    await this.ctx.storage.setAlarm(deadline);
    this.sendQuick();
  }

  async alarm(): Promise<void> {
    const flow = this.quickFlow();
    if (flow.phase !== 1) return;
    const prepared = this.preparedQuickMatch();
    if (!prepared || prepared.revision !== flow.revision) {
      await this.refreshQuickCountdown();
      return;
    }
    if (Date.now() < flow.deadline) {
      await this.ctx.storage.setAlarm(flow.deadline);
      return;
    }
    const proposal = buildQuickMatch(prepared.allocation, prepared.people, prepared.digest);
    const ports = quickPorts(prepared.allocation, prepared.people);
    this.ctx.storage.sql.exec("UPDATE quick_round SET ports=? WHERE id=1", JSON.stringify(ports));
    this.ctx.storage.sql.exec("DELETE FROM quick_results");
    this.ctx.storage.sql.exec("DELETE FROM quick_stage_votes");
    this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=2, deadline=0 WHERE id=1");
    this.ctx.storage.sql.exec("UPDATE room_match SET epoch=epoch+1, payload=? WHERE id=1", proposal);
    this.clearReady();
    this.broadcastPresence(this.nextRevision());
    this.sendQuick();
  }

  private quickParty(): QuickPartyRow | null {
    return this.ctx.storage.sql.exec<QuickPartyRow>("SELECT * FROM quick_party WHERE id=1").toArray()[0] ?? null;
  }
  private quickRound(): QuickRoundRow {
    return this.ctx.storage.sql.exec<QuickRoundRow>("SELECT completed_epoch, chooser, ports FROM quick_round WHERE id=1").one();
  }
  private stageVotes(): QuickStageVote[] {
    return this.ctx.storage.sql.exec<QuickStageVote & Record<string, SqlStorageValue>>(
      "SELECT peer, seat, stage FROM quick_stage_votes").toArray();
  }

  private async reportQuick(socket: WebSocket, epoch: number, winner: number): Promise<void> {
    const assigned = this.quickMatch(), peer = this.attachment(socket), current = this.currentMatch();
    if (!assigned || assigned.aborted || !peer || !current.payload || current.epoch !== epoch) return;
    const round = this.quickRound(), flow = this.quickFlow();
    const ports = JSON.parse(round.ports) as QuickPort[];
    if ((flow.phase !== 2 && flow.phase !== 4) || !ports.some((p) => p.peerId === peer.peerId) ||
        (winner !== 255 && !ports.some((p) => p.team === winner))) return;
    const allocation = JSON.parse(assigned.allocation) as QuickAllocation;
    if (allocation.players.some((player) => !this.ctx.getWebSockets(`peer:${player.peerId}`)
        .some((s) => s.readyState === WebSocket.OPEN))) {
      // A lost signaling socket does not stop a viable P2P game. An end-of-game
      // report establishes the boundary at which the incomplete group retires.
      await this.abortQuickAllocation(allocation.id);
      return;
    }
    const previous = this.ctx.storage.sql.exec<{winner: number}>(
      "SELECT winner FROM quick_results WHERE peer=? AND epoch=?", peer.peerId, epoch).toArray()[0];
    if (previous && previous.winner !== winner) {
      this.send(socket, encodeQuickRejected("A reported match result cannot change. Leave this match to search again."));
      return;
    }
    this.ctx.storage.sql.exec("INSERT OR IGNORE INTO quick_results VALUES (?,?,?)", peer.peerId, epoch, winner);
    const reports = this.ctx.storage.sql.exec<{peer: string; winner: number}>(
      "SELECT peer, winner FROM quick_results WHERE epoch=?", epoch).toArray();
    this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=4 WHERE id=1");
    if (reports.some((report) => report.winner !== winner)) {
      this.sendQuick();
      for (const member of this.ctx.getWebSockets())
        this.send(member, encodeQuickRejected("Players reported different results. Leave this match to search again."));
      return;
    }
    if (reports.length !== allocation.players.length) {
      this.sendQuick();
      return;
    }
    this.ctx.storage.sql.exec("UPDATE room_match SET epoch=epoch+1, payload=NULL WHERE id=1");
    this.ctx.storage.sql.exec("UPDATE quick_round SET completed_epoch=?, chooser=? WHERE id=1", epoch, winner === 255 ? 255 : 1-winner);
    this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=?, deadline=0, revision=0 WHERE id=1", winner === 255 ? 0 : 3);
    this.ctx.storage.sql.exec("DELETE FROM quick_stage_votes");
    for (const member of this.ctx.getWebSockets()) {
      const p = this.attachment(member);
      if (!p) continue;
      const selection = p.selection.slice();
      if (selection.length >= 7) selection[6] = 0;
      member.serializeAttachment({ ...p, ready: false, selection, quickRevision: undefined, quickDigest: undefined } satisfies ConnectionAttachment);
    }
    this.broadcastPresence(this.nextRevision());
    this.sendQuick();
  }

  private async voteQuickStage(socket: WebSocket, epoch: number, seat: number, stage: number): Promise<void> {
    const assigned = this.quickMatch(), peer = this.attachment(socket), round = this.quickRound();
    if (!assigned || assigned.aborted || !peer || this.quickFlow().phase !== 3 || epoch !== round.completed_epoch) return;
    const ports = JSON.parse(round.ports) as QuickPort[];
    const player = ports.find((p) => p.peerId === peer.peerId && p.seat === seat);
    if (!player || player.team !== round.chooser) {
      this.send(socket, encodeQuickRejected("The losing side chooses the next stage.")); return;
    }
    if (stage === 255) this.ctx.storage.sql.exec("DELETE FROM quick_stage_votes WHERE peer=? AND seat=?", peer.peerId, seat);
    else this.ctx.storage.sql.exec("INSERT OR REPLACE INTO quick_stage_votes VALUES (?,?,?)", peer.peerId, seat, stage);
    const agreed = agreedQuickStage(ports, round.chooser, this.stageVotes());
    if (agreed !== null) {
      const allocation = JSON.parse(assigned.allocation) as QuickAllocation;
      allocation.stage = agreed;
      this.ctx.storage.sql.exec("UPDATE quick_match SET allocation=? WHERE id=1", JSON.stringify(allocation));
      this.ctx.storage.sql.exec("UPDATE quick_round SET chooser=255 WHERE id=1");
      this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=0 WHERE id=1");
    }
    this.sendQuick();
    await this.refreshQuickCountdown();
  }
  private quickMatch(): QuickMatchRow | null {
    return this.ctx.storage.sql.exec<QuickMatchRow>("SELECT * FROM quick_match WHERE id=1").toArray()[0] ?? null;
  }
  private quickRoster(mode: number): QuickOffer | null {
    const connected: ConnectionAttachment[] = [];
    let build = "";
    for (const socket of this.ctx.getWebSockets()) {
      if (socket.readyState !== WebSocket.OPEN) continue;
      const p = this.attachment(socket);
      if (!p || p.localPlayers < 1 || p.activeMask !== (1 << p.localPlayers) - 1 || p.waitingMask ||
          (build && build !== p.build)) return null;
      build = p.build;
      connected.push(p);
    }
    connected.sort((a, b) => a.joinOrder - b.joinOrder);
    const members: QuickMember[] = connected.map((p) => ({ peerId: p.peerId, name: p.name,
      localPlayers: p.localPlayers, connectionId: p.connectionId }));
    const row = this.quickParty();
    if (!row || (mode !== 1 && mode !== 2)) return null;
    const offer: QuickOffer = { partyId: this.ctx.id.toString(), roomCode: row.room_code,
      generation: row.generation, mode, build, members };
    return members.length && quickSeats(offer) <= mode ? offer : null;
  }
  private sendQuick(socket?: WebSocket): void {
    const party = this.quickParty();
    const match = this.quickMatch();
    const flow = this.quickFlow();
    const round = this.quickRound();
    const votes = quickStageVotes(JSON.parse(round.ports) as QuickPort[], this.stageVotes());
    for (const peer of socket ? [socket] : this.ctx.getWebSockets()) {
      if (peer.readyState !== WebSocket.OPEN) continue;
      const p = this.attachment(peer);
      if (!p) continue;
      const allocation = match ? JSON.parse(match.allocation) as QuickAllocation :
        party?.allocation ? JSON.parse(party.allocation) as QuickAllocation : null;
      const player = allocation?.players.find((entry) => entry.peerId === p.peerId);
      const assigned = Boolean(match || party?.state === 3);
      this.send(peer, encodeQuickState({ mode: match ? allocation!.mode : party?.mode ?? 0,
        state: match?.aborted ? 3 : assigned ? 2 : party && party.state > 0 ? 1 : 0,
        generation: party?.generation ?? 0,
        matchRoom: assigned ? allocation?.roomCode ?? "" : "",
        admission: assigned ? player?.admission ?? "" : "",
        team: player?.team ?? 0, stage: allocation?.stage ?? 0,
        phase: match && !match.aborted ? flow.phase : 0,
        remainingMs: match && !match.aborted && flow.phase === 1 ? Math.max(0, Math.min(QUICK_COUNTDOWN_MS, flow.deadline-Date.now())) : 0,
        completedEpoch: round.completed_epoch, chooser: round.chooser, votes }));
    }
  }
  private configureQuickParty(socket: WebSocket, mode: number): void {
    const owner = this.snapshot(this.currentRevision()).ownerPeerId;
    const peer = this.attachment(socket);
    const prior = this.quickParty();
    const participants = this.snapshot(this.currentRevision()).participants;
    const seats = participants.reduce((sum, p) => sum + p.localPlayers, 0);
    if (!peer || owner !== peer.peerId || (mode !== 0 && mode !== 1 && mode !== 2) ||
        this.currentMatch().payload || this.quickMatch() || (prior && prior.state !== 0) ||
        (mode && (seats > mode || seats < 1 || participants.some((p) =>
          p.build !== peer.build || p.activeMask !== (1 << p.localPlayers) - 1 || !p.localPlayers || p.waitingMask)))) {
      this.send(socket, encodeQuickRejected("Create a compatible solo or two-player party first.")); return;
    }
    const roomCode = prior?.room_code || this.ctx.id.name;
    if (!roomCode) throw new Error("Missing named party room");
    this.ctx.storage.sql.exec("INSERT OR REPLACE INTO quick_party VALUES (1, ?, ?, 0, ?, '[]', NULL)",
      mode, (prior?.generation ?? 0) + 1, roomCode);
    this.sendQuick();
  }
  private invalidateQuickSearch(): void {
    const row = this.quickParty();
    if (!row || (row.state !== 1 && row.state !== 2)) return;
    const offer = JSON.parse(row.roster) as QuickOffer;
    this.ctx.storage.sql.exec("UPDATE quick_party SET state=0, generation=generation+1, allocation=NULL WHERE id=1");
    this.sendQuick();
    this.ctx.waitUntil(this.env.QUICK_PLAY.getByName(quickBucket(offer.mode, offer.build))
      .cancel(offer.partyId, offer.generation).catch((error: unknown) => {
        console.error(JSON.stringify({ message: "quick search cancellation deferred to lease", error: String(error) }));
      }));
  }
  private async queueQuick(socket: WebSocket, searching: boolean): Promise<void> {
    const row = this.quickParty();
    const peer = this.attachment(socket);
    if (!row?.mode || !peer || this.quickMatch() ||
        this.snapshot(this.currentRevision()).ownerPeerId !== peer.peerId) {
      this.send(socket, encodeQuickRejected("Only the party leader can enter or cancel the queue.")); return;
    }
    if (!searching) {
      if (row.state === 3 && row.allocation) {
        const allocation = JSON.parse(row.allocation) as QuickAllocation;
        const previous = JSON.parse(row.roster) as QuickOffer;
        this.ctx.storage.sql.exec("UPDATE quick_party SET state=0, generation=generation+1, allocation=NULL WHERE id=1");
        this.sendQuick();
        await this.env.QUICK_PLAY.getByName(quickBucket(previous.mode, previous.build)).cancel(previous.partyId, previous.generation);
        await this.env.ROOMS.getByName(allocation.roomCode).abortQuickAllocation(allocation.id);
      } else this.invalidateQuickSearch();
      return;
    }
    if (row.state === 2 || row.state === 3) { this.sendQuick(socket); return; }
    const offer = this.quickRoster(row.mode);
    if (!offer) { this.send(socket, encodeQuickRejected("The party is not ready to search.")); return; }
    if (row.state === 0) offer.generation++;
    this.ctx.storage.sql.exec("UPDATE quick_party SET generation=?, state=1, roster=?, allocation=NULL WHERE id=1",
      offer.generation, JSON.stringify(offer));
    const accepted = await this.env.QUICK_PLAY.getByName(quickBucket(offer.mode, offer.build)).enqueue(offer);
    if (this.quickParty()?.generation !== offer.generation) return;
    if (!accepted) {
      this.ctx.storage.sql.exec("UPDATE quick_party SET state=0 WHERE id=1");
      this.send(socket, encodeQuickRejected("The search expired or the queue is full. Enter again."));
    }
    this.sendQuick();
  }
  async prepareQuickAllocation(generation: number, allocation: QuickAllocation): Promise<boolean> {
    const row = this.quickParty();
    if (row?.state === 3 && row.generation === generation && row.allocation &&
        (JSON.parse(row.allocation) as QuickAllocation).id === allocation.id) return true;
    const offer = row && this.quickRoster(row.mode);
    const expected = allocation.parties.find((party) => party.partyId === this.ctx.id.toString());
    if (!row || !offer || !expected || row.generation !== generation || expected.generation !== generation ||
        (row.state !== 1 && row.state !== 2 && row.state !== 3) ||
        (row.allocation && (JSON.parse(row.allocation) as QuickAllocation).id !== allocation.id) ||
        offer.mode !== allocation.mode || offer.build !== allocation.build ||
        !sameQuickMembers(offer.members, expected.members)) return false;
    this.ctx.storage.sql.exec("UPDATE quick_party SET state=?, allocation=? WHERE id=1",
      row.state === 3 ? 3 : 2, JSON.stringify(allocation));
    return true;
  }
  async activateQuickAllocation(generation: number, id: string): Promise<boolean> {
    const row = this.quickParty();
    if (!row || row.generation !== generation || !row.allocation ||
        (row.state !== 2 && row.state !== 3)) return false;
    const allocation = JSON.parse(row.allocation) as QuickAllocation;
    if (row.state === 3 && allocation.id === id) return true;
    const expected = allocation.parties.find((party) => party.partyId === this.ctx.id.toString());
    const current = this.quickRoster(row.mode);
    if (allocation.id !== id || !current || !expected || !sameQuickMembers(current.members, expected.members)) return false;
    this.ctx.storage.sql.exec("UPDATE quick_party SET state=3 WHERE id=1");
    this.sendQuick();
    return true;
  }
  async releaseQuickAllocation(generation: number, id: string): Promise<QuickOffer | null> {
    const row = this.quickParty();
    if (!row || row.generation !== generation || (row.allocation && (JSON.parse(row.allocation) as QuickAllocation).id !== id)) return null;
    const offer = this.quickRoster(row.mode);
    this.ctx.storage.sql.exec("UPDATE quick_party SET state=?, allocation=NULL WHERE id=1", offer ? 1 : 0);
    this.sendQuick();
    return offer;
  }
  async expireQuickSearch(generation: number): Promise<void> {
    if (this.quickParty()?.generation === generation) this.invalidateQuickSearch();
  }
  async configureQuickMatch(allocation: QuickAllocation): Promise<boolean> {
    const previous = this.quickMatch();
    if (previous) return !previous.aborted && (JSON.parse(previous.allocation) as QuickAllocation).id === allocation.id;
    if (this.ctx.getWebSockets().some((socket) => socket.readyState === WebSocket.OPEN) ||
        this.currentMatch().payload || this.quickParty() || !allocation.players.length ||
        allocation.players.reduce((sum, p) => sum + p.localPlayers, 0) !== allocation.mode * 2 ||
        allocation.stage < 0 || allocation.stage > 5 ||
        (allocation.rules.stocks !== QUICK_RULES.stocks || allocation.rules.minutes !== QUICK_RULES.minutes ||
         allocation.rules.items !== QUICK_RULES.items)) return false;
    this.ctx.storage.sql.exec("INSERT INTO quick_match VALUES (1, ?, 0)", JSON.stringify(allocation));
    return true;
  }
  async abortQuickAllocation(id: string): Promise<void> {
    const row = this.quickMatch();
    if (!row || (JSON.parse(row.allocation) as QuickAllocation).id !== id) return;
    this.ctx.storage.sql.exec("UPDATE quick_match SET aborted=1 WHERE id=1");
    await this.refreshQuickCountdown();
    this.sendQuick();
    for (const socket of this.ctx.getWebSockets()) socket.close(4005, "Quick Play allocation cancelled");
  }

  private chatHistory(socket: WebSocket): void {
    const participant = this.attachment(socket);
    if (participant === null) return;
    const now = Date.now();
    if (participant.chatLastHistory !== undefined && now - participant.chatLastHistory < 1000) {
      this.send(socket, encodeChatRejected(0, "Please wait a moment before reloading chat."));
      return;
    }
    socket.serializeAttachment({ ...participant, chatSubscribed: true, chatLastHistory: now });
    const rows = this.ctx.storage.sql.exec<RoomChatRow>(
      "SELECT sequence, nonce, peer AS peerId, name, text FROM room_chat ORDER BY sequence",
    ).toArray();
    this.send(socket, encodeChatBegin(rows.length));
    for (const row of rows) this.send(socket, encodeChatEntry(row));
    this.send(socket, encodeChatEnd(rows.at(-1)?.sequence ?? 0));
  }

  private postChat(socket: WebSocket, nonce: number, text: string): void {
    const participant = this.attachment(socket);
    if (participant === null) return;
    const reject = (reason: string) => this.send(socket, encodeChatRejected(nonce, reason));
    if (nonce === 0 || !participant.chatSubscribed || !isBoundedUtf8(text, 1, 512) ||
        /[\u0000-\u001f\u007f]/u.test(text) || !/[^ ]/u.test(text)) {
      reject("Write a message of up to 512 bytes."); return;
    }
    const previous = this.ctx.storage.sql.exec<RoomChatRow>(
      "SELECT sequence, nonce, peer AS peerId, name, text FROM room_chat WHERE connection = ? AND nonce = ?",
      participant.connectionId, nonce,
    ).toArray()[0];
    if (previous !== undefined) {
      if (previous.text !== text) reject("That message number has already been used.");
      else this.send(socket, encodeChatEntry(previous));
      return;
    }
    if (nonce <= (participant.chatHighestNonce ?? 0)) {
      reject("That message is no longer in the lobby history."); return;
    }
    const now = Date.now();
    if (participant.chatLastPost !== undefined && now - participant.chatLastPost < 500) {
      reject("Please wait a moment before sending another message."); return;
    }
    const row = this.ctx.storage.sql.exec<RoomChatRow>(
      "INSERT INTO room_chat (connection, nonce, peer, name, text) VALUES (?, ?, ?, ?, ?) RETURNING sequence, nonce, peer AS peerId, name, text",
      participant.connectionId, nonce, participant.peerId, participant.name, text,
    ).one();
    this.ctx.storage.sql.exec("DELETE FROM room_chat WHERE sequence <= ?", row.sequence - 64);
    socket.serializeAttachment({ ...participant, chatLastPost: now, chatHighestNonce: nonce });
    const bytes = encodeChatEntry(row);
    for (const recipient of this.ctx.getWebSockets()) {
      if (recipient.readyState === WebSocket.OPEN && this.attachment(recipient)?.chatSubscribed)
        this.send(recipient, bytes);
    }
  }

  private updateParticipant(
    socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.Update }>,
  ): void {
    const current = this.attachment(socket);
    if (current === null) {
      socket.close(1011, "Missing participant state");
      return;
    }
    const name = message.name === undefined ? current.name : message.name;
    const activeMask = message.activeMask === undefined
      ? current.activeMask : message.activeMask;
    const waitingMask = message.waitingMask === undefined
      ? current.waitingMask : message.waitingMask;
    const localPlayers =
      message.localPlayers === undefined ? current.localPlayers : message.localPlayers;
    const selection =
      message.selection === undefined ? current.selection : message.selection;
    const selectionChanged = !sameBytes(selection, current.selection);
    const topologyChanged = activeMask !== current.activeMask;
    const activeSelectionChanged =
      selectionChanged && (activeMask !== 0 || current.activeMask !== 0);
    let ready = message.ready === undefined ? current.ready : message.ready;
    if (topologyChanged || activeSelectionChanged) ready = false;
    const quick = this.quickParty();
    if ((quick?.mode || this.quickMatch()) && (activeMask !== current.activeMask ||
        waitingMask !== current.waitingMask || localPlayers !== current.localPlayers)) {
      this.send(socket, encodeQuickRejected("Leave Quick Play before changing party seats.")); return;
    }
    const match = this.currentMatch();
    if (
      !isBoundedUtf8(name, 1, MAX_NAME_LENGTH) ||
      !validSeatMasks(localPlayers, activeMask, waitingMask) ||
      selection.byteLength > MAX_SELECTION_BYTES ||
      (ready && (activeMask === 0 || match.payload === null))
    ) {
      this.error(socket, "Invalid participant update");
      return;
    }
    let occupied = 0;
    for (const connected of this.ctx.getWebSockets()) {
      if (connected === socket || connected.readyState !== WebSocket.OPEN) continue;
      occupied += countActive(this.attachment(connected)?.activeMask ?? 0);
    }
    if (occupied + countActive(activeMask) > MAX_ACTIVE_PLAYERS) {
      this.error(socket, "Only four players can fight at once");
      return;
    }
    if (match.payload !== null && (topologyChanged || activeSelectionChanged)) {
      this.error(socket, "Active players are locked for this match");
      return;
    }
    if (topologyChanged || activeSelectionChanged) {
      this.clearReady();
    }
    const revision = this.nextRevision();
    socket.serializeAttachment({
      ...current,
      name,
      activeMask,
      waitingMask,
      localPlayers,
      ready,
      selection: selection.slice(),
      waitingOrder: waitingOrderForChange(current, waitingMask, revision),
    } satisfies ConnectionAttachment);
    this.broadcastPresence(revision);
  }

  private assignSeats(
    socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.AssignSeats }>,
  ): void {
    if (this.quickParty()?.mode || this.quickMatch()) {
      this.send(socket, encodeQuickRejected("Quick Play keeps its assigned players and rules.")); return;
    }

    const sender = this.attachment(socket);
    const owner = this.snapshot(this.currentRevision()).ownerPeerId;
    if (sender === null || sender.peerId !== owner) {
      this.error(socket, "Only the room owner may assign seats");
      return;
    }
    const target = isPeerId(message.peerId)
      ? this.ctx.getWebSockets(`peer:${message.peerId}`).find(
          (candidate) => candidate.readyState === WebSocket.OPEN,
        ) : undefined;
    const current = target !== undefined ? this.attachment(target) : null;
    if (target === undefined || current === null ||
        !validSeatMasks(current.localPlayers, message.activeMask,
                        message.waitingMask)) {
      this.error(socket, "Invalid seat assignment");
      return;
    }
    const activeChanged = message.activeMask !== current.activeMask;
    if (activeChanged && this.currentMatch().payload !== null) {
      this.error(socket, "Active players are locked for this match");
      return;
    }
    let occupied = 0;
    for (const connected of this.ctx.getWebSockets()) {
      if (connected === target || connected.readyState !== WebSocket.OPEN) continue;
      occupied += countActive(this.attachment(connected)?.activeMask ?? 0);
    }
    if (occupied + countActive(message.activeMask) > MAX_ACTIVE_PLAYERS) {
      this.error(socket, "Only four players can fight at once");
      return;
    }
    if (activeChanged) this.clearReady();
    const revision = this.nextRevision();
    target.serializeAttachment({
      ...current,
      activeMask: message.activeMask,
      waitingMask: message.waitingMask,
      ready: activeChanged ? false : current.ready,
      waitingOrder: waitingOrderForChange(current, message.waitingMask,
                                          revision),
    } satisfies ConnectionAttachment);
    this.broadcastPresence(revision);
  }

  private setMatch(socket: WebSocket, payload: Uint8Array): void {
    if (this.quickMatch()) {
      this.send(socket, encodeQuickRejected("Quick Play starts through its ready countdown.")); return;
    }
    const sender = this.attachment(socket);
    const owner = this.snapshot(this.currentRevision()).ownerPeerId;
    if (
      sender === null ||
      sender.peerId !== owner ||
      payload.byteLength > MAX_MATCH_BYTES
    ) {
      this.error(socket, "Only the room owner may set a bounded match proposal");
      return;
    }
    if (payload.byteLength === 0 && this.currentMatch().payload === null) return;
    const stored = payload.byteLength === 0
      ? null
      : payload.slice().buffer;
    this.ctx.storage.sql.exec(
      "UPDATE room_match SET epoch = epoch + 1, payload = ? WHERE id = 1",
      stored,
    );
    this.clearReady();
    this.broadcastPresence(this.nextRevision());
  }

  private setRotation(socket: WebSocket, policy: RotationPolicy): void {
    if (this.quickParty()?.mode || this.quickMatch()) {
      this.send(socket, encodeQuickRejected("Quick Play keeps its assigned players and rules.")); return;
    }

    const sender = this.attachment(socket);
    const owner = this.snapshot(this.currentRevision()).ownerPeerId;
    if (sender === null || sender.peerId !== owner ||
        policy < RotationPolicy.Manual || policy > RotationPolicy.WinnerOut ||
        this.currentMatch().payload !== null) {
      this.error(socket, "Only the room owner may change rotation between matches");
      return;
    }
    if (this.rotationPolicy() === policy) return;
    this.ctx.storage.sql.exec(
      "UPDATE room_policy SET rotation = ? WHERE id = 1", policy,
    );
    this.broadcastPresence(this.nextRevision());
  }

  private reportMatch(
    socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.ReportMatch }>,
  ): void {
    if (this.quickParty()?.mode || this.quickMatch()) {
      this.send(socket, encodeQuickRejected("Quick Play keeps its assigned players and rules.")); return;
    }

    const sender = this.attachment(socket);
    const match = this.currentMatch();
    if (sender === null || sender.activeMask === 0) {
      this.error(socket, "Only a fighting player may report the match result");
      return;
    }
    // A second client can finish a few frames later; its report must not
    // clear a newer proposal or rotate the queue twice.
    if (match.payload === null || match.epoch !== message.epoch) return;

    const active: { socket: WebSocket; seat: number; peerId: string }[] = [];
    let nextWaiting: { socket: WebSocket; seat: number; order: number } | null = null;
    for (const connected of this.ctx.getWebSockets()) {
      if (connected.readyState !== WebSocket.OPEN) continue;
      const participant = this.attachment(connected);
      if (participant === null) continue;
      for (let seat = 0; seat < participant.localPlayers; ++seat) {
        const bit = 1 << seat;
        if ((participant.activeMask & bit) !== 0) {
          active.push({ socket: connected, seat, peerId: participant.peerId });
        }
        if ((participant.waitingMask & bit) !== 0 &&
            (nextWaiting === null || participant.waitingOrder[seat] < nextWaiting.order)) {
          nextWaiting = { socket: connected, seat,
                          order: participant.waitingOrder[seat] };
        }
      }
    }
    let outgoing: { socket: WebSocket; seat: number; peerId: string } | null = null;
    const policy = this.rotationPolicy();
    if (message.eligible && policy !== RotationPolicy.Manual &&
        active.length === 2 && nextWaiting !== null &&
        isPeerId(message.winnerPeerId) && message.winnerSeat < 4) {
      const winner = active.find((entry) =>
        entry.peerId === message.winnerPeerId && entry.seat === message.winnerSeat);
      if (winner === undefined) {
        this.error(socket, "Winner is not a fighting player");
        return;
      }
      outgoing = policy === RotationPolicy.WinnerOut ? winner :
        active.find((entry) => entry !== winner) ?? null;
    }

    this.ctx.storage.sql.exec(
      "UPDATE room_match SET epoch = epoch + 1, payload = NULL WHERE id = 1",
    );
    this.clearReady();
    const revision = this.nextRevision();
    if (outgoing !== null && nextWaiting !== null) {
      const leaving = this.attachment(outgoing.socket)!;
      const leavingBit = 1 << outgoing.seat;
      outgoing.socket.serializeAttachment({
        ...leaving,
        activeMask: leaving.activeMask & ~leavingBit,
        waitingMask: leaving.waitingMask | leavingBit,
        waitingOrder: waitingOrderForChange(
          leaving, leaving.waitingMask | leavingBit, revision,
        ),
      } satisfies ConnectionAttachment);
      const entering = this.attachment(nextWaiting.socket)!;
      const enteringBit = 1 << nextWaiting.seat;
      nextWaiting.socket.serializeAttachment({
        ...entering,
        activeMask: entering.activeMask | enteringBit,
        waitingMask: entering.waitingMask & ~enteringBit,
        waitingOrder: waitingOrderForChange(
          entering, entering.waitingMask & ~enteringBit, revision,
        ),
      } satisfies ConnectionAttachment);
    }
    this.broadcastPresence(revision);
  }

  private forwardSignal(
    socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.Signal }>,
  ): void {
    const sender = this.attachment(socket);
    if (sender === null || !isPeerId(message.to)) {
      this.error(socket, "Invalid signal target");
      return;
    }
    const targets = this.ctx.getWebSockets(`peer:${message.to}`);
    if (targets.length === 0) {
      this.error(socket, "Signal target is not connected");
      return;
    }
    let forwarded: ArrayBuffer;
    try {
      forwarded = encodeSignal(sender.peerId, message.payload, MAX_MESSAGE_BYTES);
    } catch {
      this.error(socket, "Invalid signal payload");
      return;
    }
    for (const target of targets) {
      this.send(target, forwarded);
    }
    this.send(socket, encodeSignalSent(message.to, MAX_MESSAGE_BYTES));
  }

  private currentRevision(): number {
    return this.ctx.storage.sql
      .exec<{ revision: number }>("SELECT revision FROM room_counter WHERE id = 1")
      .one().revision;
  }

  private nextRevision(): number {
    return this.ctx.storage.sql
      .exec<{ revision: number }>(`
        UPDATE room_counter SET revision = revision + 1 WHERE id = 1
        RETURNING revision
      `)
      .one().revision;
  }

  private currentMatch(): { epoch: number; payload: Uint8Array | null } {
    const row = this.ctx.storage.sql
      .exec<RoomMatchRow>("SELECT epoch, payload FROM room_match WHERE id = 1")
      .one();
    return {
      epoch: row.epoch,
      payload: row.payload === null ? null : new Uint8Array(row.payload),
    };
  }

  private rotationPolicy(): RotationPolicy {
    return this.ctx.storage.sql
      .exec<RoomPolicyRow>("SELECT rotation FROM room_policy WHERE id = 1")
      .one().rotation as RotationPolicy;
  }

  private clearReady(excluded?: WebSocket): void {
    for (const socket of this.ctx.getWebSockets()) {
      if (socket === excluded) continue;
      const attachment = this.attachment(socket);
      if (attachment !== null && attachment.ready) {
        socket.serializeAttachment({
          ...attachment,
          ready: false,
        } satisfies ConnectionAttachment);
      }
    }
  }

  private invalidateMatch(excluded?: WebSocket): void {
    this.ctx.storage.sql.exec(
      "UPDATE room_match SET epoch = epoch + 1, payload = NULL WHERE id = 1",
    );
    this.clearReady(excluded);
  }

  private removeParticipant(socket: WebSocket): void {
    const departing = this.attachment(socket);
    const replacement = departing !== null &&
      this.ctx.getWebSockets(`peer:${departing.peerId}`).some(
        (candidate) => candidate !== socket && candidate.readyState === WebSocket.OPEN,
      );
    if (!replacement) this.invalidateQuickSearch();
    if (!replacement && departing?.activeMask !== 0 && !this.quickMatch()) {
      this.invalidateMatch(socket);
    }
    if (!this.ctx.getWebSockets().some((other) => other !== socket && other.readyState === WebSocket.OPEN))
      this.ctx.storage.sql.exec("DELETE FROM room_chat");
    this.broadcastPresence(this.nextRevision(), socket);
  }

  private attachment(socket: WebSocket): ConnectionAttachment | null {
    try {
      const attachment = socket.deserializeAttachment() as ConnectionAttachment;
      if (!validSeatMasks(attachment.localPlayers, attachment.activeMask,
                          attachment.waitingMask) ||
          !Array.isArray(attachment.waitingOrder) ||
          attachment.waitingOrder.length !== 4 ||
          attachment.waitingOrder.some((order, seat) =>
            !Number.isSafeInteger(order) ||
            ((attachment.waitingMask & (1 << seat)) !== 0) !== (order > 0))) {
        socket.close(4002, "Room protocol updated");
        return null;
      }
      return attachment;
    } catch {
      return null;
    }
  }

  private snapshot(revision: number, excluded?: WebSocket): RoomSnapshot {
    const participants: Participant[] = [];
    const waiting: { participant: Participant; seat: number; order: number }[] = [];
    for (const socket of this.ctx.getWebSockets()) {
      if (socket === excluded || socket.readyState !== WebSocket.OPEN) {
        continue;
      }
      const attachment = this.attachment(socket);
      if (attachment !== null) {
        const participant = publicParticipant(attachment);
        participants.push(participant);
        for (let seat = 0; seat < 4; ++seat) {
          if ((attachment.waitingMask & (1 << seat)) !== 0) {
            waiting.push({ participant, seat,
                           order: attachment.waitingOrder[seat] });
          }
        }
      }
    }
    participants.sort((left, right) => left.joinOrder - right.joinOrder);
    waiting.sort((left, right) => left.order - right.order);
    for (let index = 0; index < waiting.length; ++index) {
      waiting[index].participant.waitingRanks[waiting[index].seat] = index + 1;
    }
    const match = this.currentMatch();
    return {
      type: "presence",
      revision,
      ownerPeerId: participants[0]?.peerId ?? null,
      participants,
      matchEpoch: match.epoch,
      match: match.payload,
      rotationPolicy: this.rotationPolicy(),
    };
  }

  private broadcastPresence(revision: number, excluded?: WebSocket): void {
    const snapshot = encodePresence(this.snapshot(revision, excluded), MAX_MESSAGE_BYTES);
    for (const socket of this.ctx.getWebSockets()) {
      if (socket !== excluded && socket.readyState === WebSocket.OPEN) {
        this.send(socket, snapshot);
      }
    }
  }

  private error(socket: WebSocket, message: string): void {
    this.send(socket, encodeError(message, MAX_MESSAGE_BYTES));
  }

  private send(socket: WebSocket, message: ArrayBuffer): void {
    try {
      socket.send(message);
    } catch (error) {
      console.error(
        JSON.stringify({
          message: "websocket send failed",
          error: error instanceof Error ? error.message : String(error),
        }),
      );
    }
  }
}
