import { Buffer } from "node:buffer";

export const CHALLENGE_MS = 120_000;
export const SESSION_MS = 30 * 24 * 60 * 60_000;
export const RECENT_AUTH_MS = 5 * 60_000;
export const MAX_ACCOUNT_KEYS = 8;
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
  return Buffer.from(crypto.getRandomValues(new Uint8Array(32))).toString("base64url");
}
export function secretValue(value: unknown): string {
  if (typeof value !== "string" || !/^[A-Za-z0-9_-]{43}$/.test(value) ||
      Buffer.from(value, "base64url").toString("base64url") !== value)
    throw new AccountError(400, "Invalid sign-in code.");
  return value;
}
export async function secretHash(value: string): Promise<string> {
  return Buffer.from(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(value))).toString("base64url");
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
