import { isRoomId } from "./protocol";
import { turnCredentialsResponse } from "./turn";

export { Room } from "./room";
export { GlobalChat } from "./chat";
export { QuickPlayQueue } from "./quick-queue";

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
      if (url.pathname === "/v1/turn-credentials") {
        return request.method === "GET"
          ? turnCredentialsResponse(env)
          : jsonError("Method not allowed", 405);
      }

      if (url.pathname === "/chat/connect") {
        return env.GLOBAL_CHAT.getByName("friends").fetch(request);
      }

      const match = /^\/v1\/rooms\/([^/]+)\/connect$/.exec(url.pathname);
      if (match === null) {
        return jsonError("Not found", 404);
      }
      if (request.method !== "GET") {
        return jsonError("Method not allowed", 405);
      }
      if (request.headers.get("Upgrade")?.toLowerCase() !== "websocket") {
        return jsonError("Expected a WebSocket upgrade", 426);
      }
      const roomId = match[1];
      if (!isRoomId(roomId)) {
        return jsonError("Invalid room ID", 400);
      }

      return env.ROOMS.getByName(roomId).fetch(request);
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
