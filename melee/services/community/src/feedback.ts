import { boundedBytes, digest, HttpError, id, json, readJson, text } from "./http";

// A report is FMREPORT + three little-endian uint32 sizes, then UTF-8 metadata,
// the PNG, and the immutable diagnostic capture. Limits are shared with client.
const maximumMetadata = 512 * 1024;
const maximumImage = 16 * 1024 * 1024;
const maximumContext = 8 * 1024 * 1024;
const maximumReport = 20 + maximumMetadata + maximumImage + maximumContext;
type Pin = { id: number; x: number; y: number; text: string };
type ReportRow = {
  id: string; capture_at: string; build: string; notes_json: string;
  width: number; height: number; created_at: number; status: string;
  screenshot_key: string; context_key: string;
  screenshot_sha256: string; context_sha256: string;
  screenshot_size: number; context_size: number;
};

function notes(value: unknown): Pin[] {
  if (!Array.isArray(value) || value.length > 64) throw new HttpError(400, "A report can contain up to 64 notes.");
  const ids = new Set<number>();
  return value.map((pin: unknown) => {
    if (!pin || typeof pin !== "object") throw new HttpError(400, "Invalid note.");
    const p = pin as Record<string, unknown>;
    if (!Number.isSafeInteger(p.id) || Number(p.id) < 0 || ids.has(Number(p.id)) ||
        typeof p.x !== "number" || !Number.isFinite(p.x) || p.x < 0 || p.x > 1 ||
        typeof p.y !== "number" || !Number.isFinite(p.y) || p.y < 0 || p.y > 1)
      throw new HttpError(400, "Invalid note position or ID.");
    ids.add(Number(p.id));
    return { id: Number(p.id), x: p.x, y: p.y, text: text(p.text, 2048) };
  });
}

function imageSize(bytes: Uint8Array): [number, number] {
  const signature = [137, 80, 78, 71, 13, 10, 26, 10];
  if (bytes.length < 33 || !signature.every((v, i) => bytes[i] === v) ||
      new TextDecoder().decode(bytes.subarray(12, 16)) !== "IHDR")
    throw new HttpError(400, "The screenshot must be a PNG.");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const w = view.getUint32(16), h = view.getUint32(20);
  if (!w || !h || w > 16384 || h > 16384 || w * h > 64000000)
    throw new HttpError(400, "Invalid screenshot dimensions.");
  return [w, h];
}

export async function submitFeedback(request: Request, env: CloudflareBindings): Promise<Response> {
  if (request.method !== "POST") throw new HttpError(405, "Use POST.");
  if (request.headers.get("Content-Type") !== "application/octet-stream")
    throw new HttpError(415, "Expected a feedback report.");
  const limit = await env.SUBMISSION_LIMIT.limit({ key: request.headers.get("CF-Connecting-IP") ?? "local" });
  if (!limit.success) throw new HttpError(429, "Please wait before sending another report.");
  const bytes = await boundedBytes(request, maximumReport);
  if (bytes.length < 20 || new TextDecoder().decode(bytes.subarray(0, 8)) !== "FMREPORT")
    throw new HttpError(400, "Invalid feedback report.");
  const sizes = new DataView(bytes.buffer, bytes.byteOffset, 20);
  const metadataSize = sizes.getUint32(8, true), imageBytes = sizes.getUint32(12, true), contextBytes = sizes.getUint32(16, true);
  if (!metadataSize || metadataSize > maximumMetadata || !imageBytes || imageBytes > maximumImage ||
      !contextBytes || contextBytes > maximumContext || 20 + metadataSize + imageBytes + contextBytes !== bytes.length)
    throw new HttpError(400, "Invalid report attachment sizes.");
  let value: Record<string, unknown>;
  try { value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(bytes.subarray(20, 20 + metadataSize))); }
  catch { throw new HttpError(400, "Invalid feedback metadata."); }
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new HttpError(400, "Invalid feedback metadata.");
  const reportId = id(text(value.id, 36));
  const captured = text(value.capture_at, 40);
  if (!Number.isFinite(Date.parse(captured))) throw new HttpError(400, "Invalid capture time.");
  const build = text(value.build, 128);
  const pins = JSON.stringify(notes(value.pins));
  const screenshot = bytes.subarray(20 + metadataSize, 20 + metadataSize + imageBytes);
  const context = bytes.subarray(20 + metadataSize + imageBytes);
  const [width, height] = imageSize(screenshot);
  const imageHash = await digest(screenshot), contextHash = await digest(context);
  const previous = await env.COMMUNITY.prepare("SELECT * FROM feedback WHERE id=?").bind(reportId).first<ReportRow>();
  function matches(row: ReportRow): boolean {
    return row.screenshot_sha256 === imageHash && row.context_sha256 === contextHash &&
      row.notes_json === pins && row.capture_at === captured && row.build === build;
  }
  if (previous) {
    if (!matches(previous)) throw new HttpError(409, "This report ID already belongs to a different capture.");
    return json({ id: reportId, stored: true });
  }
  const prefix = `feedback/${reportId}/${crypto.randomUUID()}`;
  const imageKey = `${prefix}/screen.png`, contextKey = `${prefix}/context.bin`;
  let committed = false;
  try {
    await env.REPORTS.put(imageKey, screenshot, { httpMetadata: { contentType: "image/png" } });
    await env.REPORTS.put(contextKey, context, { httpMetadata: { contentType: "application/octet-stream" } });
    const result = await env.COMMUNITY.prepare(
      "INSERT OR IGNORE INTO feedback(id,capture_at,build,created_at,screenshot_key,context_key,notes_json,screenshot_sha256,context_sha256,screenshot_size,context_size,width,height) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)"
    ).bind(reportId, captured, build, Date.now(), imageKey, contextKey, pins, imageHash, contextHash,
      imageBytes, contextBytes, width, height).run();
    committed = result.meta.changes === 1;
    if (!committed) {
      const winner = await env.COMMUNITY.prepare("SELECT * FROM feedback WHERE id=?").bind(reportId).first<ReportRow>();
      if (!winner || !matches(winner)) throw new HttpError(409, "This report ID was submitted with different content.");
    }
    return json({ id: reportId, stored: true }, committed ? 201 : 200);
  } finally {
    if (!committed) await env.REPORTS.delete([imageKey, contextKey]);
  }
}

