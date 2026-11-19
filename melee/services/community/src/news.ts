import { conditionalJson, HttpError, id, json, readJson, text } from "./http";

type NewsRow = {
  id: string; title: string; ticker: string; body: string;
  published_at: number | null; updated_at: number; revision: number; archived: number;
};
type AnnouncementRow = {
  id: string; delivery_id: string; title: string; body: string; news_id: string | null;
  published_at: number | null; expires_at: number | null;
  updated_at: number; revision: number; withdrawn: number;
};

function timestamp(value: unknown): number | null {
  if (value === null) return null;
  if (!Number.isSafeInteger(value) || Number(value) < 0 || Number(value) > 8640000000000000)
    throw new HttpError(400, "Invalid date.");
  return Number(value);
}

function expectedRevision(value: unknown): number {
  if (!Number.isSafeInteger(value) || Number(value) < 0)
    throw new HttpError(400, "Missing record revision.");
  return Number(value);
}

function cursor(url: URL): [number, string] {
  const value = url.searchParams.get("before");
  if (!value) return [Number.MAX_SAFE_INTEGER, "ffffffff-ffff-ffff-ffff-ffffffffffff"];
  const pair = value.split(",");
  const stamp = timestamp(Number(pair[0]));
  if (pair.length !== 2 || stamp === null) throw new HttpError(400, "Invalid page cursor.");
  return [stamp, id(pair[1])];
}

async function announcements(env: CloudflareBindings, url: URL, now: number, since: number) {
  const [date, key] = cursor(url);
  const page = await env.COMMUNITY.prepare(
    "SELECT id,delivery_id,title,body,news_id,published_at,expires_at,revision FROM announcements WHERE withdrawn=0 AND published_at BETWEEN ? AND ? AND (expires_at IS NULL OR expires_at>?) AND (published_at<? OR (published_at=? AND id<?)) ORDER BY published_at DESC,id DESC LIMIT 65"
  ).bind(since, now, now, date, date, key).all<AnnouncementRow>();
  const entries = page.results.slice(0, 64), last = entries.at(-1);
  return { entries, next: page.results.length > 64 && last ? `${last.published_at},${last.id}` : null };
}

export async function publicNews(request: Request, env: CloudflareBindings): Promise<Response> {
  const url = new URL(request.url);
  if (request.method !== "GET") throw new HttpError(405, "Use GET.");
  const now = Date.now();
  const since = timestamp(Number(url.searchParams.get("since"))) ?? 0;
  if (url.pathname === "/v1/feed") {
    const headline = await env.COMMUNITY.prepare(
      "SELECT id,title,ticker,published_at,updated_at,revision FROM news WHERE archived=0 AND published_at<=? ORDER BY published_at DESC,id DESC LIMIT 1"
    ).bind(now).first();
    const notices = await announcements(env, new URL(url.origin), now, since);
    return conditionalJson(request, { headline, announcements: notices.entries, announcements_next: notices.next });
  }
  if (url.pathname === "/v1/announcements")
    return conditionalJson(request, await announcements(env, url, now, since));
  const notice = /^\/v1\/announcements\/([^/]+)$/u.exec(url.pathname);
  if (notice) {
    const entry = await env.COMMUNITY.prepare(
      "SELECT id,delivery_id,title,body,news_id,published_at,expires_at,revision FROM announcements WHERE delivery_id=? AND withdrawn=0 AND published_at BETWEEN ? AND ? AND (expires_at IS NULL OR expires_at>?)"
    ).bind(id(notice[1]), since, now, now).first();
    if (!entry) throw new HttpError(404, "This announcement is unavailable.");
    return conditionalJson(request, entry);
  }
  if (url.pathname === "/v1/news") {
    const [date, key] = cursor(url);
    const page = await env.COMMUNITY.prepare(
      "SELECT id,title,ticker,published_at,updated_at,revision FROM news WHERE archived=0 AND published_at<=? AND (published_at<? OR (published_at=? AND id<?)) ORDER BY published_at DESC,id DESC LIMIT 26"
    ).bind(now, date, date, key).all<NewsRow>();
    const entries = page.results.slice(0, 25);
    const last = entries.at(-1);
    return conditionalJson(request, { entries, next: page.results.length > 25 && last ? `${last.published_at},${last.id}` : null });
  }
  const match = /^\/v1\/news\/([^/]+)$/u.exec(url.pathname);
  if (!match) throw new HttpError(404, "Page not found.");
  const article = await env.COMMUNITY.prepare(
    "SELECT id,title,ticker,body,published_at,updated_at,revision FROM news WHERE id=? AND archived=0 AND published_at<=?"
  ).bind(id(match[1]), now).first();
  if (!article) throw new HttpError(404, "This article is unavailable.");
  return conditionalJson(request, article);
}

