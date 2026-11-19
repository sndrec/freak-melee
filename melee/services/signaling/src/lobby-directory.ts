import { DurableObject } from "cloudflare:workers";
import { MAX_BUILD_LENGTH, isBoundedUtf8 } from "./protocol";
import { ClientMessageType, decodeClientMessage, encodeError, encodeLobbyDirectory } from "./wire";

export interface LobbyEntry {
  code: string;
  title: string;
  host: string;
  build: string;
  fighting: number;
  capacity: number;
  locked: boolean;
  waiting: number;
  spectating: number;
  participants: number;
  playing: boolean;
}
interface ListingRow extends Record<string, SqlStorageValue> {
  code: string; revision: number; listed: number;
}

// Rooms publish their identity, never client-supplied counts or host names.
// Each page reads the live Room objects, including hibernated WebSockets.
export class LobbyDirectory extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    this.ctx.storage.sql.exec(`CREATE TABLE IF NOT EXISTS listings (
      code TEXT PRIMARY KEY, revision INTEGER NOT NULL, listed INTEGER NOT NULL)`);
    this.ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair("ping", "pong"));
  }

  advertise(code: string, revision: number, listed: boolean): void {
    this.ctx.storage.sql.exec(`INSERT INTO listings VALUES (?,?,?)
      ON CONFLICT(code) DO UPDATE SET revision=excluded.revision, listed=excluded.listed
      WHERE excluded.revision > listings.revision`, code, revision, listed ? 1 : 0);
  }

  async fetch(request: Request): Promise<Response> {
    const url = new URL(request.url);
    const build = url.searchParams.get("build") ?? "";
    if (request.method !== "GET" || request.headers.get("Upgrade")?.toLowerCase() !== "websocket")
      return new Response("Expected a WebSocket upgrade", { status: 426 });
    if (!isBoundedUtf8(build, 1, MAX_BUILD_LENGTH)) return new Response("Invalid build", { status: 400 });
    if (this.ctx.getWebSockets().length >= 128) return new Response("Lobby browser is busy", { status: 503 });
    const pair = new WebSocketPair();
    pair[1].serializeAttachment({ build, lastRequest: 0 });
    this.ctx.acceptWebSocket(pair[1]);
    return new Response(null, { status: 101, webSocket: pair[0] });
  }

  async webSocketMessage(socket: WebSocket, data: string | ArrayBuffer): Promise<void> {
    try {
      if (typeof data === "string" || data.byteLength > 16) throw new Error("Invalid directory request");
      const message = decodeClientMessage(data);
      if (message.type !== ClientMessageType.RequestLobbies) throw new Error("Expected a lobby request");
      const attachment = socket.deserializeAttachment() as { build: string; lastRequest: number };
      const now = Date.now();
      if (now - attachment.lastRequest < 250) throw new Error("Please wait before refreshing lobbies");
      socket.serializeAttachment({ ...attachment, lastRequest: now });
      const entries: LobbyEntry[] = [];
      const rows = this.ctx.storage.sql.exec<ListingRow>(
        "SELECT code, revision, listed FROM listings WHERE listed=1 ORDER BY code LIMIT 512").toArray();
      // Bound concurrent RPC fan-out. Work in small batches instead of waking
      // hundreds of room objects at once.
      for (let start = 0; start < rows.length; start += 16) {
        const batch = await Promise.all(rows.slice(start, start + 16).map(row =>
          this.env.ROOMS.getByName(row.code).directoryEntry()));
        for (const entry of batch) if (entry && entry.build === attachment.build) entries.push(entry);
      }
      const offset = Math.min(message.offset, Math.max(0, Math.floor((entries.length - 1) / 8) * 8));
      socket.send(encodeLobbyDirectory(offset, entries.length, entries.slice(offset, offset + 8)));
    } catch (error) {
      socket.send(encodeError(error instanceof Error ? error.message : "Lobby browser unavailable", 1024));
    }
  }

  webSocketClose(socket: WebSocket): void { socket.close(1000, "Browser closed"); }
  webSocketError(socket: WebSocket): void { socket.close(1011, "Browser disconnected"); }
}
