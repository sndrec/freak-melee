export const HEARTBEAT_MS = 10_000;
export const CONNECTION_TIMEOUT_MS = 45_000;
export const CONNECTION_RECOVERY_MS = 120_000;
export const CONNECTION_LIFETIME_MS = CONNECTION_TIMEOUT_MS + CONNECTION_RECOVERY_MS;

export interface ConnectionLease {
  lastSeenAt: number;
  uncertain: boolean;
}

export function connectionUncertain(lease: ConnectionLease, now = Date.now()): boolean {
  return now >= lease.lastSeenAt + CONNECTION_TIMEOUT_MS;
}

export function connectionExpired(lease: ConnectionLease, now = Date.now()): boolean {
  return now >= lease.lastSeenAt + CONNECTION_LIFETIME_MS;
}

// Account IDs and guest keys are private transport identities. Display names
// are deliberately absent: different players can use the same display name.
export async function playerIdentity(request: Request, accountId?: string): Promise<string | null> {
  if (accountId) return `account:${accountId}`;
  const key = request.headers.get("X-Freak-Guest");
  if (!key || !/^[a-f0-9]{64}$/.test(key)) return null;
  const url = new URL(request.url);
  if (url.protocol !== "https:" && !["localhost", "127.0.0.1", "[::1]"].includes(url.hostname)) return null;
  const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(key));
  return "guest:" + [...new Uint8Array(digest)].map(value => value.toString(16).padStart(2, "0")).join("");
}
