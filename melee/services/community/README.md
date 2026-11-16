# Community service

This Worker serves published news and announcements, accepts feedback reports,
and hosts the administration page at `/admin/`. D1 holds articles and report
metadata. The private R2 bucket holds screenshots and diagnostic captures.

## Development

Run commands from this directory:

```powershell
npm ci
npm run check
npm test
npx wrangler d1 migrations apply COMMUNITY --local
npm run dev
```

The public routes are `/v1/feed`, `/v1/news`, `/v1/news/<id>`,
`/v1/announcements`, `/v1/announcements/<delivery-id>` and `POST /v1/feedback`.
News history uses 25-entry pages and the returned `next` cursor. The feed includes
64 announcements and an `announcements_next` cursor for older notices. Individual
announcement reads let clients refresh or withdraw a notice already on screen.
Public reads support `If-None-Match`.

Articles support paragraphs, headings, lists, `[label](https://...)` links and
plain HTTP/HTTPS addresses. The admin preview and native reader display other
markup as text. Links open only when selected.

Administration requires a Cloudflare Access assertion even in local development.
Tests sign their own temporary assertions and supply a test verification key;
the service has no alternate login path.

## News drafts from the command line

Maintainers can save an unpublished article using their existing Wrangler owner
login. Run from `melee`, after installing this service's dependencies:

```powershell
python tools/save_native_news_draft.py --key my-update --title "My update" --body-file docs/releases/my-update.md --receipt .local/my-update-news.json
```

Use the release ID as the key when writing patch notes. The command inserts a
draft into the same D1 database the website uses, then reads it back and records
its ID, revision and publication state. It leaves ticker text empty and publication
time unset. It creates no announcement. The key and receipt identify the article
on a retry, which preserves existing text and any edits or publication by the
owner. `body_matches` reports whether the saved title and text match the input.

This requires D1 access through the owner's Cloudflare credentials, just like
`download_native_feedback.py`. It adds no network route or login method. The owner
can edit and publish the saved draft through the usual administration page.

## Deployment

The D1 database and private R2 bucket named in `wrangler.jsonc` were created on
2026-11-13 in Eastern North America. The initial migration has been applied.
Preview URLs are disabled; the production `workers.dev` hostname serves the
community API and administration.

Set `ACCESS_TEAM_DOMAIN`
and `ACCESS_AUDIENCE` to the Access application's team domain and audience.
Allow only the configured administrator email in that application's policy.
Protect `/admin` and `/admin/*` on every enabled hostname. The Worker also checks
the assertion's signature, issuer, audience, expiry and email on each admin
request. Writes additionally require a matching origin and the admin header.

```powershell
npx wrangler d1 migrations apply COMMUNITY --remote
npx wrangler deploy
```

Verify an allowed sign-in and a rejected sign-in through the deployed hostname,
including private screenshot and diagnostic downloads. An unconfigured Access
application returns a clear service error and grants no administrative access.

The service is deployed at
`https://freak-melee-community.sndrec32exe.workers.dev` (version
`d9f83847-ba54-493c-b9e3-9763d4b80aaf`). Administration is at `/admin/`.
The Access application `9d75a2a4-ff59-46eb-a479-8a1eab6ea982` protects that
directory and its descendants. Its allow policy contains only
`sndrec32exe@gmail.com`, with a twelve-hour sign-in session. The configured
team domain is `divine-cake-d237.cloudflareaccess.com`.

Twilight confirmed that hosted sign-in reaches the admin page. Hosted checks
passed for public news/feed reads, feedback upload and an identical retry.
Unauthenticated admin HTML, scripts, APIs and both report attachment routes
redirect to Access sign-in. The report bucket's `r2.dev` public access is
disabled. Twilight published and edited an article through the hosted admin
page and confirmed that the feedback screenshot, pins, notes and diagnostics
load. A running native client received the article and announcement, then
displayed the edited article body without restarting.

## Feedback reports

The upload body is `FMREPORT` (eight ASCII bytes), followed by three unsigned
little-endian 32-bit sizes for metadata, screenshot and context. The payloads
follow in that order. Metadata is UTF-8 JSON with `id`, `capture_at`, `build` and
`pins`. A pin has `id`, normalized `x`/`y`, and `text`.

Limits are 512 KiB of metadata, a 16 MiB PNG, an 8 MiB diagnostic capture,
64 pins and 2,048 characters per pin. Capture IDs are UUIDs. Retrying identical
content returns the original receipt; reusing an ID with different content
returns a conflict. Capture time belongs to the client's original F2 press.

Attachments are stored before the completed report is committed. A scheduled
scan checks 100 objects every ten minutes and removes unreferenced objects
older than one day. Completed reports remain until an administrator deletes
them. The report list includes the number of reports and their stored size.
