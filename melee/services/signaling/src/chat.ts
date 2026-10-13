import { DurableObject } from "cloudflare:workers";

// This is one shared conversation for the private client, independent of Rooms.
export const CHAT_HISTORY = 64;
export const CHAT_NAME_BYTES = 32;
export const CHAT_TEXT_BYTES = 512;
export const CHAT_PACKET_BYTES = 48 + CHAT_NAME_BYTES + CHAT_TEXT_BYTES;
const encoder = new TextEncoder();
const decoder = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true });

interface ChatRow extends Record<string, SqlStorageValue> {
  id: number;
  peer: string;
  nonce: number;
  name: string;
  text: string;
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
  const bytes = new Uint8Array(48 + n.length + t.length);
  const view = new DataView(bytes.buffer);
  bytes[0] = 0x81;
  view.setBigUint64(1, BigInt(row.id));
  view.setUint32(9, row.nonce);
  bytes.set(encoder.encode(row.peer), 13);
  bytes[45] = n.length;
  view.setUint16(46, t.length);
  bytes.set(n, 48);
  bytes.set(t, 48 + n.length);
  return bytes;
}

function countPacket(type: number, count: number): Uint8Array {
  const bytes = new Uint8Array(3);
  bytes[0] = type;
  new DataView(bytes.buffer).setUint16(1, count);
  return bytes;
}

export class GlobalChat extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    ctx.blockConcurrencyWhile(async () => {
      ctx.storage.sql.exec(`CREATE TABLE IF NOT EXISTS chat_messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        peer TEXT NOT NULL, nonce INTEGER NOT NULL,
        name TEXT NOT NULL, text TEXT NOT NULL,
        UNIQUE(peer, nonce)
      )`);
    });
  }

  async fetch(request: Request): Promise<Response> {
    const peer = new URL(request.url).searchParams.get("peer") ?? "";
    if (!/^[0-9a-f]{32}$/.test(peer)) return new Response("Invalid chat identity", { status: 400 });
    if (request.method !== "GET" || request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
      return new Response("Expected WebSocket", { status: 426 });
    }
    for (const old of this.ctx.getWebSockets(peer)) old.close(4001, "Reconnected");
    const pair = new WebSocketPair();
    pair[1].serializeAttachment({ peer });
    this.ctx.acceptWebSocket(pair[1], [peer]);
    const rows = this.ctx.storage.sql.exec<ChatRow>(
      "SELECT * FROM chat_messages ORDER BY id DESC LIMIT ?", CHAT_HISTORY,
    ).toArray().reverse();
    for (const row of rows) this.send(pair[1], encodeEntry(row));
    this.send(pair[1], countPacket(0x82, this.connections().length));
    this.presence();
    return new Response(null, { status: 101, webSocket: pair[0] });
  }

  webSocketMessage(socket: WebSocket, message: string | ArrayBuffer): void {
    try {
      if (typeof message === "string" || message.byteLength > CHAT_PACKET_BYTES) throw new Error();
      const bytes = new Uint8Array(message);
      if (bytes.length === 1 && bytes[0] === 2) {
        this.send(socket, new Uint8Array([0x84]));
        return;
      }
      if (bytes.length < 8 || bytes[0] !== 1) throw new Error();
      const view = new DataView(message);
      const nonce = view.getUint32(1);
      const nameSize = bytes[5];
      const textSize = view.getUint16(6);
      if (!nonce || !nameSize || nameSize > CHAT_NAME_BYTES || !textSize ||
          textSize > CHAT_TEXT_BYTES || bytes.length !== 8 + nameSize + textSize) throw new Error();
      const name = decoder.decode(bytes.subarray(8, 8 + nameSize));
      const text = decoder.decode(bytes.subarray(8 + nameSize));
      if (!name.trim() || !text.trim() || /[\x00-\x1f\x7f]/.test(name + text)) throw new Error();
      const { peer } = socket.deserializeAttachment() as { peer: string };
      const previous = this.ctx.storage.sql.exec<ChatRow>(
        "SELECT * FROM chat_messages WHERE peer = ? AND nonce = ?", peer, nonce,
      ).toArray()[0];
      if (previous) {
        if (previous.name !== name || previous.text !== text) throw new Error();
        this.send(socket, encodeEntry(previous));
        return;
      }
      const row = this.ctx.storage.sql.exec<ChatRow>(
        "INSERT INTO chat_messages (peer, nonce, name, text) VALUES (?, ?, ?, ?) RETURNING *",
        peer, nonce, name, text,
      ).one();
      this.ctx.storage.sql.exec(
        "DELETE FROM chat_messages WHERE id NOT IN (SELECT id FROM chat_messages ORDER BY id DESC LIMIT ?)",
        CHAT_HISTORY,
      );
      // Output gates hold this acknowledgement until the SQLite write commits.
      const packet = encodeEntry(row);
      for (const connection of this.connections()) this.send(connection, packet);
    } catch {
      socket.close(1008, "Invalid chat message");
    }
  }

  webSocketClose(): void { this.presence(); }
  webSocketError(socket: WebSocket): void {
    socket.close(1011, "Chat connection failed");
    this.presence();
  }

  private connections(): WebSocket[] {
    return this.ctx.getWebSockets().filter((socket) => socket.readyState === WebSocket.OPEN);
  }

  private presence(): void {
    const sockets = this.connections();
    const packet = countPacket(0x83, sockets.length);
    for (const socket of sockets) this.send(socket, packet);
  }

  private send(socket: WebSocket, packet: Uint8Array): void {
    try { socket.send(packet); }
    catch { socket.close(1011, "Chat send failed"); }
  }
}
