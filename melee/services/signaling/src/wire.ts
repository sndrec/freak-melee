import { MAX_SELECTION_BYTES, RotationPolicy, validSeatMasks } from "./protocol";
import type { Participant, RoomSnapshot } from "./protocol";
import type { QuickState } from "./quick-play";
import type { LobbyEntry } from "./lobby-directory";

const MAGIC = 0x464d5331;
const VERSION = 5;
const HEADER_BYTES = 8;

export const enum ClientMessageType {
  Update = 1,
  Signal = 2,
  RequestPresence = 3,
  SetMatch = 4,
  AssignSeats = 5,
  SetRotation = 6,
  ReportMatch = 7,
  PostChat = 8,
  RequestChat = 9,
  ConfigureQuick = 10,
  QueueQuick = 11,
  RequestQuick = 12,
  PrepareQuick = 13,
  ReportQuick = 14,
  VoteQuickStage = 15,
  UnreadyQuick = 16,
  PickLobbyStage = 17,
  SetListing = 18,
  RequestLobbies = 19,
}

export const enum ServerMessageType {
  Welcome = 0x81,
  Presence = 0x82,
  Signal = 0x83,
  SignalSent = 0x84,
  ChatEntry = 0x85,
  ChatBegin = 0x86,
  ChatEnd = 0x87,
  ChatRejected = 0x88,
  QuickState = 0x89,
  QuickRejected = 0x8a,
  Lobbies = 0x8b,
  Listing = 0x8c,
  Error = 0xff,
}

const enum UpdateField {
  Name = 1 << 0,
  ActiveMask = 1 << 1,
  LocalPlayers = 1 << 2,
  Ready = 1 << 3,
  Selection = 1 << 4,
  WaitingMask = 1 << 5,
}

export interface RoomChatEntry {
  sequence: number;
  nonce: number;
  peerId: string;
  name: string;
  text: string;
}

export type ClientMessage =
  | { type: ClientMessageType.SetListing; title: string; listed: boolean }
  | { type: ClientMessageType.RequestLobbies; offset: number }
  | {
      type: ClientMessageType.Update;
      name?: string;
      activeMask?: number;
      waitingMask?: number;
      localPlayers?: number;
      ready?: boolean;
      selection?: Uint8Array;
    }
  | { type: ClientMessageType.Signal; to: string; payload: Uint8Array }
  | { type: ClientMessageType.RequestPresence }
  | { type: ClientMessageType.RequestChat }
  | { type: ClientMessageType.ConfigureQuick; mode: number }
  | { type: ClientMessageType.QueueQuick; searching: boolean }
  | { type: ClientMessageType.RequestQuick }
  | { type: ClientMessageType.PrepareQuick; revision: number; prepared: boolean; digest: Uint8Array }
  | { type: ClientMessageType.ReportQuick; epoch: number; winner: number }
  | { type: ClientMessageType.VoteQuickStage; epoch: number; seat: number; stage: number }
  | { type: ClientMessageType.UnreadyQuick; completedEpoch: number }
  | { type: ClientMessageType.PostChat; nonce: number; text: string }
  | { type: ClientMessageType.SetMatch; payload: Uint8Array }
  | { type: ClientMessageType.AssignSeats; peerId: string; activeMask: number; waitingMask: number }
  | { type: ClientMessageType.SetRotation; policy: RotationPolicy }
  | { type: ClientMessageType.PickLobbyStage; epoch: number; seat: number; stage: number }
  | { type: ClientMessageType.ReportMatch; epoch: number; winnerPeerId: string; winnerSeat: number; eligible: boolean; loserPort: number };

export type ServerMessage =
  | { type: ServerMessageType.Lobbies; offset: number; total: number; entries: LobbyEntry[] }
  | { type: ServerMessageType.Listing; title: string; listed: boolean }
  | {
      type: ServerMessageType.Welcome;
      connectionId: string;
      peerId: string;
      revision: number;
    }
  | { type: ServerMessageType.Presence; snapshot: RoomSnapshot }
  | { type: ServerMessageType.Signal; from: string; payload: Uint8Array }
  | { type: ServerMessageType.SignalSent; to: string }
  | { type: ServerMessageType.ChatEntry; entry: RoomChatEntry }
  | { type: ServerMessageType.ChatBegin; count: number }
  | { type: ServerMessageType.ChatEnd; sequence: number }
  | { type: ServerMessageType.ChatRejected; nonce: number; reason: string }
  | { type: ServerMessageType.QuickState; quick: QuickState }
  | { type: ServerMessageType.QuickRejected; reason: string }
  | { type: ServerMessageType.Error; message: string };

