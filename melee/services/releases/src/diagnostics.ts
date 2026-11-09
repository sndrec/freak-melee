const maxReportBytes = 256 * 1024;
const reportHeader = "freak-melee-netplay-report\n";

export async function storeNetplayReport(request: Request, env: CloudflareBindings,
                                         session: string, peer: string): Promise<Response> {
  if (request.method !== "POST") return new Response(null, { status: 405 });
  const length = Number(request.headers.get("Content-Length"));
  if (!Number.isSafeInteger(length) || length <= 0 || request.body === null) {
    return new Response(null, { status: 400 });
  }
  if (length > maxReportBytes) return new Response(null, { status: 413 });
  const limit = await env.REPORT_LIMIT.limit({
    key: request.headers.get("CF-Connecting-IP") ?? "local",
  });
  if (!limit.success) return new Response(null, { status: 429 });

  const bytes = new Uint8Array(length);
  const reader = request.body.getReader();
  let offset = 0;
  try {
    for (;;) {
      const chunk = await reader.read();
      if (chunk.done) break;
      if (chunk.value.byteLength > length - offset) {
        await reader.cancel();
        return new Response(null, { status: 413 });
      }
      bytes.set(chunk.value, offset);
      offset += chunk.value.byteLength;
    }
  } finally {
    reader.releaseLock();
  }
  const header = new TextDecoder().decode(bytes.subarray(0, 512));
  if (offset !== length || !header.startsWith(reportHeader) ||
      !header.includes(`\nsession=${session} peer=${peer} `)) {
    return new Response(null, { status: 400 });
  }

  await env.RELEASES.put(`netplay-diagnostics/${session}/${peer}.log`, bytes, {
    onlyIf: { etagDoesNotMatch: "*" },
    httpMetadata: { contentType: "text/plain; charset=utf-8" },
  });
  return new Response(null, { status: 204 });
}
