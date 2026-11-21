import { lockedChoices, selectedSeats, unreadySelection } from "./lobby-selection";
import { DurableObject } from "cloudflare:workers";
import type { LobbyEntry } from "./lobby-directory";
import { HEARTBEAT_MS, CONNECTION_LIFETIME_MS, CONNECTION_TIMEOUT_MS, connectionExpired, connectionUncertain, playerIdentity } from "./connection-lease";
import { connectionAccount, currentConnectionAccount, type ConnectionAccount } from "./connection-account";
import { equalHash } from "./account-protocol";
import { ownsPartySeat, pruneRoomAccounts, type StoredQuickRoster } from "./room-ownership";
import { encodeListing, validLobbyPassword } from "./wire";
import { lobbyStageChooser, lobbyStagePlayers, automaticLobbyStage, emptyLobbyStageHistory,
  lobbyReadySignature, LOBBY_STAGES, type LobbyStageHistory } from "./lobby-stage";
import { LOBBY_COUNTDOWN_MS, encodeLobbyRules, decodeLobbyRules, type LobbyRules } from "./lobby-rules";
import { encodeLobbyState, encodeStageCursorUpdate, encodeCssPointersUpdate,
  type CssPointers } from "./wire";
import { assignOpenCards, occupiedCards, validCardSlots, NO_CARD,
  type CardSlots } from "./room-cards";
import { buildQuickMatch, quickRosterComplete, QUICK_COUNTDOWN_MS } from "./quick-match";
import { encodeMatchProposal, MATCH_PEER_COUNT_OFFSET } from "./match-proposal";
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