class Reader {
  private readonly view: DataView;
  private offset = 0;

  constructor(private readonly bytes: Uint8Array) {
    this.view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  }

  remaining(): number {
    return this.bytes.byteLength - this.offset;
  }

  u8(): number {
    this.require(1);
    return this.view.getUint8(this.offset++);
  }

  u16(): number {
    this.require(2);
    const value = this.view.getUint16(this.offset, false);
    this.offset += 2;
    return value;
  }

  u32(): number {
    this.require(4);
    const value = this.view.getUint32(this.offset, false);
    this.offset += 4;
    return value;
  }

  u64(): number {
    this.require(8);
    const value = this.view.getBigUint64(this.offset, false);
    this.offset += 8;
    const number = Number(value);
    if (!Number.isSafeInteger(number)) {
      throw new Error("Protocol integer exceeds JavaScript precision");
    }
    return number;
  }

  bytes16(): Uint8Array {
    const length = this.u16();
    this.require(length);
    const value = this.bytes.slice(this.offset, this.offset + length);
    this.offset += length;
    return value;
  }

  string8(): string {
    const length = this.u8();
    this.require(length);
    const value = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true }).decode(
      this.bytes.subarray(this.offset, this.offset + length),
    );
    this.offset += length;
    return value;
  }

  finish(): void {
    if (this.remaining() !== 0) {
      throw new Error("Trailing message bytes");
    }
  }

  private require(length: number): void {
    if (length < 0 || length > this.remaining()) {
      throw new Error("Truncated message");
    }
  }
}

class Writer {
  private readonly bytes: Uint8Array;
  private readonly view: DataView;
  private offset = HEADER_BYTES;

  constructor(type: ClientMessageType | ServerMessageType, maximumBytes: number) {
    this.bytes = new Uint8Array(maximumBytes);
    this.view = new DataView(this.bytes.buffer);
    this.view.setUint32(0, MAGIC, false);
    this.view.setUint8(4, VERSION);
    this.view.setUint8(5, type);
  }

  u8(value: number): void {
    this.require(1);
    this.view.setUint8(this.offset++, value);
  }

  u16(value: number): void {
    this.require(2);
    this.view.setUint16(this.offset, value, false);
    this.offset += 2;
  }

  u32(value: number): void {
    this.require(4);
    this.view.setUint32(this.offset, value, false);
    this.offset += 4;
  }

  u64(value: number): void {
    if (!Number.isSafeInteger(value) || value < 0) {
      throw new Error("Invalid protocol integer");
    }
    this.require(8);
    this.view.setBigUint64(this.offset, BigInt(value), false);
    this.offset += 8;
  }

  string8(value: string): void {
    const encoded = new TextEncoder().encode(value);
    if (encoded.byteLength > 0xff) {
      throw new Error("Protocol string is too long");
    }
    this.u8(encoded.byteLength);
    this.raw(encoded);
  }

  bytes16(value: Uint8Array): void {
    if (value.byteLength > 0xffff) {
      throw new Error("Protocol payload is too long");
    }
    this.u16(value.byteLength);
    this.raw(value);
  }

  finish(): ArrayBuffer {
    this.view.setUint16(6, this.offset - HEADER_BYTES, false);
    return this.bytes.slice(0, this.offset).buffer;
  }

  private raw(value: Uint8Array): void {
    this.require(value.byteLength);
    this.bytes.set(value, this.offset);
    this.offset += value.byteLength;
  }

  private require(length: number): void {
    if (this.offset + length > this.bytes.byteLength) {
      throw new Error("Message exceeds protocol limit");
    }
  }
}

