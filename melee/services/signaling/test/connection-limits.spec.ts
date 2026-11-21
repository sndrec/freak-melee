import { env } from "cloudflare:workers";
import { expect, it, vi } from "vitest";
import signaling from "../src/index";

it("shares one handshake budget across chat, directory and rooms before account or room access", async () => {
  const limit = vi.fn(async () => ({ success: false }));
  const getByName = vi.fn();
  const config = { ...env, SOCKET_NETWORK_LIMIT: { limit }, ACCOUNTS: { getByName },
    GLOBAL_CHAT: { getByName }, LOBBY_DIRECTORY: { getByName }, ROOMS: { getByName } } as unknown as CloudflareBindings;
  for (const path of ["/chat/connect", "/v1/lobbies/connect", "/v1/rooms/ROOMCODE/connect"]) {
    const response = await signaling.fetch(new Request("https://signal.test" + path, {
      headers: { Upgrade: "websocket", "CF-Connecting-IP": "2001:db8::2" },
    }), config);
    expect(response.status).toBe(429);
    expect(response.headers.get("Retry-After")).toBe("60");
  }
  expect(limit.mock.calls).toEqual(Array.from({ length: 3 }, () => [{ key: "2001:db8::2" }]));
  expect(getByName).not.toHaveBeenCalled();
});