interface RoomStageRow extends Record<string, SqlStorageValue> {
  loser_peer: string;
  loser_seat: number;
  stage: number;
}
interface LobbyFlowRow extends Record<string, SqlStorageValue> {
  rules: ArrayBuffer | null; phase: number; deadline: number; revision: number;
  signature: string; history: string; next_history: string;
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
  private lastDirectoryUpdate?: { payload: string; issued: number };
  private controlMessages = new Map<string, { tail?: Promise<void>; pending: number }>();
  private pendingConnections = new Map<string, boolean>();
  private changingMembership = new Set<string>();
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
        CREATE TABLE IF NOT EXISTS room_accounts (peer TEXT PRIMARY KEY, account_id TEXT NOT NULL);
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
        CREATE TABLE IF NOT EXISTS room_stage (
          id INTEGER PRIMARY KEY CHECK(id=1), loser_peer TEXT NOT NULL,
          loser_seat INTEGER NOT NULL, stage INTEGER NOT NULL
        );
        INSERT OR IGNORE INTO room_stage VALUES (1,'',255,255);
        CREATE TABLE IF NOT EXISTS lobby_flow (
          id INTEGER PRIMARY KEY CHECK(id=1), rules BLOB, phase INTEGER NOT NULL,
          deadline INTEGER NOT NULL, revision INTEGER NOT NULL, signature TEXT NOT NULL,
          history TEXT NOT NULL, next_history TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS room_listing (
          id INTEGER PRIMARY KEY CHECK(id=1), title TEXT NOT NULL, listed INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS room_capacity (
          id INTEGER PRIMARY KEY CHECK(id=1), fighters INTEGER NOT NULL CHECK(fighters BETWEEN 2 AND 4)
        );
        CREATE TABLE IF NOT EXISTS room_password (
          id INTEGER PRIMARY KEY CHECK(id=1), salt BLOB NOT NULL, verifier BLOB NOT NULL
        );
        CREATE TABLE IF NOT EXISTS room_recovery (
          connection TEXT PRIMARY KEY, expires INTEGER NOT NULL, attachment TEXT NOT NULL
        );
      `);
      const history = JSON.stringify(emptyLobbyStageHistory());
      this.ctx.storage.sql.exec("INSERT OR IGNORE INTO lobby_flow VALUES (1,NULL,0,0,0,'',?,?)", history, history);
    });
  }

  async fetch(request: Request): Promise<Response> {
    if (request.method !== "GET" || request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("Expected a WebSocket upgrade", { status: 426 });
    }

    const url = new URL(request.url);
    const peerId = url.searchParams.get("peer") ?? "";
    const account = connectionAccount(request);
    const identity = await playerIdentity(request, account?.profile.id);
    if (!identity) return new Response("Missing guest installation key", { status: 400 });
    const lock = this.password();
    if (lock) {
      const password = request.headers.get("X-Freak-Lobby-Password");
      const reconnecting = this.ctx.getWebSockets().some(socket => this.attachment(socket)?.playerIdentity === identity)
        || this.recovering().some(peer => peer.playerIdentity === identity);
      if (!reconnecting && (!password || !validLobbyPassword(password) ||
          !crypto.subtle.timingSafeEqual(lock.verifier, await this.passwordVerifier(password, lock.salt)) ||
          !crypto.subtle.timingSafeEqual(this.password()?.salt ?? new Uint8Array(16), lock.salt))) {
        return new Response("Lobby password required", { status: 423 });
      }
    }
    const name = account?.profile.name ?? url.searchParams.get("name") ?? "";
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

    pruneRoomAccounts(this.ctx, this.changingMembership);
    const boundAccount = this.boundAccount(peerId);
    if (boundAccount !== undefined && boundAccount !== account?.profile.id)
      return new Response("This connection belongs to another account", { status: 403 });
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
    if (!current.participants.length && !this.recovering().length && this.currentMatch().payload !== null) {
      this.invalidateMatch();
      this.clearEmptyLobby();
    }
    const replacement = current.participants.find((peer) => peer.peerId === peerId);
    const reconnectId = request.headers.get("X-Freak-Connection") ?? "";
    const returning = quick?.state === 3 && ownsPartySeat(JSON.parse(quick.roster) as StoredQuickRoster,
      peerId, localPlayers, build, identity, reconnectId, boundAccount);
    if (quick?.mode && !replacement && !returning) {
      const occupied = current.participants.reduce((sum, peer) => sum + peer.localPlayers, 0);
      if (quick.state !== 0 || localPlayers === 0 || occupied + localPlayers > quick.mode ||
          current.participants.some((peer) => peer.build !== build))
        return new Response("Quick Play party is unavailable or full", { status: 409 });
    }
    const previousConnections = this.ctx.getWebSockets().filter(socket => {
      const peer = this.attachment(socket);
      return socket.readyState === WebSocket.OPEN && !peer?.superseded &&
        (peer?.peerId === peerId || peer?.playerIdentity === identity);
    });
    // A public peer ID cannot prove ownership of an active guest connection.
    // Guests sign in through attachAccount using their private Welcome ID.
    if (previousConnections.some((socket) => {
      const previous = this.attachment(socket);
      return socket.readyState === WebSocket.OPEN && previous?.playerIdentity !== identity && !previous?.account &&
        (!previous || reconnectId.length !== previous.connectionId.length ||
          !equalHash(reconnectId, previous.connectionId));
    }))
      return new Response("This guest connection is already active", { status: 409 });
    const reservations = this.recovering();
    if (reservations.some(peer => peer.peerId === peerId && peer.playerIdentity !== identity))
      return new Response("This player is reconnecting", { status: 409 });
    if (
      previousConnections.length === 0 &&
      current.participants.length >= MAX_PARTICIPANTS &&
      !reservations.some(peer => peer.playerIdentity === identity)
    ) {
      return new Response("Room is full", { status: 409 });
    }
    let occupied = 0;
    for (const connected of this.ctx.getWebSockets()) {
      if (previousConnections.includes(connected) || connected.readyState !== WebSocket.OPEN) continue;
      occupied += countActive(this.attachment(connected)?.activeMask ?? 0);
    }
    for (const reserved of reservations)
      if (reserved.playerIdentity !== identity) occupied += countActive(reserved.activeMask);
    const recovery = reservations.find(peer => peer.playerIdentity === identity);
    const previous = previousConnections.length !== 0
      ? this.attachment(previousConnections[0]) : recovery ?? null;
    const free = Math.max(0, this.fighterLimit() - occupied);
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
        (this.currentMatch().payload !== null && activeMask !== (previous?.activeMask ?? 0))) {
      return Response.json({ error: "Invalid local player assignment" }, { status: 400 });
    }

    if ((quick?.mode || admission) && (localPlayers === 0 || activeMask !== (1 << localPlayers) - 1 || waitingMask !== 0))
      return new Response("Quick Play uses active human seats", { status: 400 });

    if (this.changingMembership.has(peerId) || this.changingMembership.size >= MAX_PARTICIPANTS)
      return new Response("Lobby sign-in is already in progress; please try again", { status: 409 });
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
      playerIdentity: identity, lastSeenAt: Date.now(), lastAuthorityCheckAt: Date.now(), uncertain: false,
      account,
      connectionId: crypto.randomUUID(),
      peerId,
      name,
      build,
      localPlayers,
      activeMask,
      cardSlots: [NO_CARD, NO_CARD, NO_CARD, NO_CARD],
      waitingMask,
      ready: false,
      selection: previous ? unreadySelection(previous.selection) : new Uint8Array(),
      joinOrder: previous?.joinOrder ?? counters.join_order,
      waitingOrder: waitingOrderForChange(
        previous?.localPlayers === localPlayers ? previous : null,
        waitingMask, counters.revision,
      ),
      lastPlayed: previous?.lastPlayed ?? [0, 0, 0, 0],
    };

    this.pendingConnections.set(attachment.connectionId, false);
    this.changingMembership.add(peerId);
    let admitted: boolean;
    try {
      admitted = await this.env.PLAYER_SESSIONS.getByName(identity).enter(this.ctx.id.toString(), attachment.connectionId);
      admitted &&= !this.pendingConnections.get(attachment.connectionId);
    } finally {
      this.pendingConnections.delete(attachment.connectionId);
      this.changingMembership.delete(peerId);
    }
    if (!admitted) return new Response("A newer lobby connection replaced this request", { status: 409 });
    const displaced = this.ctx.getWebSockets().filter(socket => socket.readyState === WebSocket.OPEN &&
      this.attachment(socket)?.playerIdentity === identity);
    for (const socket of displaced) attachment.joinOrder = Math.min(attachment.joinOrder, this.attachment(socket)!.joinOrder);
    // Membership claims await another object; recheck seats after that yield.
    const others = this.ctx.getWebSockets().filter(socket => socket.readyState === WebSocket.OPEN)
      .map(socket => this.attachment(socket)).filter((peer): peer is ConnectionAttachment =>
        peer !== null && !peer.superseded && peer.playerIdentity !== identity);
    others.push(...this.recovering().filter(peer => peer.playerIdentity !== identity &&
      !others.some(live => live.playerIdentity === peer.playerIdentity)));
    const cards = assignOpenCards(activeMask, occupiedCards(others), this.fighterLimit(), previous?.cardSlots);
    if (others.length >= MAX_PARTICIPANTS || !cards ||
        others.some(peer => peer.peerId === peerId && peer.connectionId !== previous?.connectionId) ||
        (activeMask !== (previous?.activeMask ?? 0) && this.currentMatch().payload !== null)) {
      await this.env.PLAYER_SESSIONS.getByName(identity).leave(attachment.connectionId);
      return new Response("Lobby seats changed while joining; please try again", { status: 409 });
    }
    attachment.cardSlots = cards;
    if (recovery) this.ctx.storage.sql.exec("DELETE FROM room_recovery WHERE connection=?", recovery.connectionId);

    const pair = new WebSocketPair();
    const client = pair[0];
    const server = pair[1];
    if (account) this.ctx.storage.sql.exec("INSERT OR IGNORE INTO room_accounts VALUES (?,?)", peerId, account.profile.id);
    server.serializeAttachment(attachment);
    this.ctx.acceptWebSocket(server, [`peer:${peerId}`]);
    if (previous && previous.peerId !== peerId && !this.quickMatch()) {
      this.invalidateMatch();
      this.resetLobbyReadiness();
    }
    for (const oldSocket of new Set([...previousConnections, ...displaced])) {
      const old = this.attachment(oldSocket);
      if (old) this.ctx.waitUntil(this.env.PLAYER_SESSIONS.getByName(old.playerIdentity).leave(old.connectionId));
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
    if (previous?.peerId === peerId && previous.lobbyResult) {
      this.reportMatch(server, { type: ClientMessageType.ReportMatch, ...previous.lobbyResult });
    }
    this.sendListing(server);
    await this.refreshQuickCountdown();
    await this.refreshLobbyCountdown();
    if (quick?.mode || admission) this.sendQuick(server);
    await this.scheduleAlarm();
    return new Response(null, { status: 101, webSocket: client });
  }

  private boundAccount(peerId: string): string | undefined {
    return this.ctx.storage.sql.exec<{ account_id: string }>(
      "SELECT account_id FROM room_accounts WHERE peer=?", peerId).toArray()[0]?.account_id;
  }

  async attachAccount(connectionId: string, account: ConnectionAccount): Promise<boolean> {
    const profile = await currentConnectionAccount(account, this.env);
    if (!profile) return false;
    // The random connection ID is delivered only in this socket's Welcome.
    // A public peer ID or another member's presence record cannot claim it.
    for (const socket of this.ctx.getWebSockets()) {
      if (socket.readyState !== WebSocket.OPEN) continue;
      const current = this.attachment(socket);
      if (!current || current.connectionId.length !== connectionId.length || !equalHash(current.connectionId, connectionId)) continue;
      const bound = this.boundAccount(current.peerId);
      if (current.superseded || this.changingMembership.size >= MAX_PARTICIPANTS || this.changingMembership.has(current.peerId) ||
          (bound !== undefined && bound !== profile.id) ||
          (current.account && current.account.profile.id !== profile.id)) return false;
      const identity = `account:${profile.id}`;
      const membership = this.env.PLAYER_SESSIONS.getByName(identity);
      this.changingMembership.add(current.peerId);
      try {
        const alreadyAttached = current.playerIdentity === identity;
        const owns = alreadyAttached ? await membership.owns(connectionId) :
          await membership.enter(this.ctx.id.toString(), connectionId);
        if (!owns) return false;
        const latest = this.attachment(socket);
        const latestBinding = this.boundAccount(current.peerId);
        if (socket.readyState !== WebSocket.OPEN || !latest || latest.superseded ||
            latest.connectionId !== connectionId || (latestBinding !== undefined && latestBinding !== profile.id)) {
          if (!alreadyAttached) await membership.leave(connectionId);
          return false;
        }
        this.ctx.storage.sql.exec("INSERT OR IGNORE INTO room_accounts VALUES (?,?)", latest.peerId, profile.id);
        socket.serializeAttachment({ ...latest, playerIdentity: identity, account: { ...account, profile },
          name: profile.name, lastAuthorityCheckAt: Date.now() } satisfies ConnectionAttachment);
        this.broadcastPresence(this.nextRevision());
        if (!alreadyAttached) await this.env.PLAYER_SESSIONS.getByName(latest.playerIdentity).leave(connectionId);
        return true;
      } finally { this.changingMembership.delete(current.peerId); }
    }
    return false;
  }

  async webSocketMessage(socket: WebSocket, message: string | ArrayBuffer): Promise<void> {
    const initial = this.attachment(socket);
    if (!initial || initial.superseded || socket.readyState !== WebSocket.OPEN) return;
    if (typeof message !== "string" && message.byteLength >= 8 && message.byteLength <= MAX_MESSAGE_BYTES) {
      const type = new Uint8Array(message, 5, 1)[0];
      if (type === ClientMessageType.CssPointers || type === ClientMessageType.StageCursor) {
        // Motion only affects drawing. Heartbeats and room actions still
        // refresh authentication and ownership through the regular path.
        const now = Date.now();
        if (initial.uncertain || now >= initial.lastAuthorityCheckAt + 2 * HEARTBEAT_MS ||
            (initial.account && now >= initial.account.expires)) return;
        // Allow six batched samples while bounding the sustained update rate.
        const budgetMs = Math.max(now - 80, initial.motionBudgetMs ?? 0) + 16;
        if (budgetMs > now + 16) return;
        initial.motionBudgetMs = budgetMs;
        let cursorMessage: ClientMessage;
        try { cursorMessage = decodeClientMessage(message); }
        catch { socket.serializeAttachment(initial); return; }
        if (cursorMessage.type === ClientMessageType.CssPointers) this.cssPointers(socket, initial, cursorMessage.pointers);
        else if (cursorMessage.type === ClientMessageType.StageCursor) this.stageCursor(initial, cursorMessage.cursor);
        socket.serializeAttachment(initial);
        return;
      }
    }
    // ICE candidates can arrive together when a room connects. Keep room
    // commands in order and bound outstanding work before account or room I/O.
    const now = Date.now(), interval = 20, burst = 128;
    const budget = Math.max(now - (burst - 1) * interval, initial.controlBudgetMs ?? 0) + interval;
    const queue = this.controlMessages.get(initial.connectionId) ?? { pending: 0 };
    if (budget > now + interval || queue.pending >= burst) {
      socket.close(1008, "Too many pending room commands"); return;
    }
    initial.controlBudgetMs = budget;
    socket.serializeAttachment(initial);
    if (typeof message === "string" ? message !== "ping" : message.byteLength > MAX_MESSAGE_BYTES) {
      this.error(socket, "Messages must use the bounded binary room protocol"); return;
    }
    const previous = queue.tail;
    let release!: () => void;
    queue.tail = new Promise<void>(resolve => { release = resolve; });
    ++queue.pending;
    this.controlMessages.set(initial.connectionId, queue);
    try {
      if (previous) await previous;
      if (socket.readyState !== WebSocket.OPEN) return;
      const identity = this.attachment(socket);
      if (!identity || identity.superseded) return;
      if (identity.account) {
        const profile = await currentConnectionAccount(identity.account, this.env);
        if (socket.readyState !== WebSocket.OPEN) return;
        const latest = this.attachment(socket);
        if (!latest?.account || latest.superseded || latest.account.session !== identity.account.session) return;
        if (!profile) { socket.close(1008, "Account sign-in expired"); return; }
        if (latest.name !== profile.name) {
          latest.name = profile.name;
          latest.account.profile = profile;
          socket.serializeAttachment(latest);
          this.broadcastPresence(this.nextRevision());
        }
      }
      if (typeof message === "string") {
        await this.touchConnection(socket);
        if (socket.readyState === WebSocket.OPEN) socket.send("pong");
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

      await this.touchConnection(socket);
      if (socket.readyState !== WebSocket.OPEN) return;

      switch (parsed.type) {
      case ClientMessageType.RequestLobbyState: {
        const peer = this.attachment(socket);
        if (!peer || this.quickParty()?.mode || this.quickMatch()) return;
        socket.serializeAttachment({ ...peer, lobbySubscribed: true } satisfies ConnectionAttachment);
        this.sendLobbyState(socket);
        await this.refreshLobbyCountdown();
        return;
      }
      case ClientMessageType.SetLobbyRules:
        this.setLobbyRules(socket, parsed.rules);
        await this.refreshLobbyCountdown();
        return;
      case ClientMessageType.SetListing:
        await this.setListing(socket, parsed.title, parsed.listed, parsed.capacity, parsed.password);
        return;
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
        await this.refreshLobbyCountdown();
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
      case ClientMessageType.StartLobbyMatch:
        this.startLobbyMatch(socket, parsed.epoch, parsed.digest);
        await this.refreshLobbyCountdown();
        return;
      case ClientMessageType.ChangeCard:
        this.changeCard(socket, parsed);
        await this.refreshLobbyCountdown();
        return;
      case ClientMessageType.SetRotation:
        this.setRotation(socket, parsed.policy);
        return;
      case ClientMessageType.ReportMatch:
        this.reportMatch(socket, parsed);
        await this.refreshLobbyCountdown();
        return;
      case ClientMessageType.PickLobbyStage:
        this.pickLobbyStage(socket, parsed);
        return;
      }
    } finally {
      release();
      if (!--queue.pending) this.controlMessages.delete(initial.connectionId);
    }
  }

  async webSocketClose(socket: WebSocket, code: number): Promise<void> {
    const peer = this.attachment(socket);
    if (peer?.superseded) return;
    const replacement = peer && this.ctx.getWebSockets().some(other => other !== socket &&
      other.readyState === WebSocket.OPEN && this.attachment(other)?.playerIdentity === peer.playerIdentity);
    if (!replacement && code !== 1000 && code !== 4001 && code !== 4006 && code !== 4008 && peer && !connectionExpired(peer)) {
      this.reserveConnection(peer);
      socket.serializeAttachment({ ...peer, superseded: true });
      this.broadcastPresence(this.nextRevision(), socket);
    } else {
      this.removeParticipant(socket);
    }
    await this.refreshQuickCountdown();
    await this.refreshLobbyCountdown();
    await this.scheduleAlarm();
    socket.close(1000, "Lobby connection closed");
  }
  async invitationFor(accountId: string): Promise<{ room: string; title: string; expires: number } | null> {
    const room = this.ctx.id.name, party = this.quickParty();
    if (!room || this.quickMatch() || (party?.mode && (party.mode !== 2 || party.state !== 0 ||
        this.snapshot(this.currentRevision()).participants.reduce((sum, peer) => sum + peer.localPlayers, 0) >= 2))) return null;
    for (const socket of this.ctx.getWebSockets()) {
      const peer = this.attachment(socket);
      if (socket.readyState !== WebSocket.OPEN || peer?.account?.profile.id !== accountId) continue;
      if (await currentConnectionAccount(peer.account, this.env) && socket.readyState === WebSocket.OPEN)
        return { room, title: this.listing()?.title ?? (party?.mode ? "Doubles party" : "Lobby"), expires: Date.now() + 15 * 60_000 };
    }
    return null;
  }

  async webSocketError(socket: WebSocket): Promise<void> {
    const peer = this.attachment(socket);
    if (peer?.superseded) return;
    if (peer && !connectionExpired(peer)) {
      this.reserveConnection(peer);
      socket.serializeAttachment({ ...peer, superseded: true });
    }
    else this.removeParticipant(socket);
    await this.refreshQuickCountdown();
    await this.refreshLobbyCountdown();
    socket.close(1011, "Lobby connection failed");
    await this.scheduleAlarm();
  }

  private lobbyFlow(): LobbyFlowRow {
    return this.ctx.storage.sql.exec<LobbyFlowRow>("SELECT * FROM lobby_flow WHERE id=1").one();
  }

  private sendLobbyState(target?: WebSocket): void {
    const flow = this.lobbyFlow();
    const message = encodeLobbyState({ revision: flow.revision, phase: flow.phase,
      remainingMs: Math.max(0, Math.min(LOBBY_COUNTDOWN_MS, flow.deadline-Date.now())),
      rules: flow.rules ? decodeLobbyRules(new Uint8Array(flow.rules)) : null });
    for (const socket of target ? [target] : this.ctx.getWebSockets())
      if (socket.readyState === WebSocket.OPEN && this.attachment(socket)?.lobbySubscribed) this.send(socket, message);
  }

  private setLobbyRules(socket: WebSocket, rules: LobbyRules): void {
    const peer = this.attachment(socket), room = this.snapshot(this.currentRevision());
    if (!peer?.lobbySubscribed || peer.peerId !== room.ownerPeerId ||
        this.quickParty()?.mode || this.quickMatch() || room.match !== null) {
      this.error(socket, "Only the lobby host may change rules between matches"); return;
    }
    const bytes = encodeLobbyRules(rules), flow = this.lobbyFlow();
    if (flow.rules && sameBytes(new Uint8Array(flow.rules), bytes)) return;
    this.ctx.storage.sql.exec("UPDATE lobby_flow SET rules=?, phase=0, deadline=0, signature='', revision=revision+1 WHERE id=1", bytes);
    this.ctx.storage.sql.exec("UPDATE room_stage SET stage=255 WHERE id=1");
    this.clearReady();
    this.broadcastPresence(this.nextRevision());
    this.sendLobbyState();
  }

  private async refreshLobbyCountdown(): Promise<void> {
    if (this.quickParty()?.mode || this.quickMatch() || this.currentMatch().payload !== null) return;
    const flow = this.lobbyFlow(), room = this.snapshot(this.currentRevision());
    const host = this.ctx.getWebSockets().find((socket) => socket.readyState === WebSocket.OPEN &&
      this.attachment(socket)?.peerId === room.ownerPeerId);
    const rules = flow.rules ? decodeLobbyRules(new Uint8Array(flow.rules)) : null;
    const signature = !this.hasUncertainConnections() && host && this.attachment(host)?.lobbySubscribed && rules
      ? lobbyReadySignature(room.participants, rules) : null;
    if (!signature) {
      if (flow.phase || flow.signature) {
        this.ctx.storage.sql.exec("UPDATE lobby_flow SET phase=0, deadline=0, signature='', revision=revision+1 WHERE id=1");
        this.ctx.storage.sql.exec("UPDATE room_stage SET stage=255 WHERE id=1");
        this.broadcastPresence(this.nextRevision()); this.sendLobbyState();
      }
      return;
    }
    if (flow.signature !== signature) {
      const deadline = Date.now()+LOBBY_COUNTDOWN_MS;
      this.ctx.storage.sql.exec("UPDATE lobby_flow SET phase=1, deadline=?, signature=?, revision=revision+1 WHERE id=1", deadline, signature);
      this.ctx.storage.sql.exec("UPDATE room_stage SET stage=255 WHERE id=1");
      await this.scheduleAlarm();
      this.broadcastPresence(this.nextRevision()); this.sendLobbyState();
    } else if (flow.phase === 1 && flow.deadline <= Date.now()) {
      const history = JSON.parse(flow.history) as LobbyStageHistory;
      const stage = automaticLobbyStage(rules!, history, crypto.getRandomValues(new Uint32Array(1))[0]);
      const loser = this.ctx.storage.sql.exec<RoomStageRow>("SELECT * FROM room_stage WHERE id=1").one();
      const chooser = lobbyStageChooser(room.participants, { peerId: loser.loser_peer, seat: loser.loser_seat }, rules!.stagePolicy, history.turn);
      if (rules!.stagePolicy === 3 && chooser) history.turn = chooser;
      // Picks carry the match epoch. Reopening stage select must invalidate
      // selections still in flight from an earlier countdown.
      this.ctx.storage.sql.exec("UPDATE room_match SET epoch=epoch+1 WHERE id=1");
      this.ctx.storage.sql.exec("UPDATE lobby_flow SET phase=2, deadline=0, next_history=?, revision=revision+1 WHERE id=1", JSON.stringify(history));
      this.ctx.storage.sql.exec("UPDATE room_stage SET stage=? WHERE id=1", stage);
      this.broadcastPresence(this.nextRevision()); this.sendLobbyState();
    } else if (flow.phase === 1) {
      await this.scheduleAlarm();
    }
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
    if (this.hasUncertainConnections() || !quickRosterComplete(allocation, people)) return null;
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
        await this.scheduleAlarm();
        this.sendQuick();
      }
      return;
    }
    if (flow.phase === 1 && flow.revision === prepared.revision) return;
    const deadline = Date.now() + QUICK_COUNTDOWN_MS;
    this.ctx.storage.sql.exec("UPDATE quick_flow SET phase=1, deadline=?, revision=? WHERE id=1", deadline, prepared.revision);
    await this.scheduleAlarm();
    this.sendQuick();
  }

  private async advanceCountdown(): Promise<void> {
    if (!this.quickMatch()) { await this.refreshLobbyCountdown(); return; }
    const flow = this.quickFlow();
    if (flow.phase !== 1) return;
    const prepared = this.preparedQuickMatch();
    if (!prepared || prepared.revision !== flow.revision) {
      await this.refreshQuickCountdown();
      return;
    }
    if (Date.now() < flow.deadline) {
      await this.scheduleAlarm();
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

  private recovering(): ConnectionAttachment[] {
    return this.ctx.storage.sql.exec<{ attachment: string }>(
      "SELECT attachment FROM room_recovery WHERE expires>?", Date.now()).toArray().map(row => {
      const peer = JSON.parse(row.attachment) as ConnectionAttachment;
      peer.selection = new Uint8Array(peer.selection);
      return peer;
    }).filter(peer => validCardSlots(peer.cardSlots, peer.activeMask));
  }

  private reserveConnection(peer: ConnectionAttachment): void {
    const saved = { ...peer, ready: false, uncertain: true, selection: [...unreadySelection(peer.selection)] };
    this.ctx.storage.sql.exec("INSERT OR REPLACE INTO room_recovery VALUES (?,?,?)",
      peer.connectionId, peer.lastSeenAt + CONNECTION_LIFETIME_MS, JSON.stringify(saved));
  }

  private hasUncertainConnections(): boolean {
    return this.recovering().length !== 0 || this.ctx.getWebSockets().some(socket =>
      socket.readyState === WebSocket.OPEN && this.attachment(socket)?.uncertain);
  }

  private clearEmptyLobby(): void {
    if (this.changingMembership.size || this.recovering().length ||
        this.ctx.getWebSockets().some(socket => socket.readyState === WebSocket.OPEN &&
          !this.attachment(socket)?.superseded)) return;
    this.ctx.storage.sql.exec("DELETE FROM room_chat");
    this.ctx.storage.sql.exec("UPDATE room_listing SET listed=0 WHERE id=1");
    this.ctx.storage.sql.exec("DELETE FROM room_capacity");
    this.ctx.storage.sql.exec("DELETE FROM room_password");
    this.ctx.storage.sql.exec("UPDATE room_stage SET loser_peer='', loser_seat=255, stage=255 WHERE id=1");
    const history = JSON.stringify(emptyLobbyStageHistory());
    this.ctx.storage.sql.exec("UPDATE lobby_flow SET rules=NULL, phase=0, deadline=0, signature='', revision=revision+1, history=?, next_history=? WHERE id=1", history, history);
    pruneRoomAccounts(this.ctx, this.changingMembership);
  }

  async evictConnection(connectionId: string, preserveRoom = false): Promise<void> {
    if (this.pendingConnections.has(connectionId)) this.pendingConnections.set(connectionId, true);
    const reserved = this.ctx.storage.sql.exec<{ active: number }>(
      "DELETE FROM room_recovery WHERE connection=? RETURNING json_extract(attachment,'$.activeMask') AS active", connectionId).toArray()[0];
    if (!preserveRoom && reserved?.active && !this.quickMatch()) {
      this.invalidateMatch();
      this.resetLobbyReadiness();
    }
    for (const socket of this.ctx.getWebSockets()) {
      if (this.attachment(socket)?.connectionId !== connectionId) continue;
      if (preserveRoom) socket.serializeAttachment({ ...this.attachment(socket)!, superseded: true });
      socket.close(4001, "Player moved to another lobby");
      if (!preserveRoom) this.removeParticipant(socket);
    }
    if (preserveRoom || reserved) pruneRoomAccounts(this.ctx, this.changingMembership);
    if (preserveRoom) return;
    if (!this.snapshot(this.currentRevision()).participants.length) this.clearEmptyLobby();
    this.broadcastPresence(this.nextRevision());
    await this.refreshQuickCountdown();
    await this.refreshLobbyCountdown();
    await this.scheduleAlarm();
  }

  private async touchConnection(socket: WebSocket): Promise<void> {
    const peer = this.attachment(socket);
    if (!peer) return;
    const checkAuthority = peer.uncertain || Date.now() - peer.lastAuthorityCheckAt >= HEARTBEAT_MS;
    const owns = !checkAuthority || await this.env.PLAYER_SESSIONS.getByName(peer.playerIdentity).owns(peer.connectionId);
    if (socket.readyState !== WebSocket.OPEN) return;
    const latest = this.attachment(socket);
    if (!latest || latest.superseded || latest.playerIdentity !== peer.playerIdentity) return;
    if (!owns) {
      await this.evictConnection(peer.connectionId);
      return;
    }
    socket.serializeAttachment({ ...latest, lastSeenAt: Date.now(), uncertain: false,
      lastAuthorityCheckAt: checkAuthority ? Date.now() : latest.lastAuthorityCheckAt });
    if (latest.uncertain) {
      this.broadcastPresence(this.nextRevision());
      await this.refreshQuickCountdown();
      await this.refreshLobbyCountdown();
    } else if (checkAuthority) {
      this.renewDirectoryListing();
    }
  }

  private async scheduleAlarm(): Promise<void> {
    const deadlines: number[] = [];
    if (this.ctx.getWebSockets().some(socket => socket.readyState === WebSocket.OPEN) || this.recovering().length)
      deadlines.push(Date.now() + HEARTBEAT_MS);
    const quick = this.quickFlow(), lobby = this.lobbyFlow();
    if (this.quickMatch() && quick.phase === 1) deadlines.push(quick.deadline);
    if (!this.quickMatch() && !this.quickParty()?.mode && lobby.phase === 1) deadlines.push(lobby.deadline);
    if (deadlines.length) await this.ctx.storage.setAlarm(Math.max(Date.now() + 1, Math.min(...deadlines)));
    else await this.ctx.storage.deleteAlarm();
  }

  async alarm(): Promise<void> {
    let changed = false;
    for (const socket of this.ctx.getWebSockets()) {
      if (socket.readyState !== WebSocket.OPEN) continue;
      const peer = this.attachment(socket);
      if (!peer) continue;
      if (connectionExpired(peer)) {
        socket.close(4008, "Heartbeat timed out");
        this.removeParticipant(socket);
        changed = true;
      } else if (peer.uncertain !== connectionUncertain(peer)) {
        socket.serializeAttachment({ ...peer, uncertain: connectionUncertain(peer), ready: false,
          selection: unreadySelection(peer.selection) });
        changed = true;
      }
    }
    const expired = this.ctx.storage.sql.exec<{ connection: string; attachment: string }>(
      "DELETE FROM room_recovery WHERE expires<=? RETURNING connection, attachment", Date.now()).toArray();
    const expiredPeers = expired.map(row => JSON.parse(row.attachment) as ConnectionAttachment);
    if (expired.length) pruneRoomAccounts(this.ctx, this.changingMembership);
    if (expiredPeers.some(peer => peer.activeMask) && !this.quickMatch()) {
      this.invalidateMatch();
      this.resetLobbyReadiness();
    }
    for (const peer of expiredPeers) {
      await this.env.PLAYER_SESSIONS.getByName(peer.playerIdentity).leave(peer.connectionId);
      changed = true;
    }
    if (changed) {
      if (!this.snapshot(this.currentRevision()).participants.length) this.clearEmptyLobby();
      this.broadcastPresence(this.nextRevision());
      await this.refreshQuickCountdown();
      await this.refreshLobbyCountdown();
    } else {
      this.renewDirectoryListing();
    }
    await this.advanceCountdown();
    await this.scheduleAlarm();
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
    const pending = this.stageVotes()[0];
    if (stage !== 255 && pending && (pending.peer !== peer.peerId || pending.seat !== seat) &&
        pending.stage !== stage) {
      this.send(socket, encodeQuickRejected("Your teammate picked another stage. Confirm that stage or wait for a new selection."));
      return;
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
  private quickRoster(mode: number, owners?: string[]): QuickOffer | null {
    const connected: ConnectionAttachment[] = [];
    let build = "";
    for (const socket of this.ctx.getWebSockets()) {
      if (socket.readyState !== WebSocket.OPEN) continue;
      const p = this.attachment(socket);
      if (p?.superseded) continue;
      if (!p || p.localPlayers < 1 || p.activeMask !== (1 << p.localPlayers) - 1 || p.waitingMask ||
          (build && build !== p.build)) return null;
      build = p.build;
      connected.push(p);
    }
    connected.sort((a, b) => a.joinOrder - b.joinOrder);
    if (owners) owners.push(...connected.map(peer => peer.playerIdentity));
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
    if (mode !== (prior?.mode ?? 0) && this.listing()?.listed) this.broadcastPresence(this.nextRevision());
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
    const owners: string[] = [];
    const offer = this.quickRoster(row.mode, owners);
    if (!offer) { this.send(socket, encodeQuickRejected("The party is not ready to search.")); return; }
    if (row.state === 0) offer.generation++;
    this.ctx.storage.sql.exec("UPDATE quick_party SET generation=?, state=1, roster=?, allocation=NULL WHERE id=1",
      offer.generation, JSON.stringify({ ...offer, owners } satisfies StoredQuickRoster));
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
    const name = current.account?.profile.name ?? (message.name === undefined ? current.name : message.name);
    const activeMask = message.activeMask === undefined
      ? current.activeMask : message.activeMask;
    let waitingMask = message.waitingMask === undefined
      ? current.waitingMask : message.waitingMask;
    const localPlayers =
      message.localPlayers === undefined ? current.localPlayers : message.localPlayers;
    const selection =
      message.selection === undefined ? current.selection : message.selection;
    if (message.selection !== undefined && !this.quickParty()?.mode && !this.quickMatch()) {
      waitingMask &= this.rotationPolicy() === RotationPolicy.Manual ? 0 :
        selectedSeats(selection, localPlayers) & ~activeMask;
    }
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
    if (occupied + countActive(activeMask) > this.fighterLimit()) {
      this.error(socket, "All fighting seats are taken");
      return;
    }
    if (match.payload !== null && (topologyChanged || activeSelectionChanged)) {
      this.error(socket, "Active players are locked for this match");
      return;
    }
    // A spectating owner can cancel/restart stage selection too.
    if (!this.lobbyFlow().rules && match.payload === null && (topologyChanged || activeSelectionChanged ||
        (selectionChanged && current.peerId === this.snapshot(this.currentRevision()).ownerPeerId)))
      this.invalidateMatch();
    const cardSlots = activeMask === current.activeMask
      ? current.cardSlots : this.cardsFor(current, activeMask);
    if (!cardSlots) { this.error(socket, "All fighting seats are taken"); return; }
    const revision = this.nextRevision();
    socket.serializeAttachment({
      ...current,
      name,
      activeMask,
      cardSlots,
      waitingMask,
      localPlayers,
      ready,
      selection: selection.slice(),
      waitingOrder: waitingOrderForChange(current, waitingMask, revision),
    } satisfies ConnectionAttachment);
    this.broadcastPresence(revision);
  }

  private cardsFor(current: ConnectionAttachment, activeMask: number): CardSlots | null {
    const others = this.snapshot(this.currentRevision()).participants.filter(
      participant => participant.peerId !== current.peerId);
    return assignOpenCards(activeMask, occupiedCards(others),
      this.fighterLimit(), current.cardSlots);
  }

  private changeCard(socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.ChangeCard }>): void {
    const current = this.attachment(socket);
    if (!current || message.seat >= current.localPlayers ||
        this.quickParty()?.mode || this.quickMatch()) {
      this.send(socket, encodeError("This controller cannot claim a lobby card.", 164, true)); return;
    }
    if (this.currentMatch().payload !== null) {
      this.send(socket, encodeError("Active players are locked for this match", 164, true)); return;
    }
    const bit = 1 << message.seat;
    const owned = current.cardSlots[message.seat];
    if (message.card === NO_CARD ? owned !== message.expected || owned === NO_CARD
      : owned !== NO_CARD || message.card >= this.fighterLimit() ||
        (occupiedCards(this.snapshot(this.currentRevision()).participants) & (1 << message.card))) {
      this.send(socket, encodeError("That fighting card has changed", 164, true)); return;
    }
    const cardSlots = current.cardSlots.slice() as CardSlots;
    cardSlots[message.seat] = message.card;
    const revision = this.nextRevision();
    const activeMask = message.card === NO_CARD ? current.activeMask & ~bit : current.activeMask | bit;
    const waitingMask = current.waitingMask & ~bit;
    socket.serializeAttachment({ ...current, activeMask, cardSlots, waitingMask, ready: false,
      selection: unreadySelection(current.selection),
      waitingOrder: waitingOrderForChange(current, waitingMask, revision) } satisfies ConnectionAttachment);
    this.resetLobbyReadiness();
    this.broadcastPresence(revision);
  }

  private startLobbyMatch(socket: WebSocket, epoch: number, digest: Uint8Array): void {
    const sender = this.attachment(socket);
    const flow = this.lobbyFlow(), room = this.snapshot(this.currentRevision());
    if (!sender?.lobbySubscribed || sender.peerId !== room.ownerPeerId || !flow.rules ||
        this.quickMatch() || this.quickParty()?.mode || flow.phase !== 2 ||
        room.matchEpoch !== epoch || room.match !== null || room.stage === 255 || this.hasUncertainConnections()) {
      this.send(socket, encodeError("That lobby round is no longer available to start", 165, true));
      return;
    }
    const rules = decodeLobbyRules(new Uint8Array(flow.rules));
    if (!flow.signature || flow.signature !== lobbyReadySignature(room.participants, rules)) {
      this.send(socket, encodeError("The lobby's ready players have changed", 165, true));
      return;
    }
    const match = encodeMatchProposal(room.participants, digest, room.stage, rules);
    if (!match) {
      this.send(socket, encodeError("Could not start this lobby roster", 165, true));
      return;
    }
    this.ctx.storage.sql.exec("UPDATE lobby_flow SET history=next_history WHERE id=1");
    this.ctx.storage.sql.exec("UPDATE room_match SET epoch=epoch+1, payload=? WHERE id=1", match);
    this.clearReady();
    this.broadcastPresence(this.nextRevision());
    this.sendLobbyState();
  }

  private setMatch(socket: WebSocket, payload: Uint8Array): void {
    if (this.quickMatch() || this.quickParty()?.mode) {
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
    const flow = this.lobbyFlow();
    if (flow.rules || this.ctx.getWebSockets().some(member =>
        member.readyState === WebSocket.OPEN && this.attachment(member)?.lobbySubscribed)) {
      if (payload.length) this.error(socket, "The lobby service creates the match proposal");
      return;
    }
    const stored = payload.byteLength === 0
      ? null
      : payload.slice().buffer;
    if (stored === null) {
      this.ctx.storage.sql.exec("UPDATE room_stage SET stage=255 WHERE id=1");
      this.resetLobbyReadiness();
    }
    this.ctx.storage.sql.exec(
      "UPDATE room_match SET epoch = epoch + 1, payload = ? WHERE id = 1",
      stored,
    );
    this.clearReady();
    this.broadcastPresence(this.nextRevision());
    this.sendLobbyState();
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

  private cssPointers(socket: WebSocket, sender: ConnectionAttachment, pointers: CssPointers): void {
    if (this.quickParty()?.mode || this.quickMatch()) return;
    const handMask = (1 << Math.max(1, sender.localPlayers)) - 1;
    if (pointers.visible & ~handMask || (sender.localPlayers === 0 && pointers.held)) return;
    const packet = encodeCssPointersUpdate(sender.peerId, pointers);
    for (const other of this.ctx.getWebSockets())
      if (other !== socket && other.readyState === WebSocket.OPEN) this.send(other, packet);
  }

  private stageCursor(sender: ConnectionAttachment, cursor: import("./wire").StageCursor): void {
    const last = sender.stageCursor;
    if (last && last.epoch === cursor.epoch && last.revision === cursor.revision && cursor.sequence <= last.sequence) return;
    const quick = this.quickMatch();
    if (quick) {
      const round = this.quickRound();
      const player = (JSON.parse(round.ports) as QuickPort[]).find(
        p => p.peerId === sender.peerId && p.seat === cursor.seat);
      if (quick.aborted || this.quickFlow().phase !== 3 || cursor.revision !== 0 ||
          cursor.epoch !== round.completed_epoch || !player || player.team !== round.chooser) return;
    } else {
      const flow = this.lobbyFlow();
      if (!sender.lobbySubscribed || this.quickParty()?.mode || !flow.rules ||
          flow.phase !== 2 || flow.revision !== cursor.revision) return;
      const room = this.snapshot(this.currentRevision());
      const rules = decodeLobbyRules(new Uint8Array(flow.rules));
      if (room.match !== null || room.stage !== 255 || room.matchEpoch !== cursor.epoch ||
          !(sender.activeMask & (1 << cursor.seat)) || ![0, 3, 4].includes(rules.stagePolicy) ||
          (rules.stagePolicy !== 0 && (room.stagePeerId !== sender.peerId || room.stageSeat !== cursor.seat))) return;
    }
    // Attachments survive hibernation. Motion never rewrites lobby SQL or
    // readiness, and cannot confirm a stage or restart the match countdown.
    sender.stageCursor = {
      epoch: cursor.epoch, revision: cursor.revision, sequence: cursor.sequence,
    };
    const bytes = encodeStageCursorUpdate(sender.peerId, cursor);
    for (const target of this.ctx.getWebSockets())
      if (target.readyState === WebSocket.OPEN && (quick || this.attachment(target)?.lobbySubscribed))
        this.send(target, bytes);
  }

  private pickLobbyStage(socket: WebSocket,
    message: Extract<ClientMessage, { type: ClientMessageType.PickLobbyStage }>): void {
    const sender = this.attachment(socket);
    const room = this.snapshot(this.currentRevision());
    const owner = room.participants.find((person) => person.peerId === room.ownerPeerId);
    const selection = owner?.selection;
    const flow = this.lobbyFlow();
    const rules = flow.rules ? decodeLobbyRules(new Uint8Array(flow.rules)) : null;
    const requested = rules ? flow.phase === 2 && flow.signature === lobbyReadySignature(room.participants, rules) : selection && selection.length >= 7 &&
      new DataView(selection.buffer, selection.byteOffset).getUint32(0) === 0x464d534c &&
      selection[4] === 1 && (selection[6] & 2) !== 0;
    if (this.quickParty()?.mode || this.quickMatch() || sender === null ||
        !requested || room.match !== null || room.matchEpoch !== message.epoch ||
        // The same retail-versus range accepted by nativeStockContent_StageFromId.
        !LOBBY_STAGES.includes(message.stage) ||
        (rules?.stagePolicy === 0
          ? !(sender.activeMask & (1 << message.seat))
          : room.stagePeerId !== sender.peerId || room.stageSeat !== message.seat)) {
      this.error(socket, "Only the selected active player may choose this stage");
      return;
    }
    // First accepted choice wins until the roster/start request changes.
    if (room.stage !== 255) return;
    this.ctx.storage.sql.exec("UPDATE room_stage SET stage=? WHERE id=1", message.stage);
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

    const ports = lobbyStagePlayers(this.snapshot(this.currentRevision()).participants);
    const allFighters = (1 << ports.length) - 1;
    let loser: typeof ports[number] | undefined = ports[message.loserPort];
    let winners = message.winners;
    if ((message.loserPort !== NO_CARD && !loser) || (message.winners & ~allFighters)) {
      this.error(socket, "Match result names a non-fighting player");
      return;
    }
    type Seat = { socket: WebSocket; seat: number; peerId: string };
    const active: Seat[] = [];
    const waiters: (Seat & { lastPlayed: number; draw: number })[] = [];
    const managed = this.lobbyFlow().rules !== null;
    if (managed) {
      const previous = sender.lobbyResult;
      if (previous?.epoch === match.epoch) return;
      sender.lobbyResult = { epoch: match.epoch, winners, loserPort: message.loserPort };
      socket.serializeAttachment(sender);
    }
    let reportedPeers = 0, agreed = true;
    for (const connected of this.ctx.getWebSockets()) {
      if (connected.readyState !== WebSocket.OPEN) continue;
      const participant = this.attachment(connected);
      if (participant === null || participant.superseded) continue;
      if (managed && participant.activeMask && participant.lobbyResult?.epoch === match.epoch) {
        ++reportedPeers;
        agreed &&= participant.lobbyResult.winners === winners && participant.lobbyResult.loserPort === message.loserPort;
      }
      for (let seat = 0; seat < participant.localPlayers; ++seat) {
        const bit = 1 << seat;
        if ((participant.activeMask & bit) !== 0) {
          active.push({ socket: connected, seat, peerId: participant.peerId });
        }
        if (participant.waitingMask & bit && !participant.uncertain) {
          waiters.push({ socket: connected, seat, peerId: participant.peerId,
            lastPlayed: participant.lastPlayed[seat], draw: 0 });
        }
      }
    }
    if (managed && reportedPeers !== match.payload[MATCH_PEER_COUNT_OFFSET]) return;
    if (!agreed) {
      winners = 0; loser = undefined;
      const notice = encodeError("The players reported different results. Everyone keeps their seat.", 164, true);
      for (const connected of this.ctx.getWebSockets())
        if (connected.readyState === WebSocket.OPEN) this.send(connected, notice);
    }
    const outgoing: Seat[] = [];
    const policy = this.rotationPolicy();
    if (winners && winners !== allFighters &&
        policy !== RotationPolicy.Manual && waiters.length) {
      for (const [port, player] of ports.entries()) {
        const won = !!(winners & (1 << port));
        if (won !== (policy === RotationPolicy.WinnerOut)) continue;
        const seat = active.find(entry => entry.peerId === player.peerId && entry.seat === player.seat);
        if (seat) outgoing.push(seat);
      }
      const draws = crypto.getRandomValues(new Uint32Array(waiters.length));
      waiters.forEach((person, i) => person.draw = draws[i]);
      waiters.sort((a, b) => a.lastPlayed - b.lastPlayed || a.draw - b.draw);
    }
    this.ctx.storage.sql.exec(
      "UPDATE room_stage SET loser_peer=?, loser_seat=?, stage=255 WHERE id=1",
      loser?.peerId ?? "", loser?.seat ?? 255,
    );

    this.ctx.storage.sql.exec(
      "UPDATE room_match SET epoch = epoch + 1, payload = NULL WHERE id = 1",
    );
    this.resetLobbyReadiness();
    this.clearReady();
    const revision = this.nextRevision();
    if (winners || loser) {
      for (const { socket: player, seat } of active) {
        const current = this.attachment(player)!;
        current.lastPlayed[seat] = revision;
        player.serializeAttachment(current);
      }
    }
    for (let i = 0; i < Math.min(outgoing.length, waiters.length); ++i) {
      const departed = outgoing[i], nextWaiting = waiters[i];
      const leaving = this.attachment(departed.socket)!;
      const leavingBit = 1 << departed.seat;
      const card = leaving.cardSlots[departed.seat];
      const leavingCards = leaving.cardSlots.slice() as CardSlots;
      leavingCards[departed.seat] = NO_CARD;
      departed.socket.serializeAttachment({
        ...leaving,
        activeMask: leaving.activeMask & ~leavingBit,
        cardSlots: leavingCards,
        waitingMask: leaving.waitingMask | leavingBit,
        waitingOrder: waitingOrderForChange(
          leaving, leaving.waitingMask | leavingBit, revision,
        ),
      } satisfies ConnectionAttachment);
      const entering = this.attachment(nextWaiting.socket)!;
      const enteringBit = 1 << nextWaiting.seat;
      const enteringCards = entering.cardSlots.slice() as CardSlots;
      enteringCards[nextWaiting.seat] = card;
      nextWaiting.socket.serializeAttachment({
        ...entering,
        activeMask: entering.activeMask | enteringBit,
        cardSlots: enteringCards,
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
    this.ctx.storage.sql.exec("UPDATE room_stage SET stage=255 WHERE id=1");
  }

  private resetLobbyReadiness(): void {
    if (!this.lobbyFlow().rules) return;
    this.ctx.storage.sql.exec("UPDATE lobby_flow SET phase=0, deadline=0, signature='', revision=revision+1 WHERE id=1");
    for (const socket of this.ctx.getWebSockets()) {
      const peer = this.attachment(socket);
      if (!peer || socket.readyState !== WebSocket.OPEN || peer.selection.length < 7) continue;
      const selection = unreadySelection(peer.selection);
      socket.serializeAttachment({ ...peer, ready: false, selection } satisfies ConnectionAttachment);
    }
    this.sendLobbyState();
  }

  private removeParticipant(socket: WebSocket): void {
    const departing = this.attachment(socket);
    if (!departing) {
      if (!this.snapshot(this.currentRevision()).participants.length) {
        this.invalidateMatch(); this.clearEmptyLobby();
      }
      return;
    }
    if (departing?.superseded) return;
    if (departing) socket.serializeAttachment({ ...departing, superseded: true });
    pruneRoomAccounts(this.ctx, this.changingMembership);
    const replacement = departing !== null &&
      this.ctx.getWebSockets().some(candidate => candidate !== socket && candidate.readyState === WebSocket.OPEN &&
        this.attachment(candidate)?.playerIdentity === departing.playerIdentity);
    if (replacement) return;
    if (departing) this.ctx.waitUntil(this.env.PLAYER_SESSIONS.getByName(departing.playerIdentity).leave(departing.connectionId));
    if (!replacement) this.invalidateQuickSearch();
    if (!replacement && departing?.activeMask !== 0 && !this.quickMatch()) {
      this.invalidateMatch(socket);
    }
    if (!this.ctx.getWebSockets().some(other => other !== socket && other.readyState === WebSocket.OPEN) && !this.recovering().length)
      this.clearEmptyLobby();
    this.broadcastPresence(this.nextRevision(), socket);
  }

  private attachment(socket: WebSocket): ConnectionAttachment | null {
    try {
      const attachment = socket.deserializeAttachment() as ConnectionAttachment;
      if (typeof attachment.playerIdentity !== "string" || !attachment.playerIdentity ||
          !Number.isFinite(attachment.lastSeenAt) || !Number.isFinite(attachment.lastAuthorityCheckAt) || typeof attachment.uncertain !== "boolean" ||
          !validSeatMasks(attachment.localPlayers, attachment.activeMask,
                          attachment.waitingMask) ||
          !validCardSlots(attachment.cardSlots, attachment.activeMask) ||
          !Array.isArray(attachment.lastPlayed) || attachment.lastPlayed.length !== 4 ||
          !attachment.lastPlayed.every(value => Number.isSafeInteger(value) && value >= 0) ||
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
    const attached = this.ctx.getWebSockets().filter(socket => socket !== excluded && socket.readyState === WebSocket.OPEN)
      .map(socket => this.attachment(socket)).filter((peer): peer is ConnectionAttachment => peer !== null && !peer.superseded);
    attached.push(...this.recovering().filter(peer => !attached.some(live => live.playerIdentity === peer.playerIdentity)));
    for (const attachment of attached) {
      const participant = publicParticipant(attachment);
      participants.push(participant);
      for (let seat = 0; seat < 4; ++seat) {
        if ((attachment.waitingMask & (1 << seat)) !== 0) {
          waiting.push({ participant, seat,
                         order: attachment.waitingOrder[seat] });
        }
      }
    }
    participants.sort((left, right) => left.joinOrder - right.joinOrder);
    waiting.sort((left, right) => left.order - right.order);
    for (let index = 0; index < waiting.length; ++index) {
      waiting[index].participant.waitingRanks[waiting[index].seat] = index + 1;
    }
    const match = this.currentMatch();
    const stage = this.ctx.storage.sql.exec<RoomStageRow>(
      "SELECT loser_peer, loser_seat, stage FROM room_stage WHERE id=1").one();
    const flow = this.lobbyFlow();
    const rules = flow.rules ? decodeLobbyRules(new Uint8Array(flow.rules)) : null;
    const history = JSON.parse(flow.history) as LobbyStageHistory;
    const chooser = lobbyStageChooser(participants, { peerId: stage.loser_peer, seat: stage.loser_seat }, rules?.stagePolicy, history.turn);
    return {
      type: "presence",
      revision,
      ownerPeerId: participants[0]?.peerId ?? null,
      participants,
      matchEpoch: match.epoch,
      match: match.payload,
      rotationPolicy: this.rotationPolicy(),
      stagePeerId: chooser?.peerId ?? "",
      stageSeat: chooser?.seat ?? 255,
      stage: stage.stage,
    };
  }

  private broadcastPresence(revision: number, excluded?: WebSocket): void {
    const room = this.snapshot(revision, excluded);
    const snapshot = encodePresence(room, MAX_MESSAGE_BYTES);
    for (const socket of this.ctx.getWebSockets()) {
      if (socket !== excluded && socket.readyState === WebSocket.OPEN) {
        this.send(socket, snapshot);
      }
    }
    if (this.listing()) this.advertise(room.participants, excluded);
  }

  private listing(): { title: string; listed: number } | null {
    return this.ctx.storage.sql.exec<{ title: string; listed: number }>(
      "SELECT title, listed FROM room_listing WHERE id=1").toArray()[0] ?? null;
  }
  private fighterLimit(): number {
    return this.ctx.storage.sql.exec<{ fighters: number }>(
      "SELECT fighters FROM room_capacity WHERE id=1").toArray()[0]?.fighters ?? MAX_ACTIVE_PLAYERS;
  }
  private password(): { salt: ArrayBuffer; verifier: ArrayBuffer } | undefined {
    return this.ctx.storage.sql.exec<{ salt: ArrayBuffer; verifier: ArrayBuffer }>(
      "SELECT salt, verifier FROM room_password WHERE id=1").toArray()[0];
  }
  private async passwordVerifier(password: string, salt: ArrayBuffer): Promise<ArrayBuffer> {
    const key = await crypto.subtle.importKey(
      "raw", new TextEncoder().encode(password), "PBKDF2", false, ["deriveBits"]);
    return crypto.subtle.deriveBits({ name: "PBKDF2", salt, hash: "SHA-256", iterations: 100_000 }, key, 256);
  }
  private sendListing(socket: WebSocket): void {
    const listing = this.listing();
    if (listing) this.send(socket, encodeListing(listing.title, !!listing.listed, this.fighterLimit(), !!this.password()));
  }
  private async setListing(socket: WebSocket, title: string, listed: boolean, capacity: number, password: string | null): Promise<void> {
    const peer = this.attachment(socket), revisionBeforeHash = this.currentRevision();
    if (!peer || peer.peerId !== this.snapshot(revisionBeforeHash).ownerPeerId || this.quickParty()?.mode || this.quickMatch()) {
      this.error(socket, "Only a regular lobby's host can change its listing"); return;
    }
    const salt = password ? crypto.getRandomValues(new Uint8Array(16)).buffer : null;
    const verifier = salt && password ? await this.passwordVerifier(password, salt) : null;
    if (socket.readyState !== WebSocket.OPEN || this.currentRevision() !== revisionBeforeHash) {
      this.error(socket, "The lobby changed. Apply these settings again."); return;
    }
    const limitChanged = capacity !== this.fighterLimit();
    if (limitChanged) {
      const people = this.snapshot(revisionBeforeHash).participants;
      const fighting = people.reduce((n,p) => n + countActive(p.activeMask), 0);
      if (this.currentMatch().payload !== null || fighting > capacity) {
        this.error(socket, "Change the fighter limit between matches with enough open seats");
        return;
      }
      let occupied = occupiedCards(people);
      const moved: CardSlots = [0, 1, 2, 3];
      for (let card = capacity; card < 4; ++card) {
        if (!(occupied & (1 << card))) continue;
        let replacement = 0;
        while (occupied & (1 << replacement)) ++replacement;
        moved[card] = replacement;
        occupied |= 1 << replacement;
      }
      if (moved.some((card, previous) => card !== previous)) {
        for (const connected of this.ctx.getWebSockets()) {
          if (connected.readyState !== WebSocket.OPEN) continue;
          const current = this.attachment(connected);
          if (!current) continue;
          current.cardSlots = current.cardSlots.map(card => moved[card] ?? card) as CardSlots;
          connected.serializeAttachment(current);
        }
        for (const reserved of this.recovering()) {
          reserved.cardSlots = reserved.cardSlots.map(card => moved[card] ?? card) as CardSlots;
          this.reserveConnection(reserved);
        }
      }
      this.resetLobbyReadiness();
    }
    this.ctx.storage.transactionSync(() => {
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO room_listing VALUES (1,?,?)", title.trim(), listed ? 1 : 0);
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO room_capacity VALUES (1,?)", capacity);
      if (salt && verifier)
        this.ctx.storage.sql.exec("INSERT OR REPLACE INTO room_password VALUES (1,?,?)", salt, verifier);
      else if (password === "")
        this.ctx.storage.sql.exec("DELETE FROM room_password");
    });
    for (const connected of this.ctx.getWebSockets()) if (connected.readyState === WebSocket.OPEN) this.sendListing(connected);
    const revision = this.nextRevision();
    if (limitChanged) this.broadcastPresence(revision);
    else this.advertise();
  }
  private renewDirectoryListing(): void {
    if (this.lastDirectoryUpdate && Date.now() - this.lastDirectoryUpdate.issued < HEARTBEAT_MS) return;
    if (this.listing()?.listed) this.advertise();
  }
  private advertise(participants?: Participant[], excluded?: WebSocket): void {
    const code = this.ctx.id.name;
    if (!code) return;
    const issued = Date.now(), listing = this.publicListing(participants, excluded);
    const entry = listing?.entry ?? null, payload = JSON.stringify(entry);
    const previous = this.lastDirectoryUpdate;
    if (previous?.payload === payload && (!entry || issued - previous.issued < HEARTBEAT_MS)) return;
    const publication = { payload, issued };
    this.lastDirectoryUpdate = publication;
    this.ctx.waitUntil(this.env.LOBBY_DIRECTORY.getByName("public")
      .advertise(code, this.currentRevision(), issued, entry, listing?.visibleUntil ?? 0).catch(error => {
        if (this.lastDirectoryUpdate === publication) this.lastDirectoryUpdate = undefined;
        throw error;
      }));
  }
  private publicListing(participants?: Participant[], excluded?: WebSocket): { entry: LobbyEntry; visibleUntil: number } | null {
    const listing = this.listing();
    if (!listing?.listed || this.quickParty()?.mode || this.quickMatch()) return null;
    let visibleUntil = 0;
    for (const socket of this.ctx.getWebSockets()) {
      if (socket === excluded || socket.readyState !== WebSocket.OPEN) continue;
      const peer = this.attachment(socket);
      if (peer && !peer.superseded && !peer.uncertain) visibleUntil = Math.max(visibleUntil, peer.lastSeenAt + CONNECTION_TIMEOUT_MS);
    }
    if (visibleUntil <= Date.now()) return null;
    const people = participants ?? this.snapshot(this.currentRevision(), excluded).participants;
    if (!people.length) return null;
    return { visibleUntil, entry: { code: this.ctx.id.name ?? "", title: listing.title, host: people[0].name, build: people[0].build,
      fighting: people.reduce((n,p) => n + countActive(p.activeMask), 0), capacity: this.fighterLimit(), locked: !!this.password(),
      waiting: people.reduce((n,p) => n + countActive(p.waitingMask), 0),
      spectating: people.reduce((n,p) => n + p.localPlayers - countActive(p.activeMask | p.waitingMask) + (p.localPlayers === 0 ? 1 : 0), 0),
      participants: people.length, playing: this.currentMatch().payload !== null } };
  }
  directoryEntry(): LobbyEntry | null {
    return this.publicListing()?.entry ?? null;
  }

  // Operator RPC only: the public Worker exposes no route to these methods.
  maintenanceState(): { revision: number; lobby: LobbyEntry | null } {
    return { revision: this.currentRevision(), lobby: this.directoryEntry() };
  }

  async closeLobby(expectedRevision: number): Promise<boolean> {
    if (this.currentRevision() !== expectedRevision || !this.listing() || this.quickParty()?.mode || this.quickMatch()) return false;
    this.ctx.storage.sql.exec("UPDATE room_listing SET listed=0 WHERE id=1");
    this.ctx.storage.sql.exec("DELETE FROM room_recovery");
    this.invalidateMatch();
    for (const socket of this.ctx.getWebSockets()) socket.close(4006, "Lobby closed by administrator");
    const revision = this.nextRevision();
    this.lastDirectoryUpdate = undefined;
    await this.env.LOBBY_DIRECTORY.getByName("public").advertise(this.ctx.id.name!, revision, Date.now(), null, 0);
    return true;
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