function readEnvelope(message: ArrayBuffer): { type: number; reader: Reader } {
  const bytes = new Uint8Array(message);
  if (bytes.byteLength < HEADER_BYTES) {
    throw new Error("Truncated message header");
  }
  const header = new DataView(bytes.buffer, bytes.byteOffset, HEADER_BYTES);
  if (header.getUint32(0, false) !== MAGIC || header.getUint8(4) !== VERSION) {
    throw new Error("Unsupported signaling protocol");
  }
  if (header.getUint16(6, false) !== bytes.byteLength - HEADER_BYTES) {
    throw new Error("Invalid message length");
  }
  return { type: header.getUint8(5), reader: new Reader(bytes.subarray(HEADER_BYTES)) };
}

export function decodeClientMessage(message: ArrayBuffer): ClientMessage {
  const { type, reader } = readEnvelope(message);
  if (type === ClientMessageType.SetListing) {
    const title = reader.string8(), listed = reader.u8(); reader.finish();
    if (new TextEncoder().encode(title).length > 64 || !title.trim() || /[\x00-\x1f\x7f]/.test(title) || listed > 1)
      throw new Error("Invalid lobby listing");
    return { type, title, listed: listed === 1 };
  }
  if (type === ClientMessageType.RequestLobbies) {
    const offset = reader.u16(); reader.finish(); return { type, offset };
  }
  if (type === ClientMessageType.UnreadyQuick) {
    const completedEpoch = reader.u64(); reader.finish();
    return { type, completedEpoch };
  }
  if (type === ClientMessageType.ReportQuick) {
    const epoch = reader.u64(), winner = reader.u8(); reader.finish();
    if (winner > 1 && winner !== 255) throw new Error("Invalid Quick Play winner");
    return { type, epoch, winner };
  }
  if (type === ClientMessageType.VoteQuickStage) {
    const epoch = reader.u64(), seat = reader.u8(), stage = reader.u8(); reader.finish();
    if (seat > 1 || (stage > 5 && stage !== 255)) throw new Error("Invalid Quick Play stage vote");
    return { type, epoch, seat, stage };
  }
  if (type === ClientMessageType.PrepareQuick) {
    const revision = reader.u64(), prepared = reader.u8(), digest = reader.bytes16();
    reader.finish();
    if (prepared > 1 || digest.length !== (prepared ? 32 : 0)) throw new Error("Invalid Quick Play preparation");
    return { type, revision, prepared: prepared === 1, digest };
  }
  if (type === ClientMessageType.ConfigureQuick) {
    const mode = reader.u8(); reader.finish(); return { type, mode };
  }
  if (type === ClientMessageType.QueueQuick) {
    const value = reader.u8(); reader.finish();
    if (value > 1) throw new Error("Invalid queue command");
    return { type, searching: value === 1 };
  }
  if (type === ClientMessageType.RequestQuick) {
    reader.finish(); return { type };
  }
  if (type === ClientMessageType.RequestChat) {
    reader.finish();
    return { type };
  }
  if (type === ClientMessageType.PostChat) {
    const nonce = reader.u32();
    const text = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true }).decode(reader.bytes16());
    reader.finish();
    return { type, nonce, text };
  }
  if (type === ClientMessageType.RequestPresence) {
    reader.finish();
    return { type };
  }
  if (type === ClientMessageType.SetMatch) {
    const result: ClientMessage = { type, payload: reader.bytes16() };
    reader.finish();
    return result;
  }
  if (type === ClientMessageType.AssignSeats) {
    const result: ClientMessage = {
      type,
      peerId: reader.string8(),
      activeMask: reader.u8(),
      waitingMask: reader.u8(),
    };
    reader.finish();
    return result;
  }
  if (type === ClientMessageType.SetRotation) {
    const policy = reader.u8();
    if (policy > RotationPolicy.WinnerOut) throw new Error("Invalid rotation policy");
    reader.finish();
    return { type, policy };
  }
  if (type === ClientMessageType.ReportMatch) {
    const epoch = reader.u64();
    const winnerPeerId = reader.string8();
    const winnerSeat = reader.u8();
    const eligibleByte = reader.u8();
    const loserPort = reader.u8();
    if (eligibleByte > 1 || (loserPort > 3 && loserPort !== 255)) throw new Error("Invalid match report");
    reader.finish();
    return { type, epoch, winnerPeerId, winnerSeat,
      eligible: eligibleByte === 1, loserPort };
  }
  if (type === ClientMessageType.PickLobbyStage) {
    const epoch = reader.u64(), seat = reader.u8(), stage = reader.u8();
    if (seat > 3 || stage === 255) throw new Error("Invalid stage choice");
    reader.finish();
    return { type, epoch, seat, stage };
  }
  if (type === ClientMessageType.Update) {
    const fields = reader.u8();
    if ((fields & ~0x3f) !== 0 || fields === 0) {
      throw new Error("Invalid update fields");
    }
    const result: Extract<ClientMessage, { type: ClientMessageType.Update }> = { type };
    if ((fields & UpdateField.Name) !== 0) result.name = reader.string8();
    if ((fields & UpdateField.ActiveMask) !== 0) result.activeMask = reader.u8();
    if ((fields & UpdateField.LocalPlayers) !== 0) result.localPlayers = reader.u8();
    if ((fields & UpdateField.Ready) !== 0) {
      const ready = reader.u8();
      if (ready > 1) throw new Error("Invalid ready state");
      result.ready = ready === 1;
    }
    if ((fields & UpdateField.Selection) !== 0) {
      result.selection = reader.bytes16();
      if (result.selection.byteLength > MAX_SELECTION_BYTES) {
        throw new Error("Participant selection is too large");
      }
    }
    if ((fields & UpdateField.WaitingMask) !== 0) result.waitingMask = reader.u8();
    reader.finish();
    return result;
  }
  if (type === ClientMessageType.Signal) {
    const result: ClientMessage = {
      type,
      to: reader.string8(),
      payload: reader.bytes16(),
    };
    reader.finish();
    return result;
  }
  throw new Error("Unknown client message type");
}

