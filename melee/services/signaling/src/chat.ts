import { DurableObject } from "cloudflare:workers";
import { connectionAccount, currentConnectionAccount, type ConnectionAccount } from "./connection-account";
import { HEARTBEAT_MS, CONNECTION_LIFETIME_MS, CONNECTION_TIMEOUT_MS, playerIdentity } from "./connection-lease";

// This is one shared conversation for the private client, independent of Rooms.
export const CHAT_HISTORY = 16;
export const CHAT_NAME_BYTES = 32;
export const CHAT_TEXT_BYTES = 512;
export const CHAT_PACKET_BYTES = 48 + CHAT_NAME_BYTES + CHAT_TEXT_BYTES + 2 + 43 + 24;
export const CHAT_PLAYERS = 128;
const encoder = new TextEncoder();
const decoder = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true });
interface ChatIdentity { peer: string; name: string; playerIdentity: string; account?: ConnectionAccount; lastMessageAt: number; uncertain: boolean }
function accountBytes(id: string, handle: string): Uint8Array {
  const bytes = new Uint8Array(2 + id.length + handle.length);
  bytes.set([id.length, handle.length]); bytes.set(encoder.encode(id), 2);
  bytes.set(encoder.encode(handle), 2 + id.length); return bytes;
}

export function encodeChatIdentity(name: string): Uint8Array {
  const encoded = encoder.encode(name);
  const bytes = new Uint8Array(2 + encoded.length);
  bytes.set([3, encoded.length]);
  bytes.set(encoded, 2);
  return bytes;
}

function identityPacket(identity: ChatIdentity): Uint8Array {
  const name = encoder.encode(identity.name);
  const account = accountBytes(identity.account?.profile.id ?? "", identity.account?.profile.handle ?? "");
  const bytes = new Uint8Array(34 + name.length + account.length);
  bytes[0] = 0x86;
  bytes.set(encoder.encode(identity.peer), 1);
  bytes[33] = name.length;
  bytes.set(name, 34);
  bytes.set(account, 34 + name.length);
  return bytes;
}

interface ChatRow extends Record<string, SqlStorageValue> {
  id: number;
  peer: string;
  nonce: number;
  name: string;
  text: string;
  account_id: string; account_handle: string;
}

export function encodeChatPost(nonce: number, name: string, text: string): Uint8Array {
  const n = encoder.encode(name);
  const t = encoder.encode(text);
  const bytes = new Uint8Array(8 + n.length + t.length);
  const view = new DataView(bytes.buffer);
  bytes[0] = 1;
  view.setUint32(1, nonce);
  bytes[5] = n.length;
  view.setUint16(6, t.length);
  bytes.set(n, 8);
  bytes.set(t, 8 + n.length);
  return bytes;
}

function encodeEntry(row: ChatRow): Uint8Array {
  const n = encoder.encode(row.name);
  const t = encoder.encode(row.text);
  const account = accountBytes(row.account_id, row.account_handle);
  const bytes = new Uint8Array(48 + n.length + t.length + account.length);
  const view = new DataView(bytes.buffer);
  bytes[0] = 0x81;
  view.setBigUint64(1, BigInt(row.id));
  view.setUint32(9, row.nonce);
  bytes.set(encoder.encode(row.peer), 13);
  bytes[45] = n.length;
  view.setUint16(46, t.length);
  bytes.set(n, 48);
  bytes.set(t, 48 + n.length);
  bytes.set(account, 48 + n.length + t.length);
  return bytes;
}

function countPacket(type: number, count: number): Uint8Array {
  const bytes = new Uint8Array(3);
  bytes[0] = type;
  new DataView(bytes.buffer).setUint16(1, count);
  return bytes;
}

