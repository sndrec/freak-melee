export class HttpError extends Error {
  constructor(public status: number, message: string) { super(message); }
}

export function json(value: unknown, status = 200): Response {
  return Response.json(value, { status, headers: {
    "Cache-Control": "no-store", "X-Content-Type-Options": "nosniff",
  } });
}

export async function boundedBytes(request: Request, maximum: number): Promise<Uint8Array<ArrayBuffer>> {
  const length = Number(request.headers.get("Content-Length"));
  if (!Number.isSafeInteger(length) || length <= 0 || length > maximum || !request.body)
    throw new HttpError(length > maximum ? 413 : 400, "Invalid request size.");
  const output = new Uint8Array(length);
  const reader = request.body.getReader();
  let offset = 0;
  try {
    for (;;) {
      const chunk = await reader.read();
      if (chunk.done) break;
      if (offset + chunk.value.byteLength > length) {
        await reader.cancel();
        throw new HttpError(413, "Request exceeds its declared size.");
      }
      output.set(chunk.value, offset);
      offset += chunk.value.byteLength;
    }
  } finally { reader.releaseLock(); }
  if (offset !== length) throw new HttpError(400, "The upload was incomplete.");
  return output;
}

export async function readJson(request: Request, maximum = 128 * 1024): Promise<Record<string, unknown>> {
  if (!request.headers.get("Content-Type")?.startsWith("application/json"))
    throw new HttpError(415, "Expected JSON.");
  let value: unknown;
  try { value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(await boundedBytes(request, maximum))); }
  catch (error) {
    if (error instanceof HttpError) throw error;
    throw new HttpError(400, "Invalid JSON.");
  }
  if (!value || typeof value !== "object" || Array.isArray(value))
    throw new HttpError(400, "Expected an object.");
  return value as Record<string, unknown>;
}

export function text(value: unknown, maximum: number, allowEmpty = false): string {
  if (typeof value !== "string" || value.length > maximum ||
      (!allowEmpty && !value.trim()) || /[\u0000-\u0008\u000b\u000c\u000e-\u001f]/u.test(value))
    throw new HttpError(400, "Invalid text field.");
  return value.trim();
}

export function id(value: string): string {
  if (!/^[a-f0-9-]{36}$/u.test(value)) throw new HttpError(400, "Invalid ID.");
  return value;
}

export async function digest(bytes: Uint8Array<ArrayBuffer>): Promise<string> {
  return Array.from(new Uint8Array(await crypto.subtle.digest("SHA-256", bytes)),
    byte => byte.toString(16).padStart(2, "0")).join("");
}

export async function conditionalJson(request: Request, value: unknown): Promise<Response> {
  const bytes = new TextEncoder().encode(JSON.stringify(value));
  const etag = `"${await digest(bytes)}"`;
  const headers = { ETag: etag, "Cache-Control": "no-cache", "Content-Type": "application/json; charset=utf-8" };
  return request.headers.get("If-None-Match") === etag ? new Response(null, { status: 304, headers })
    : new Response(bytes, { headers });
}