export function decodeServerMessage(message: ArrayBuffer): ServerMessage {
  const { type, reader } = readEnvelope(message);
  if (type === ServerMessageType.Listing) {
    const title = reader.string8(), listed = reader.u8(); reader.finish();
    if (listed > 1) throw new Error("Invalid listing state");
    return { type, title, listed: listed === 1 };
  }
  if (type === ServerMessageType.Lobbies) {
    const offset = reader.u16(), total = reader.u16(), count = reader.u8();
    if (count > 8 || offset + count > total) throw new Error("Invalid lobby page");
    const entries: LobbyEntry[] = [];
    for (let i = 0; i < count; ++i) entries.push({ code: reader.string8(), title: reader.string8(),
      host: reader.string8(), build: "", fighting: reader.u8(), waiting: reader.u8(),
      spectating: reader.u8(), participants: reader.u8(), playing: reader.u8() !== 0 });
    reader.finish(); return { type, offset, total, entries };
  }
  if (type === ServerMessageType.QuickState) {
    const quick = { mode: reader.u8(), state: reader.u8(), generation: reader.u64(),
      matchRoom: reader.string8(), admission: reader.string8(), team: reader.u8(), stage: reader.u8(),
      phase: reader.u8(), remainingMs: reader.u16(), completedEpoch: reader.u64(),
      chooser: reader.u8(), votes: [reader.u8(), reader.u8(), reader.u8(), reader.u8()] };
    reader.finish(); return { type, quick };
  }
  if (type === ServerMessageType.QuickRejected) {
    const reason = reader.string8(); reader.finish(); return { type, reason };
  }
  if (type === ServerMessageType.ChatEntry) {
    const entry = { sequence: reader.u64(), nonce: reader.u32(), peerId: reader.string8(),
      name: reader.string8(), text: new TextDecoder("utf-8", { fatal: true, ignoreBOM: true }).decode(reader.bytes16()) };
    reader.finish();
    return { type, entry };
  }
  if (type === ServerMessageType.ChatBegin) {
    const count = reader.u8(); reader.finish(); return { type, count };
  }
  if (type === ServerMessageType.ChatEnd) {
    const sequence = reader.u64(); reader.finish(); return { type, sequence };
  }
  if (type === ServerMessageType.ChatRejected) {
    const nonce = reader.u32(), reason = reader.string8();
    reader.finish(); return { type, nonce, reason };
  }
  if (type === ServerMessageType.Welcome) {
    const result: ServerMessage = {
      type,
      connectionId: reader.string8(),
      peerId: reader.string8(),
      revision: reader.u64(),
    };
    reader.finish();
    return result;
  }
  if (type === ServerMessageType.Presence) {
    const revision = reader.u64();
    const ownerPeerId = reader.string8() || null;
    const count = reader.u8();
    const participants: Participant[] = [];
    for (let i = 0; i < count; ++i) {
      participants.push(readParticipant(reader));
    }
    const matchEpoch = reader.u64();
    const match = reader.bytes16();
    const rotationPolicy = reader.u8();
    const stagePeerId = reader.string8(), stageSeat = reader.u8(), stage = reader.u8();
    if (rotationPolicy > RotationPolicy.WinnerOut) {
      throw new Error("Invalid rotation policy");
    }
    reader.finish();
    return {
      type,
      snapshot: {
        type: "presence",
        revision,
        ownerPeerId,
        participants,
        matchEpoch,
        match: match.byteLength === 0 ? null : match,
        rotationPolicy,
        stagePeerId, stageSeat, stage,
      },
    };
  }
  if (type === ServerMessageType.Signal) {
    const result: ServerMessage = {
      type,
      from: reader.string8(),
      payload: reader.bytes16(),
    };
    reader.finish();
    return result;
  }
  if (type === ServerMessageType.SignalSent) {
    const result: ServerMessage = { type, to: reader.string8() };
    reader.finish();
    return result;
  }
  if (type === ServerMessageType.Error) {
    const result: ServerMessage = {
      type,
      message: new TextDecoder("utf-8", { fatal: true, ignoreBOM: true }).decode(
        reader.bytes16(),
      ),
    };
    reader.finish();
    return result;
  }
  throw new Error("Unknown server message type");
}