export async function adminNews(request: Request, env: CloudflareBindings): Promise<Response> {
  const url = new URL(request.url);
  const match = /^\/admin\/api\/(news|announcements)(?:\/([^/]+))?$/u.exec(url.pathname);
  if (!match) throw new HttpError(404, "Page not found.");
  const kind = match[1];
  const key = match[2] ? id(match[2]) : null;
  if (request.method === "GET") {
    if (key) {
      const row = kind === "news"
        ? await env.COMMUNITY.prepare("SELECT * FROM news WHERE id=?").bind(key).first()
        : await env.COMMUNITY.prepare("SELECT * FROM announcements WHERE id=?").bind(key).first();
      if (!row) throw new HttpError(404, "Record not found.");
      return json(row);
    }
    const [date, before] = cursor(url);
    const search = text(url.searchParams.get("search") ?? "", 120, true);
    const query = kind === "news"
      ? "SELECT id,title,ticker,published_at,updated_at,revision,archived FROM news WHERE (updated_at<? OR (updated_at=? AND id<?)) AND instr(lower(title),lower(?))>0 ORDER BY updated_at DESC,id DESC LIMIT 26"
      : "SELECT * FROM announcements WHERE (updated_at<? OR (updated_at=? AND id<?)) AND instr(lower(title),lower(?))>0 ORDER BY updated_at DESC,id DESC LIMIT 26";
    const page = await env.COMMUNITY.prepare(query).bind(date, date, before, search).all<{ id: string; updated_at: number }>();
    const entries = page.results.slice(0, 25);
    const last = entries.at(-1);
    return json({ entries, next: page.results.length > 25 && last ? `${last.updated_at},${last.id}` : null });
  }
  if (request.method !== "POST") throw new HttpError(405, "Use POST to save.");
  const value = await readJson(request, 160 * 1024);
  const revision = expectedRevision(value.revision);
  if ((key && !revision) || (!key && revision)) throw new HttpError(400, "Invalid record revision.");
  const title = text(value.title, 120);
  const body = text(value.body, kind === "news" ? 32000 : 380);
  const publish = timestamp(value.published_at);
  const now = Date.now();
  const recordId = key ?? crypto.randomUUID();
  let result: D1Result;
  if (kind === "news") {
    const ticker = text(value.ticker ?? "", 240, true);
    if (typeof value.archived !== "boolean") throw new HttpError(400, "Choose an archive state.");
    result = key
      ? await env.COMMUNITY.prepare("UPDATE news SET title=?,ticker=?,body=?,published_at=?,updated_at=?,archived=?,revision=revision+1 WHERE id=? AND revision=?")
        .bind(title, ticker, body, publish, now, +value.archived, key, revision).run()
      : await env.COMMUNITY.prepare("INSERT INTO news(id,title,ticker,body,published_at,updated_at,archived) VALUES(?,?,?,?,?,?,?)")
        .bind(recordId, title, ticker, body, publish, now, +value.archived).run();
  } else {
    const expiry = timestamp(value.expires_at);
    if (typeof value.withdrawn !== "boolean" || typeof value.resend !== "boolean")
      throw new HttpError(400, "Choose withdrawal and resend options.");
    const article = value.news_id ? id(text(value.news_id, 36)) : null;
    if (article && !await env.COMMUNITY.prepare("SELECT id FROM news WHERE id=?").bind(article).first())
      throw new HttpError(400, "The linked news article does not exist.");
    const original = key ? await env.COMMUNITY.prepare("SELECT delivery_id,published_at,withdrawn FROM announcements WHERE id=?")
      .bind(key).first<Pick<AnnouncementRow, "delivery_id" | "published_at" | "withdrawn">>() : null;
    if (key && !original) throw new HttpError(404, "Announcement not found.");
    // Editing a delivery that's already gone out keeps its original audience.
    const previous = original?.published_at;
    const preserve = previous != null && previous <= now && !original?.withdrawn && !value.resend;
    const sentAt = publish === null ? null : preserve ? previous : Math.max(now, publish);
    if (sentAt !== null && expiry !== null && expiry <= sentAt)
      throw new HttpError(400, "Expiry must follow publication.");
    if (original) {
      const delivery = value.resend ? crypto.randomUUID() : original.delivery_id;
      result = await env.COMMUNITY.prepare("UPDATE announcements SET delivery_id=?,title=?,body=?,news_id=?,published_at=?,expires_at=?,withdrawn=?,updated_at=?,revision=revision+1 WHERE id=? AND revision=?")
        .bind(delivery, title, body, article, sentAt, expiry, +value.withdrawn, now, key, revision).run();
    } else {
      result = await env.COMMUNITY.prepare("INSERT INTO announcements(id,delivery_id,title,body,news_id,published_at,expires_at,withdrawn,updated_at) VALUES(?,?,?,?,?,?,?,?,?)")
        .bind(recordId, crypto.randomUUID(), title, body, article, sentAt, expiry, +value.withdrawn, now).run();
    }
  }
  if (!result.meta.changes) throw new HttpError(409, "This record changed in another tab. Reload it before saving.");
  return json({ id: recordId, revision: revision + 1 }, key ? 200 : 201);
}