export async function adminFeedback(request: Request, env: CloudflareBindings): Promise<Response> {
  const url = new URL(request.url);
  if (url.pathname === "/admin/api/feedback") {
    if (request.method !== "GET") throw new HttpError(405, "Use GET.");
    const before = url.searchParams.get("before")?.split(",");
    const date = before ? Number(before[0]) : Number.MAX_SAFE_INTEGER;
    const key = before ? id(before[1] ?? "") : "ffffffff-ffff-ffff-ffff-ffffffffffff";
    if (!Number.isSafeInteger(date) || date < 0) throw new HttpError(400, "Invalid history page.");
    const status = url.searchParams.get("status") ?? "";
    const search = text(url.searchParams.get("search") ?? "", 120, true);
    if (!["", "new", "investigating", "resolved"].includes(status)) throw new HttpError(400, "Invalid status filter.");
    const page = await env.COMMUNITY.prepare(
      "SELECT id,capture_at,build,created_at,status,width,height,screenshot_size,context_size FROM feedback WHERE (?='' OR status=?) AND instr(lower(build),lower(?))>0 AND (created_at<? OR (created_at=? AND id<?)) ORDER BY created_at DESC,id DESC LIMIT 26"
    ).bind(status, status, search, date, date, key).all<ReportRow>();
    const entries = page.results.slice(0, 25), last = entries.at(-1);
    const usage = await env.COMMUNITY.prepare("SELECT count(*) reports,coalesce(sum(screenshot_size+context_size),0) bytes FROM feedback").first();
    return json({ entries, usage, next: page.results.length > 25 && last ? `${last.created_at},${last.id}` : null });
  }
  const match = /^\/admin\/api\/feedback\/([^/]+)(?:\/(screen|context))?$/u.exec(url.pathname);
  if (!match) throw new HttpError(404, "Report not found.");
  const key = id(match[1]);
  const report = await env.COMMUNITY.prepare("SELECT * FROM feedback WHERE id=?").bind(key).first<ReportRow>();
  if (!report) {
    if (request.method === "DELETE" && !match[2]) return json({ deleted: true });
    throw new HttpError(404, "Report not found.");
  }
  if (request.method === "GET") {
    if (!match[2]) return json({ ...report, notes: JSON.parse(report.notes_json) });
    const image = match[2] === "screen";
    const object = await env.REPORTS.get(image ? report.screenshot_key : report.context_key);
    if (!object) throw new HttpError(404, "Report attachment not found.");
    return new Response(object.body, { headers: {
      "Content-Type": image ? "image/png" : "application/octet-stream", "Cache-Control": "no-store",
      "X-Content-Type-Options": "nosniff",
      "Content-Disposition": `${image ? "inline" : "attachment"}; filename="${key}-${image ? "screen.png" : "context.bin"}"`,
    } });
  }
  if (match[2]) throw new HttpError(405, "Use GET for report attachments.");
  if (request.method === "POST") {
    const input = await readJson(request, 1024);
    const state = text(input.status, 20);
    if (!["new", "investigating", "resolved"].includes(state)) throw new HttpError(400, "Invalid report status.");
    await env.COMMUNITY.prepare("UPDATE feedback SET status=? WHERE id=?").bind(state, key).run();
    return json({ id: key, status: state });
  }
  if (request.method === "DELETE") {
    // Remove the visible record first. A failed object delete can be retried;
    // the scheduled orphan scan also completes cleanup after an interruption.
    await env.COMMUNITY.prepare("DELETE FROM feedback WHERE id=?").bind(key).run();
    await env.REPORTS.delete([report.screenshot_key, report.context_key]);
    return json({ deleted: true });
  }
  throw new HttpError(405, "Method not allowed.");
}