export function encodeUpdate(
  update: Omit<Extract<ClientMessage, { type: ClientMessageType.Update }>, "type">,
  maximumBytes: number,
): ArrayBuffer {
  if (update.selection !== undefined &&
      update.selection.byteLength > MAX_SELECTION_BYTES) {
    throw new Error("Participant selection is too large");
  }
  const writer = new Writer(ClientMessageType.Update, maximumBytes);
  let fields = 0;
  if (update.name !== undefined) fields |= UpdateField.Name;
  if (update.activeMask !== undefined) fields |= UpdateField.ActiveMask;
  if (update.localPlayers !== undefined) fields |= UpdateField.LocalPlayers;
  if (update.ready !== undefined) fields |= UpdateField.Ready;
  if (update.selection !== undefined) fields |= UpdateField.Selection;
  if (update.waitingMask !== undefined) fields |= UpdateField.WaitingMask;
  if (fields === 0) throw new Error("Empty participant update");
  writer.u8(fields);
  if (update.name !== undefined) writer.string8(update.name);
  if (update.activeMask !== undefined) writer.u8(update.activeMask);
  if (update.localPlayers !== undefined) writer.u8(update.localPlayers);
  if (update.ready !== undefined) writer.u8(update.ready ? 1 : 0);
  if (update.selection !== undefined) writer.bytes16(update.selection);
  if (update.waitingMask !== undefined) writer.u8(update.waitingMask);
  return writer.finish();
}

export function encodeSignalRequest(
  to: string,
  payload: Uint8Array,
  maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ClientMessageType.Signal, maximumBytes);
  writer.string8(to);
  writer.bytes16(payload);
  return writer.finish();
}

export function encodeRequestPresence(maximumBytes: number): ArrayBuffer {
  return new Writer(
    ClientMessageType.RequestPresence,
    maximumBytes,
  ).finish();
}

export function encodeSetMatch(
  payload: Uint8Array,
  maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ClientMessageType.SetMatch, maximumBytes);
  writer.bytes16(payload);
  return writer.finish();
}

export function encodeAssignSeats(
  peerId: string, activeMask: number, waitingMask: number,
  maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ClientMessageType.AssignSeats, maximumBytes);
  writer.string8(peerId);
  writer.u8(activeMask);
  writer.u8(waitingMask);
  return writer.finish();
}

export function encodeSetRotation(policy: RotationPolicy, maximumBytes: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.SetRotation, maximumBytes);
  writer.u8(policy);
  return writer.finish();
}

