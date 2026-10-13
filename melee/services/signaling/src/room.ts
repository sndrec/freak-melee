import { DurableObject } from "cloudflare:workers";

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
} from "./wire";

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
    return new Response(null, { status: 101, webSocket: client });
  }

  webSocketMessage(socket: WebSocket, message: string | ArrayBuffer): void {
    if (typeof message === "string" || message.byteLength > MAX_MESSAGE_BYTES) {
      this.error(socket, "Messages must use the bounded binary room protocol");
      return;
    }

    let parsed: ClientMessage;
    try {
      parsed = decodeClientMessage(message);
    } catch {
      this.error(socket, "Malformed room message");
      return;
    }

    switch (parsed.type) {
    case ClientMessageType.Update:
      this.updateParticipant(socket, parsed);
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

  webSocketClose(socket: WebSocket): void {
    this.removeParticipant(socket);
  }

  webSocketError(socket: WebSocket): void {
    this.removeParticipant(socket);
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
    if (!replacement && departing?.activeMask !== 0) {
      this.invalidateMatch(socket);
    }
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
