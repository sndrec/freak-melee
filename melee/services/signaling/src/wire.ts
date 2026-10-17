import { MAX_SELECTION_BYTES, RotationPolicy, validSeatMasks } from "./protocol";
import type { Participant, RoomSnapshot } from "./protocol";

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
  | { type: ClientMessageType.PostChat; nonce: number; text: string }
  | { type: ClientMessageType.SetMatch; payload: Uint8Array }
  | { type: ClientMessageType.AssignSeats; peerId: string; activeMask: number; waitingMask: number }
  | { type: ClientMessageType.SetRotation; policy: RotationPolicy }
  | { type: ClientMessageType.ReportMatch; epoch: number; winnerPeerId: string; winnerSeat: number; eligible: boolean };

export type ServerMessage =
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
    if (eligibleByte > 1) throw new Error("Invalid match report");
    reader.finish();
    return { type, epoch, winnerPeerId, winnerSeat,
      eligible: eligibleByte === 1 };
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
  eligible: boolean, maximumBytes: number,
): ArrayBuffer {
  const writer = new Writer(ClientMessageType.ReportMatch, maximumBytes);
  writer.u64(epoch);
  writer.string8(winnerPeerId);
  writer.u8(winnerSeat);
  writer.u8(eligible ? 1 : 0);
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