export function encodeReportMatch(
  epoch: number, winnerPeerId: string, winnerSeat: number,
  eligible: boolean, loserPort: number, maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ClientMessageType.ReportMatch, maximumBytes);
  writer.u64(epoch);
  writer.string8(winnerPeerId);
  writer.u8(winnerSeat);
  writer.u8(eligible ? 1 : 0);
  writer.u8(loserPort);
  return writer.finish();
}

export function encodePickLobbyStage(epoch: number, seat: number, stage: number, maximumBytes: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.PickLobbyStage, maximumBytes);
  writer.u64(epoch); writer.u8(seat); writer.u8(stage);
  return writer.finish();
}

export function encodeWelcome(
  connectionId: string,
  peerId: string,
  revision: number,
  maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Welcome, maximumBytes);
  writer.string8(connectionId);
  writer.string8(peerId);
  writer.u64(revision);
  return writer.finish();
}

export function encodePresence(snapshot: RoomSnapshot, maximumBytes: number): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Presence, maximumBytes);
  writer.u64(snapshot.revision);
  writer.string8(snapshot.ownerPeerId ?? "");
  writer.u8(snapshot.participants.length);
  for (const participant of snapshot.participants) {
    writeParticipant(writer, participant);
  }
  writer.u64(snapshot.matchEpoch);
  writer.bytes16(snapshot.match ?? new Uint8Array());
  writer.u8(snapshot.rotationPolicy);
  writer.string8(snapshot.stagePeerId);
  writer.u8(snapshot.stageSeat);
  writer.u8(snapshot.stage);
  return writer.finish();
}

export function encodeSignal(
  from: string,
  payload: Uint8Array,
  maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Signal, maximumBytes);
  writer.string8(from);
  writer.bytes16(payload);
  return writer.finish();
}

export function encodeSignalSent(to: string, maximumBytes: number): ArrayBuffer {
  const writer = new Writer(ServerMessageType.SignalSent, maximumBytes);
  writer.string8(to);
  return writer.finish();
}

export function encodeError(message: string, maximumBytes: number): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Error, maximumBytes);
  writer.bytes16(new TextEncoder().encode(message));
  return writer.finish();
}

function writeParticipant(writer: Writer, participant: Participant): void {
  writer.string8(participant.peerId);
  writer.string8(participant.name);
  writer.string8(participant.build);
  writer.u8(participant.localPlayers);
  writer.u8(participant.activeMask);
  writer.u8(participant.waitingMask);
  writer.u8(participant.ready ? 1 : 0);
  writer.u64(participant.joinOrder);
  for (const rank of participant.waitingRanks) writer.u8(rank);
  writer.bytes16(participant.selection);
}

function readParticipant(reader: Reader): Participant {
  const peerId = reader.string8();
  const name = reader.string8();
  const build = reader.string8();
  const localPlayers = reader.u8();
  const activeMask = reader.u8();
  const waitingMask = reader.u8();
  const ready = reader.u8();
  const joinOrder = reader.u64();
  const waitingRanks: [number, number, number, number] = [
    reader.u8(), reader.u8(), reader.u8(), reader.u8(),
  ];
  const selection = reader.bytes16();
  if (
    !validSeatMasks(localPlayers, activeMask, waitingMask) ||
    ready > 1 ||
    selection.byteLength > MAX_SELECTION_BYTES ||
    (ready === 1 && activeMask === 0) ||
    waitingRanks.some((rank, seat) =>
      ((waitingMask & (1 << seat)) !== 0) !== (rank > 0))
  ) {
    throw new Error("Invalid participant state");
  }
  const participant: Participant = {
    peerId,
    name,
    build,
    localPlayers,
    activeMask,
    waitingMask,
    ready: ready === 1,
    joinOrder,
    waitingRanks,
    selection,
  };
  return participant;
}

