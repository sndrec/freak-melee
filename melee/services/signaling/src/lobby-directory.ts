import { DurableObject } from "cloudflare:workers";
import { CONNECTION_TIMEOUT_MS } from "./connection-lease";
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
const CLOCK_SKEW_MS = 5000;
const MAX_RESULTS = 512;
const PAGE_SIZE = 8;

// Each Room publishes the summary it derives from its own sockets and settings.
export class LobbyDirectory extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    this.ctx.storage.sql.exec(`CREATE TABLE IF NOT EXISTS lobby_summaries (
      code TEXT PRIMARY KEY, revision INTEGER NOT NULL, issued INTEGER NOT NULL,
      build TEXT, entry TEXT, visible_until INTEGER NOT NULL, retire_at INTEGER NOT NULL);
      CREATE INDEX IF NOT EXISTS lobby_build ON lobby_summaries(build, code);
      CREATE INDEX IF NOT EXISTS lobby_retirement ON lobby_summaries(retire_at);
      DROP TABLE IF EXISTS listings`);
    this.ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair("ping", "pong"));
  }

  async advertise(code: string, revision: number, issued: number, entry: LobbyEntry | null, visibleUntil: number): Promise<void> {
    const now = Date.now();
    if (!Number.isSafeInteger(issued) || issued <= now - CONNECTION_TIMEOUT_MS || issued > now + CLOCK_SKEW_MS) return;
    this.ctx.storage.sql.exec(`INSERT INTO lobby_summaries VALUES (?,?,?,?,?,?,?)
      ON CONFLICT(code) DO UPDATE SET revision=excluded.revision, issued=excluded.issued,
        build=excluded.build, entry=excluded.entry, visible_until=excluded.visible_until, retire_at=excluded.retire_at
      WHERE excluded.revision > lobby_summaries.revision OR
        (excluded.revision = lobby_summaries.revision AND excluded.issued > lobby_summaries.issued)`,
      code, revision, issued, entry?.build ?? null, entry ? JSON.stringify(entry) : null,
      entry ? Math.min(visibleUntil, issued + CONNECTION_TIMEOUT_MS) : 0, now + CONNECTION_TIMEOUT_MS + CLOCK_SKEW_MS);
    if (await this.ctx.storage.getAlarm() === null) await this.ctx.storage.setAlarm(now + CONNECTION_TIMEOUT_MS + CLOCK_SKEW_MS);
  }

  async alarm(): Promise<void> {
    const sql = this.ctx.storage.sql, now = Date.now();
    sql.exec(`DELETE FROM lobby_summaries WHERE code IN
      (SELECT code FROM lobby_summaries WHERE retire_at <= ? ORDER BY retire_at LIMIT 512)`, now);
    const next = sql.exec<{ deadline: number | null }>("SELECT MIN(retire_at) AS deadline FROM lobby_summaries").one().deadline;
    if (next !== null) await this.ctx.storage.setAlarm(Math.max(now + 1000, next));
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

  webSocketMessage(socket: WebSocket, data: string | ArrayBuffer): void {
    let requestedOffset: number;
    try {
      if (typeof data === "string" || data.byteLength > 16) throw new Error("Invalid directory request");
      const message = decodeClientMessage(data);
      if (message.type !== ClientMessageType.RequestLobbies) throw new Error("Expected a lobby request");
      requestedOffset = message.offset;
    } catch {
      socket.close(1008, "Invalid directory request");
      return;
    }
    const attachment = socket.deserializeAttachment() as { build: string; lastRequest: number };
    const now = Date.now();
    if (now - attachment.lastRequest < 250) {
      socket.send(encodeError("Please wait before refreshing lobbies", 1024));
      return;
    }
    socket.serializeAttachment({ ...attachment, lastRequest: now });
    try {
      const sql = this.ctx.storage.sql;
      const { total } = sql.exec<{ total: number }>(`SELECT COUNT(*) AS total FROM
        (SELECT 1 FROM lobby_summaries WHERE build=? AND visible_until>? LIMIT ?)`, attachment.build, now, MAX_RESULTS).one();
      const offset = Math.min(requestedOffset, Math.max(0, Math.floor((total - 1) / PAGE_SIZE) * PAGE_SIZE));
      const rows = sql.exec<{ entry: string }>(`SELECT entry FROM lobby_summaries
        WHERE build=? AND visible_until>? ORDER BY code LIMIT ? OFFSET ?`,
        attachment.build, now, Math.min(PAGE_SIZE, total - offset), offset);
      socket.send(encodeLobbyDirectory(offset, total, rows.toArray().map(row => JSON.parse(row.entry) as LobbyEntry)));
    } catch (error) {
      console.error("Reading lobby directory", error);
      socket.send(encodeError("Lobby browser unavailable", 1024));
    }
  }

  webSocketClose(socket: WebSocket): void { socket.close(1000, "Browser closed"); }
  webSocketError(socket: WebSocket): void { socket.close(1011, "Browser disconnected"); }
}
