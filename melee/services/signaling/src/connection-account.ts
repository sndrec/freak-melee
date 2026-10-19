import { accountHandle } from "./account-protocol";
import type { SocialProfile } from "./social-protocol";

// Internal socket attachment data. The session reference is a hash; it is
// never a public identity field or a bearer token.
export interface ConnectionAccount {
  profile: SocialProfile; session: string; epoch: number; expires: number;
}
export const VERIFIED_ACCOUNT_HEADER = "X-Freak-Verified-Account";
export function connectionAccount(request: Request): ConnectionAccount | undefined {
  const value = request.headers.get(VERIFIED_ACCOUNT_HEADER);
  return value ? JSON.parse(value) as ConnectionAccount : undefined;
}
export async function authenticatedConnection(request: Request, env: CloudflareBindings): Promise<Request | Response> {
  const headers = new Headers(request.headers);
  // Only this entry point can set the verified identity passed to an object.
  headers.delete(VERIFIED_ACCOUNT_HEADER);
  const authorization = headers.get("Authorization"), supplied = headers.get("X-Freak-Account");
  headers.delete("Authorization"); headers.delete("X-Freak-Account");
  if (authorization || supplied) {
    const url = new URL(request.url);
    if (url.protocol !== "https:" && !["localhost", "127.0.0.1", "[::1]"].includes(url.hostname))
      return Response.json({ error: "Account connections require HTTPS." }, { status: 403 });
    const handle = supplied ? accountHandle(supplied) : null;
    if (!handle || !authorization) return Response.json({ error: "Please sign in again." }, { status: 401 });
    const result = await env.ACCOUNTS.getByName(handle).openConnection(authorization);
    if (!result) return Response.json({ error: "Please sign in again." }, { status: 401 });
    headers.set(VERIFIED_ACCOUNT_HEADER, JSON.stringify(result));
  }
  return new Request(request, { headers });
}
export async function currentConnectionAccount(account: ConnectionAccount, env: CloudflareBindings): Promise<SocialProfile | null> {
  return env.ACCOUNTS.getByName(account.profile.handle).checkConnection(account);
}