export function encodePostChat(nonce: number, text: string): ArrayBuffer {
  const writer = new Writer(ClientMessageType.PostChat, 16 * 1024);
  writer.u32(nonce);
  writer.bytes16(new TextEncoder().encode(text));
  return writer.finish();
}
export function encodeRequestChat(): ArrayBuffer {
  return new Writer(ClientMessageType.RequestChat, 8).finish();
}
export function encodeChatEntry(entry: RoomChatEntry): ArrayBuffer {
  const writer = new Writer(ServerMessageType.ChatEntry, 1024);
  writer.u64(entry.sequence);
  writer.u32(entry.nonce);
  writer.string8(entry.peerId);
  writer.string8(entry.name);
  writer.bytes16(new TextEncoder().encode(entry.text));
  return writer.finish();
}
export function encodeChatBegin(count: number): ArrayBuffer {
  const writer = new Writer(ServerMessageType.ChatBegin, 9);
  writer.u8(count);
  return writer.finish();
}
export function encodeChatEnd(sequence: number): ArrayBuffer {
  const writer = new Writer(ServerMessageType.ChatEnd, 16);
  writer.u64(sequence);
  return writer.finish();
}
export function encodeChatRejected(nonce: number, reason: string): ArrayBuffer {
  const writer = new Writer(ServerMessageType.ChatRejected, 141);
  writer.u32(nonce);
  writer.string8(reason);
  return writer.finish();
}

export function encodeConfigureQuick(mode: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.ConfigureQuick, 9);
  writer.u8(mode); return writer.finish();
}
export function encodeQueueQuick(searching: boolean): ArrayBuffer {
  const writer = new Writer(ClientMessageType.QueueQuick, 9);
  writer.u8(searching ? 1 : 0); return writer.finish();
}
export function encodeRequestQuick(): ArrayBuffer {
  return new Writer(ClientMessageType.RequestQuick, 8).finish();
}

export function encodeSetListing(title: string, listed: boolean): ArrayBuffer {
  const writer = new Writer(ClientMessageType.SetListing, 80);
  writer.string8(title); writer.u8(listed ? 1 : 0); return writer.finish();
}
export function encodeRequestLobbies(offset: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.RequestLobbies, 10);
  writer.u16(offset); return writer.finish();
}
export function encodeListing(title: string, listed: boolean): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Listing, 80);
  writer.string8(title); writer.u8(listed ? 1 : 0); return writer.finish();
}
export function encodeLobbyDirectory(offset: number, total: number, entries: LobbyEntry[]): ArrayBuffer {
  const writer = new Writer(ServerMessageType.Lobbies, 2048);
  writer.u16(offset); writer.u16(total); writer.u8(entries.length);
  for (const entry of entries) {
    writer.string8(entry.code); writer.string8(entry.title); writer.string8(entry.host);
    writer.u8(entry.fighting); writer.u8(entry.waiting); writer.u8(entry.spectating);
    writer.u8(entry.participants); writer.u8(entry.playing ? 1 : 0);
  }
  return writer.finish();
}
export function encodeQuickState(quick: QuickState): ArrayBuffer {
  const writer = new Writer(ServerMessageType.QuickState, 128);
  writer.u8(quick.mode); writer.u8(quick.state); writer.u64(quick.generation);
  writer.string8(quick.matchRoom); writer.string8(quick.admission);
  writer.u8(quick.team); writer.u8(quick.stage);
  writer.u8(quick.phase); writer.u16(quick.remainingMs);
  writer.u64(quick.completedEpoch); writer.u8(quick.chooser);
  for (const vote of quick.votes) writer.u8(vote);
  return writer.finish();
}
export function encodeReportQuick(epoch: number, winner: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.ReportQuick, 17);
  writer.u64(epoch); writer.u8(winner); return writer.finish();
}
export function encodeVoteQuickStage(epoch: number, seat: number, stage: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.VoteQuickStage, 18);
  writer.u64(epoch); writer.u8(seat); writer.u8(stage); return writer.finish();
}
export function encodeUnreadyQuick(completedEpoch: number): ArrayBuffer {
  const writer = new Writer(ClientMessageType.UnreadyQuick, 16);
  writer.u64(completedEpoch); return writer.finish();
}
export function encodePrepareQuick(revision: number, prepared: boolean, digest: Uint8Array): ArrayBuffer {
  const writer = new Writer(ClientMessageType.PrepareQuick, 51);
  writer.u64(revision); writer.u8(prepared ? 1 : 0);
  writer.bytes16(prepared ? digest : new Uint8Array()); return writer.finish();
}
export function encodeQuickRejected(reason: string): ArrayBuffer {
  const writer = new Writer(ServerMessageType.QuickRejected, 138);
  writer.string8(reason); return writer.finish();
}
