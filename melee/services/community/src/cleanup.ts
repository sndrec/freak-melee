// Scan a bounded page each run. New uploads have a full day to finish before
// an unreferenced object can be collected; completed reports have no expiry.
export async function cleanAbandonedAttachments(env: CloudflareBindings): Promise<void> {
  await env.COMMUNITY.prepare("DELETE FROM feedback_uploads WHERE expires_at<=unixepoch()*1000").run();
  const saved = await env.COMMUNITY.prepare("SELECT value FROM service_state WHERE key='attachment-scan'")
    .first<{ value: string }>();
  const page = await env.REPORTS.list({ prefix: "feedback/", limit: 100,
    cursor: saved?.value || undefined });
  const cutoff = Date.now() - 86400000;
  const candidates = page.objects.filter(object => object.uploaded.getTime() < cutoff);
  if (candidates.length) {
    const references = await env.COMMUNITY.batch(candidates.map(object => env.COMMUNITY.prepare(
      "SELECT id FROM feedback WHERE screenshot_key=? OR context_key=? LIMIT 1"
    ).bind(object.key, object.key)));
    const abandoned = candidates.filter((_object, index) => !references[index].results.length);
    if (abandoned.length) await env.REPORTS.delete(abandoned.map(object => object.key));
  }
  await env.COMMUNITY.prepare("INSERT INTO service_state(key,value) VALUES('attachment-scan',?) ON CONFLICT(key) DO UPDATE SET value=excluded.value")
    .bind(page.truncated ? page.cursor : "").run();
}