export class GlobalChat extends DurableObject<CloudflareBindings> {
  private accountMessages = new Map<string, Promise<void>>();
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    ctx.blockConcurrencyWhile(async () => {
      ctx.storage.sql.exec(`CREATE TABLE IF NOT EXISTS chat_messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        peer TEXT NOT NULL, nonce INTEGER NOT NULL,
        name TEXT NOT NULL, text TEXT NOT NULL,
        UNIQUE(peer, nonce)
      )`);
      const columns = ctx.storage.sql.exec<{ name: string }>("PRAGMA table_info(chat_messages)").toArray();
      if (!columns.some(column => column.name === "account_id"))
        ctx.storage.sql.exec("ALTER TABLE chat_messages ADD COLUMN account_id TEXT NOT NULL DEFAULT ''");
      if (!columns.some(column => column.name === "account_handle"))
        ctx.storage.sql.exec("ALTER TABLE chat_messages ADD COLUMN account_handle TEXT NOT NULL DEFAULT ''");
      ctx.storage.sql.exec("CREATE TABLE IF NOT EXISTS chat_accounts (peer TEXT PRIMARY KEY, account_id TEXT NOT NULL)");
      this.trimHistory();
      for (const socket of ctx.getWebSockets()) {
        const identity = socket.deserializeAttachment() as ChatIdentity;
        if (!identity.playerIdentity || !Number.isFinite(identity.lastMessageAt) || typeof identity.uncertain !== "boolean")
          socket.close(4002, "Connection protocol updated; reconnect with the current client");
      }
    });
  }

  async fetch(request: Request): Promise<Response> {
    const peer = new URL(request.url).searchParams.get("peer") ?? "";
    if (!/^[0-9a-f]{32}$/.test(peer)) return new Response("Invalid chat identity", { status: 400 });
    if (request.method !== "GET" || request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("Expected WebSocket", { status: 426 });
    }
    const account = connectionAccount(request);
    const identity = await playerIdentity(request, account?.profile.id);
    if (!identity) return new Response("Missing guest installation key", { status: 400 });
    const binding = this.ctx.storage.sql.exec<{ account_id: string }>("SELECT account_id FROM chat_accounts WHERE peer=?", peer).toArray()[0];
    if (binding && binding.account_id !== account?.profile.id) return new Response("This connection belongs to another account", { status: 403 });
    for (const old of this.ctx.getWebSockets()) {
      const attached = old.deserializeAttachment() as ChatIdentity;
      if (attached.peer === peer && attached.playerIdentity !== identity)
        return new Response("This connection belongs to another player", { status: 403 });
      if (attached.playerIdentity === identity) old.close(4001, "Reconnected from another client");
    }
    if (this.connections().length >= CHAT_PLAYERS) {
      return new Response("Chatroom is full", { status: 503 });
    }
    const pair = new WebSocketPair();
    if (account) this.ctx.storage.sql.exec("INSERT OR IGNORE INTO chat_accounts VALUES (?,?)", peer, account.profile.id);
    pair[1].serializeAttachment({ peer, name: account?.profile.name ?? "Player", account,
      playerIdentity: identity, lastMessageAt: Date.now(), uncertain: false } satisfies ChatIdentity);
    this.ctx.acceptWebSocket(pair[1], [peer]);
    const rows = this.ctx.storage.sql.exec<ChatRow>(
      "SELECT * FROM chat_messages ORDER BY id DESC LIMIT ?", CHAT_HISTORY,
    ).toArray().reverse();
    for (const row of rows) this.send(pair[1], encodeEntry(row));
    this.send(pair[1], countPacket(0x82, this.connections().length));
    this.presence();
    await this.ctx.storage.setAlarm(Date.now() + HEARTBEAT_MS);
    return new Response(null, { status: 101, webSocket: pair[0] });
  }

  async webSocketMessage(socket: WebSocket, message: string | ArrayBuffer): Promise<void> {
    const initial = socket.deserializeAttachment() as ChatIdentity;
    let release: (() => void) | undefined, lane: Promise<void> | undefined;
    if (initial.account) {
      const previous = this.accountMessages.get(initial.peer);
      lane = new Promise<void>(resolve => { release = resolve; });
      this.accountMessages.set(initial.peer, lane);
      if (previous) await previous;
    }
    try {
      if (socket.readyState !== WebSocket.OPEN) return;
      const identity = socket.deserializeAttachment() as ChatIdentity;
      socket.serializeAttachment({ ...identity, lastMessageAt: Date.now(), uncertain: false });
      if (identity.uncertain) this.presence();
      if (identity.account) {
        const profile = await currentConnectionAccount(identity.account, this.env);
        if (!profile) { socket.close(1008, "Account sign-in expired"); return; }
        if (socket.readyState !== WebSocket.OPEN) return;
        identity.account.profile = profile;
        socket.serializeAttachment({ ...identity, lastMessageAt: Date.now(), uncertain: false });
        this.identify(socket, profile.name);
      }
      if (typeof message === "string" || message.byteLength > CHAT_PACKET_BYTES) throw new Error();
      const bytes = new Uint8Array(message);
      if (bytes.length === 1 && bytes[0] === 2) {
        this.send(socket, new Uint8Array([0x84]));
        return;
      }
      if (bytes.length >= 2 && bytes[0] === 3) {
        const size = bytes[1];
        if (!size || size > CHAT_NAME_BYTES || bytes.length !== 2 + size) throw new Error();
        const name = decoder.decode(bytes.subarray(2));
        if (!name.trim() || /[\x00-\x1f\x7f]/.test(name)) throw new Error();
        this.identify(socket, name);
        return;
      }
      if (bytes.length < 8 || bytes[0] !== 1) throw new Error();
      const view = new DataView(message);
      const nonce = view.getUint32(1);
      const nameSize = bytes[5];
      const textSize = view.getUint16(6);
      if (!nonce || !nameSize || nameSize > CHAT_NAME_BYTES || !textSize ||
          textSize > CHAT_TEXT_BYTES || bytes.length !== 8 + nameSize + textSize) throw new Error();
      const suppliedName = decoder.decode(bytes.subarray(8, 8 + nameSize));
      const name = identity.account?.profile.name ?? suppliedName;
      const text = decoder.decode(bytes.subarray(8 + nameSize));
      if (!name.trim() || !text.trim() || /[\x00-\x1f\x7f]/.test(name + text)) throw new Error();
      const { peer } = socket.deserializeAttachment() as ChatIdentity;
      this.identify(socket, name);
      const previous = this.ctx.storage.sql.exec<ChatRow>(
        "SELECT * FROM chat_messages WHERE peer = ? AND nonce = ?", peer, nonce,
      ).toArray()[0];
      if (previous) {
        if (previous.name !== name || previous.text !== text) throw new Error();
        this.send(socket, encodeEntry(previous));
        return;
      }
      const row = this.ctx.storage.sql.exec<ChatRow>(
        "INSERT INTO chat_messages (peer, nonce, name, text, account_id, account_handle) VALUES (?, ?, ?, ?, ?, ?) RETURNING *",
        peer, nonce, name, text, identity.account?.profile.id ?? "", identity.account?.profile.handle ?? "",
      ).one();
      this.trimHistory();
      // Output gates hold this acknowledgement until the SQLite write commits.
      const packet = encodeEntry(row);
      for (const connection of this.connections()) this.send(connection, packet);
    } catch {
      socket.close(1008, "Invalid chat message");
    } finally {
      release?.();
      if (this.accountMessages.get(initial.peer) === lane) this.accountMessages.delete(initial.peer);
    }
  }

  private trimHistory(): void {
    this.ctx.storage.sql.exec(
      "DELETE FROM chat_messages WHERE id NOT IN (SELECT id FROM chat_messages ORDER BY id DESC LIMIT ?)",
      CHAT_HISTORY,
    );
  }

  webSocketClose(): void { this.presence(); }
  webSocketError(socket: WebSocket): void {
    socket.close(1011, "Chat connection failed");
    this.presence();
  }

  private connections(): WebSocket[] {
    return this.ctx.getWebSockets().filter(socket => socket.readyState === WebSocket.OPEN &&
      !(socket.deserializeAttachment() as ChatIdentity).uncertain);
  }

  async alarm(): Promise<void> {
    let changed = false, remaining = false;
    for (const socket of this.ctx.getWebSockets()) {
      if (socket.readyState !== WebSocket.OPEN) continue;
      const identity = socket.deserializeAttachment() as ChatIdentity;
      const age = Date.now() - identity.lastMessageAt;
      if (age >= CONNECTION_LIFETIME_MS) {
        socket.close(4008, "Heartbeat timed out"); changed = true;
      } else {
        remaining = true;
        const uncertain = age >= CONNECTION_TIMEOUT_MS;
        if (identity.uncertain !== uncertain) {
          socket.serializeAttachment({ ...identity, uncertain }); changed = true;
        }
      }
    }
    if (changed) this.presence();
    if (remaining) await this.ctx.storage.setAlarm(Date.now() + HEARTBEAT_MS);
  }

  // Operator RPC only. Heartbeat timestamps distinguish disconnected ghosts
  // from players who are connected but have not posted chat messages.
  connectedPlayers(): { peer: string; name: string; lastMessageAt: number }[] {
    return this.connections().map(socket => {
      const identity = socket.deserializeAttachment() as ChatIdentity;
      return { peer: identity.peer, name: identity.name, lastMessageAt: identity.lastMessageAt ?? 0 };
    });
  }

  disconnectPlayer(peer: string, expectedLastMessageAt: number): boolean {
    const socket = this.connections().find(socket => {
      const identity = socket.deserializeAttachment() as ChatIdentity;
      return identity.peer === peer && (identity.lastMessageAt ?? 0) === expectedLastMessageAt;
    });
    if (!socket) return false;
    socket.close(4006, "Disconnected connection removed by administrator");
    this.presence();
    return true;
  }

  private presence(): void {
    const sockets = this.connections();
    // One ordered snapshot, published by clients only after its final marker.
    // Small records keep native mailboxes bounded without a large variable
    // receive buffer. Attachment identities survive hibernation.
    const packets = [countPacket(0x85, sockets.length),
      ...sockets.map(socket => identityPacket(socket.deserializeAttachment() as ChatIdentity)),
      new Uint8Array([0x87])];
    for (const socket of sockets) {
      for (const packet of packets) this.send(socket, packet);
    }
  }

  private identify(socket: WebSocket, name: string): void {
    const identity = socket.deserializeAttachment() as ChatIdentity;
    if (identity.account) name = identity.account.profile.name;
    if (identity.name === name) return;
    identity.name = name;
    socket.serializeAttachment(identity);
    this.presence();
  }

  private send(socket: WebSocket, packet: Uint8Array): void {
    try { socket.send(packet); }
    catch { socket.close(1011, "Chat send failed"); }
  }
}
