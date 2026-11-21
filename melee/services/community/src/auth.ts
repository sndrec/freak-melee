import { createRemoteJWKSet, jwtVerify } from "jose";
import { HttpError } from "./http";

// Only public verification keys are cached across requests.
const keys = new Map<string, ReturnType<typeof createRemoteJWKSet>>();

export async function requireAdmin(request: Request, env: CloudflareBindings): Promise<void> {
  const domain = env.ACCESS_TEAM_DOMAIN;
  if (!/^[a-z0-9-]+\.cloudflareaccess\.com$/u.test(domain) || !env.ACCESS_AUDIENCE)
    throw new HttpError(503, "Administrator sign-in has not been configured.");
  const token = request.headers.get("Cf-Access-Jwt-Assertion");
  if (!token) throw new HttpError(401, "Sign in to access administration.");
  let set = keys.get(domain);
  if (!set) {
    set = createRemoteJWKSet(new URL(`https://${domain}/cdn-cgi/access/certs`));
    keys.set(domain, set);
  }
  try {
    const { payload } = await jwtVerify(token, set, {
      issuer: `https://${domain}`, audience: env.ACCESS_AUDIENCE,
      requiredClaims: ["exp"],
      algorithms: ["RS256"],
    });
    if (typeof payload.email !== "string" ||
        payload.email.toLowerCase() !== env.ADMIN_EMAIL.toLowerCase())
      throw new Error("Unapproved administrator.");
  } catch { throw new HttpError(403, "Administrator access denied."); }
  if (!["GET", "HEAD"].includes(request.method)) {
    if (request.headers.get("Origin") !== new URL(request.url).origin ||
        request.headers.get("X-Freak-Admin") !== "1")
      throw new HttpError(403, "Reload administration before submitting this change.");
  }
}
