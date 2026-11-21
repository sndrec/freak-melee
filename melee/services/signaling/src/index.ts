import { isRoomId } from "./protocol";
import { turnCredentialsResponse } from "./turn";
import { accountHandle, accountJson } from "./account-protocol";
import { authenticatedConnection } from "./connection-account";

export { Room } from "./room";
export { GlobalChat } from "./chat";
export { QuickPlayQueue } from "./quick-queue";
export { LobbyDirectory } from "./lobby-directory";
export { Account } from "./account";
export { SocialMailbox } from "./social";
export { SocialFriendship } from "./social-friendship";
export { PlayerSession } from "./player-session";

function jsonError(message: string, status: number): Response {
  return Response.json({ error: message }, { status });
}

export default {
  async fetch(request: Request, env: CloudflareBindings): Promise<Response> {
    try {
      const url = new URL(request.url);
      if (request.method === "GET" && url.pathname === "/health") {
        return Response.json({ service: "freak-melee-signaling", status: "ok" });
      }
      const account = /^\/v1\/accounts\/([^/]+)\/(.+)$/.exec(url.pathname);
      if (account) {
        if (url.protocol !== "https:" && !["localhost", "127.0.0.1", "[::1]"].includes(url.hostname))
          return accountJson({ error: "Account requests require HTTPS." }, 403);
        const handle = accountHandle(account[1]);
        if (!handle) return accountJson({ error: "Use 3 to 24 letters, numbers or underscores for your account name." }, 400);
        if (request.method !== "GET" && request.method !== "POST") return accountJson({ error: "Method not allowed." }, 405);
        // Authentication attempts share a generous per-network limit. Neither
        // addresses nor raw credentials are written into account records.
        const limit = await env.ACCOUNT_LIMIT.limit({ key: `account:${request.headers.get("CF-Connecting-IP") ?? "local"}` });
        if (!limit.success) return accountJson({ error: "Too many account requests. Please wait a minute." }, 429);
        return env.ACCOUNTS.getByName(handle).fetch(request);
      }
      if (url.pathname === "/v1/turn-credentials") {
        if (request.method !== "GET") return jsonError("Method not allowed", 405);
        if (url.protocol !== "https:" && !["localhost", "127.0.0.1", "[::1]"].includes(url.hostname))
          return jsonError("TURN credentials require HTTPS", 403);
        // Guests can play online. Limit issuance without making an account a
        // prerequisite. Cloudflare supplies this address at the edge; never
        // trust an X-Forwarded-For value supplied by a client.
        const network = request.headers.get("CF-Connecting-IP") ?? "local";
        const allowed = await env.TURN_NETWORK_LIMIT.limit({ key: network });
        const total = allowed.success && await env.TURN_TOTAL_LIMIT.limit({ key: "issuance" });
        if (!allowed.success || !total || !total.success) {
          return Response.json({ error: "Too many connection requests. Please wait a minute." }, {
            status: 429, headers: { "Cache-Control": "no-store", "Retry-After": "60" },
          });
        }
        return turnCredentialsResponse(env);
      }

      const chat = url.pathname === "/chat/connect";
      const directory = url.pathname === "/v1/lobbies/connect";
      const match = /^\/v1\/rooms\/([^/]+)\/connect$/.exec(url.pathname);
      if (!chat && !directory && !match) {
        return jsonError("Not found", 404);
      }
      if (request.method !== "GET") {
        return jsonError("Method not allowed", 405);
      }
      if (request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
        return jsonError("Expected a WebSocket upgrade", 426);
      }
      const roomId = match?.[1] ?? "";
      if (match && !isRoomId(roomId)) {
        return jsonError("Invalid room ID", 400);
      }
      const network = request.headers.get("CF-Connecting-IP") ?? "local";
      if (!(await env.SOCKET_NETWORK_LIMIT.limit({ key: network })).success) {
        return Response.json({ error: "Too many connection attempts. Please wait a minute." }, {
          status: 429, headers: { "Retry-After": "60", "Cache-Control": "no-store" },
        });
      }
      if (directory) return env.LOBBY_DIRECTORY.getByName("public").fetch(request);
      if (match && request.headers.has("X-Freak-Lobby-Password")) {
        if (url.protocol !== "https:" && !["localhost", "127.0.0.1", "[::1]"].includes(url.hostname))
          return jsonError("Lobby passwords require HTTPS.", 403);
        const limit = await env.ACCOUNT_LIMIT.limit({ key: `lobby:${roomId}:${network}` });
        if (!limit.success) return jsonError("Please wait before trying the lobby password again.", 429);
      }

      const authenticated = await authenticatedConnection(request, env);
      if (authenticated instanceof Response) return authenticated;
      if (chat) return env.GLOBAL_CHAT.getByName("friends").fetch(authenticated);
      return env.ROOMS.getByName(roomId).fetch(authenticated);
    } catch (error) {
      console.error(
        JSON.stringify({
          message: "signaling request failed",
          error: error instanceof Error ? error.message : String(error),
        }),
      );
      return jsonError("Internal server error", 500);
    }
  },
} satisfies ExportedHandler<CloudflareBindings>;
