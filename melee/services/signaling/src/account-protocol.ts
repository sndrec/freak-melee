import { isoBase64URL } from "@simplewebauthn/server/helpers";
import type { AuthenticationResponseJSON, RegistrationResponseJSON } from "@simplewebauthn/server";

export const CHALLENGE_MS = 120_000;
export const SESSION_MS = 30 * 24 * 60 * 60_000;
export const RECENT_AUTH_MS = 5 * 60_000;
export const MAX_PASSKEYS = 8;
export const MAX_SESSIONS = 16;
export type Ceremony = "register" | "login" | "add" | "recover" | "reauth";
export type JsonObject = Record<string, unknown>;

export class AccountError extends Error {
  constructor(readonly status: number, message: string) { super(message); }
}
export function accountJson(value: unknown, status = 200): Response {
  return Response.json(value, { status, headers: {
    "Cache-Control": "no-store", "X-Content-Type-Options": "nosniff",
  } });
}
export function accountHandle(value: string): string | null {
  return /^[A-Za-z0-9_]{3,24}$/.test(value) ? value.toLowerCase() : null;
}
export function secret(): string {
  return isoBase64URL.fromBuffer(crypto.getRandomValues(new Uint8Array(32)));
}
export function secretValue(value: unknown): string {
  if (typeof value !== "string" || !/^[A-Za-z0-9_-]{43}$/.test(value) ||
      isoBase64URL.fromBuffer(isoBase64URL.toBuffer(value)) !== value)
    throw new AccountError(400, "Invalid sign-in code.");
  return value;
}
export async function secretHash(value: string): Promise<string> {
  return isoBase64URL.fromBuffer(new Uint8Array(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(value))));
}
export function equalHash(left: string, right: string): boolean {
  return crypto.subtle.timingSafeEqual(new TextEncoder().encode(left), new TextEncoder().encode(right));
}
export function text(value: unknown, maximum: number, multiline = false): string {
  if (typeof value !== "string" || new TextEncoder().encode(value).length > maximum ||
      (multiline ? /[\x00-\x08\x0b-\x1f\x7f]/ : /[\x00-\x1f\x7f]/).test(value))
    throw new AccountError(400, "Text is too long or contains unsupported control characters.");
  return value.trim();
}
export async function readAccountBody(request: Request): Promise<JsonObject> {
  if (request.headers.get("Content-Type")?.split(";")[0].trim() !== "application/json")
    throw new AccountError(415, "Expected JSON.");
  const reader = request.body?.getReader();
  if (!reader) throw new AccountError(400, "Missing request body.");
  const buffer = new Uint8Array(64 * 1024);
  let length = 0;
  try {
    for (;;) {
      const { done, value } = await reader.read();
      if (done) break;
      if (length + value.length > buffer.length) throw new AccountError(413, "Request is too large.");
      buffer.set(value, length); length += value.length;
    }
    const value: unknown = JSON.parse(new TextDecoder("utf-8", { fatal: true, ignoreBOM: true }).decode(buffer.subarray(0, length)));
    if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("object expected");
    return value as JsonObject;
  } catch (error) {
    await reader.cancel();
    if (error instanceof AccountError) throw error;
    throw new AccountError(400, "Invalid JSON request.");
  } finally { reader.releaseLock(); }
}

export function credentialResponse(body: JsonObject, register: true): RegistrationResponseJSON;
export function credentialResponse(body: JsonObject, register: false): AuthenticationResponseJSON;
export function credentialResponse(body: JsonObject, register: boolean): RegistrationResponseJSON | AuthenticationResponseJSON {
  const value = body.credential;
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new AccountError(400, "Missing passkey response.");
  const credential = value as JsonObject, response = credential.response as JsonObject | undefined;
  if (credential.type !== "public-key" || typeof credential.id !== "string" ||
      !/^[A-Za-z0-9_-]{1,1400}$/.test(credential.id) || credential.rawId !== credential.id ||
      !response || typeof response !== "object" || Array.isArray(response) ||
      typeof response.clientDataJSON !== "string" || response.clientDataJSON.length > 8192 ||
      (register ? typeof response.attestationObject !== "string" || response.attestationObject.length > 32768
        : typeof response.authenticatorData !== "string" || response.authenticatorData.length > 8192 ||
          typeof response.signature !== "string" || response.signature.length > 4096))
    throw new AccountError(400, "Invalid passkey response.");
  try {
    const client: unknown = JSON.parse(isoBase64URL.toUTF8String(response.clientDataJSON));
    if (!client || typeof client !== "object" || ("crossOrigin" in client && client.crossOrigin !== false)) throw new Error();
  } catch { throw new AccountError(400, "Invalid passkey client data."); }
  // Cryptographic, format, RP, origin and challenge validation is performed
  // by SimpleWebAuthn immediately after this bounded transport check.
  const common = { id: credential.id, rawId: credential.id, type: "public-key" as const,
    clientExtensionResults: {} };
  if (register) return { ...common, response: {
    clientDataJSON: response.clientDataJSON, attestationObject: String(response.attestationObject),
  } };
  if (response.userHandle !== undefined && response.userHandle !== null && typeof response.userHandle !== "string")
    throw new AccountError(400, "Invalid passkey user handle.");
  return { ...common, response: { clientDataJSON: response.clientDataJSON,
    authenticatorData: String(response.authenticatorData), signature: String(response.signature),
    userHandle: typeof response.userHandle === "string" ? response.userHandle : undefined,
  } };
}
