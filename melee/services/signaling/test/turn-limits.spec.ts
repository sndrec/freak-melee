import { env } from "cloudflare:workers";
import { afterEach, describe, expect, it, vi } from "vitest";
import signaling from "../src/index";

afterEach(() => vi.restoreAllMocks());

describe("TURN credential admission", () => {
  function bindings(network: boolean, total: boolean) {
    return {
      ...env,
      TURN_KEY_ID: "test-key",
      TURN_KEY_API_TOKEN: "test-secret",
      TURN_NETWORK_LIMIT: { limit: vi.fn(async () => ({ success: network })) },
      TURN_TOTAL_LIMIT: { limit: vi.fn(async () => ({ success: total })) },
    };
  }

  it.each([[false, true], [true, false]])("blocks issuance before contacting the paid provider (%s/%s)", async (network, total) => {
    const upstream = vi.spyOn(globalThis, "fetch");
    const config = bindings(network, total);
    const response = await signaling.fetch(new Request("https://signal.test/v1/turn-credentials", {
      headers: { "CF-Connecting-IP": "2001:db8::1", "X-Forwarded-For": "spoofed-address" },
    }), config);
    expect(response.status).toBe(429);
    expect(response.headers.get("Retry-After")).toBe("60");
    expect(response.headers.get("Cache-Control")).toBe("no-store");
    expect(config.TURN_NETWORK_LIMIT.limit).toHaveBeenCalledWith({ key: "2001:db8::1" });
    expect(config.TURN_TOTAL_LIMIT.limit).toHaveBeenCalledTimes(network ? 1 : 0);
    expect(upstream).not.toHaveBeenCalled();
  });

  it("allows a guest through both limits without an account", async () => {
    const config = bindings(true, true);
    const upstream = vi.spyOn(globalThis, "fetch").mockResolvedValue(Response.json({ iceServers: [{
      urls: ["turn:turn.cloudflare.com:3478?transport=udp"], username: "temporary-user", credential: "temporary-password",
    }] }));
    const response = await signaling.fetch(new Request("https://signal.test/v1/turn-credentials"), config);
    expect(response.status).toBe(200);
    expect(config.TURN_TOTAL_LIMIT.limit).toHaveBeenCalledWith({ key: "issuance" });
    expect(upstream).toHaveBeenCalledTimes(1);
    expect(response.headers.get("Cache-Control")).toBe("no-store");
  });

  it("rejects insecure remote requests before admission or credential issuance", async () => {
    const config = bindings(true, true);
    const response = await signaling.fetch(new Request("http://signal.test/v1/turn-credentials"), config);
    expect(response.status).toBe(403);
    expect(config.TURN_NETWORK_LIMIT.limit).not.toHaveBeenCalled();
  });
});
