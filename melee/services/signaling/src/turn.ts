const TURN_CREDENTIAL_TTL_SECONDS = 86_400;
const TURN_WIRE_HEADER_BYTES = 20;
const TURN_WIRE_VERSION = 1;
const TURN_FLAG_RELAY = 1;
const STUN_HOST = "stun.cloudflare.com";
const STUN_PORT = 3478;

interface IceServerResponse {
  urls: string[];
  username?: string;
  credential?: string;
}

interface TurnConfiguration {
  stunHost: string;
  stunPort: number;
  turnHost?: string;
  turnPort?: number;
  username?: string;
  password?: string;
  ttlSeconds: number;
}

export interface TurnBindings {
  TURN_KEY_ID?: string;
  TURN_KEY_API_TOKEN?: string;
}

function boundedText(value: string, required: boolean): Uint8Array | null {
  const bytes = new TextEncoder().encode(value);
  if ((required && bytes.byteLength === 0) || bytes.byteLength > 255) return null;
  return bytes;
}

function encodeConfiguration(configuration: TurnConfiguration): Uint8Array | null {
  const stunHost = boundedText(configuration.stunHost, true);
  const turnHost = boundedText(configuration.turnHost ?? "", false);
  const username = boundedText(configuration.username ?? "", false);
  const password = boundedText(configuration.password ?? "", false);
  if (stunHost === null || turnHost === null || username === null || password === null) {
    return null;
  }
  const hasTurn = turnHost.byteLength !== 0 || username.byteLength !== 0 ||
    password.byteLength !== 0;
  if (
    hasTurn &&
    (turnHost.byteLength === 0 || username.byteLength === 0 || password.byteLength === 0 ||
      configuration.turnPort === undefined)
  ) {
    return null;
  }
  const output = new Uint8Array(
    TURN_WIRE_HEADER_BYTES + stunHost.byteLength + turnHost.byteLength +
      username.byteLength + password.byteLength,
  );
  output.set(new TextEncoder().encode("FMT1"), 0);
  output[4] = TURN_WIRE_VERSION;
  output[5] = hasTurn ? TURN_FLAG_RELAY : 0;
  output[6] = stunHost.byteLength;
  output[7] = turnHost.byteLength;
  output[8] = username.byteLength;
  output[9] = password.byteLength;
  const view = new DataView(output.buffer);
  view.setUint32(12, configuration.ttlSeconds);
  view.setUint16(16, configuration.stunPort);
  view.setUint16(18, configuration.turnPort ?? 0);
  let offset = TURN_WIRE_HEADER_BYTES;
  for (const field of [stunHost, turnHost, username, password]) {
    output.set(field, offset);
    offset += field.byteLength;
  }
  return output;
}

function iceServers(value: unknown): IceServerResponse[] | null {
  if (typeof value !== "object" || value === null || !("iceServers" in value) ||
      !Array.isArray(value.iceServers)) {
    return null;
  }
  const result: IceServerResponse[] = [];
  for (const candidate of value.iceServers) {
    if (typeof candidate !== "object" || candidate === null || !("urls" in candidate)) {
      return null;
    }
    const urls = typeof candidate.urls === "string"
      ? [candidate.urls]
      : Array.isArray(candidate.urls) && candidate.urls.every((url: unknown) => typeof url === "string")
      ? candidate.urls
      : null;
    if (urls === null) return null;
    result.push({
      urls,
      username: "username" in candidate && typeof candidate.username === "string"
        ? candidate.username
        : undefined,
      credential: "credential" in candidate && typeof candidate.credential === "string"
        ? candidate.credential
        : undefined,
    });
  }
  return result;
}

function udpTurnServer(servers: IceServerResponse[]): TurnConfiguration | null {
  for (const server of servers) {
    if (server.username === undefined || server.credential === undefined) continue;
    for (const url of server.urls) {
      const match = /^turn:([^:/?#]+):(\d+)\?transport=udp$/i.exec(url);
      if (match === null) continue;
      const port = Number(match[2]);
      if (!Number.isInteger(port) || port < 1 || port > 65_535) continue;
      return {
        stunHost: STUN_HOST,
        stunPort: STUN_PORT,
        turnHost: match[1],
        turnPort: port,
        username: server.username,
        password: server.credential,
        ttlSeconds: TURN_CREDENTIAL_TTL_SECONDS,
      };
    }
  }
  return null;
}

function binaryResponse(bytes: Uint8Array): Response {
  return new Response(bytes, {
    headers: {
      "Cache-Control": "no-store",
      "Content-Type": "application/octet-stream",
    },
  });
}

export async function turnCredentialsResponse(
  env: TurnBindings,
  fetcher: typeof fetch = fetch,
): Promise<Response> {
  if (env.TURN_KEY_ID === undefined || env.TURN_KEY_API_TOKEN === undefined) {
    const stunOnly = encodeConfiguration({
      stunHost: STUN_HOST,
      stunPort: STUN_PORT,
      ttlSeconds: 0,
    });
    return stunOnly === null
      ? Response.json({ error: "Could not encode STUN configuration" }, { status: 500 })
      : binaryResponse(stunOnly);
  }
  let response: Response;
  try {
    response = await fetcher(
      `https://rtc.live.cloudflare.com/v1/turn/keys/${encodeURIComponent(env.TURN_KEY_ID)}` +
        "/credentials/generate-ice-servers",
      {
        method: "POST",
        headers: {
          Authorization: `Bearer ${env.TURN_KEY_API_TOKEN}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ ttl: TURN_CREDENTIAL_TTL_SECONDS }),
      },
    );
  } catch (error) {
    console.error(JSON.stringify({
      message: "TURN credential request failed",
      error: error instanceof Error ? error.message : String(error),
    }));
    return Response.json({ error: "Could not issue TURN credentials" }, { status: 502 });
  }
  if (!response.ok) {
    console.error(JSON.stringify({
      message: "TURN credential request failed",
      status: response.status,
    }));
    return Response.json({ error: "Could not issue TURN credentials" }, { status: 502 });
  }
  let body: unknown;
  try {
    body = await response.json();
  } catch {
    return Response.json({ error: "Invalid TURN credential response" }, { status: 502 });
  }
  const servers = iceServers(body);
  const configuration = servers === null ? null : udpTurnServer(servers);
  const encoded = configuration === null ? null : encodeConfiguration(configuration);
  return encoded === null
    ? Response.json({ error: "Invalid TURN credential response" }, { status: 502 })
    : binaryResponse(encoded);
}
