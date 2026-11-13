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

Current setup: the Cloudflare account still requires activation of Zero Trust
Free. Its checkout asks the account owner to accept terms and authorize charges
above the free allowance. That step has been left for Twilight in the browser.
The Worker denies administration until the Access application is configured.

The foundation Worker is deployed at
`https://freak-melee-community.sndrec32exe.workers.dev` (version
`9b5db411-8e5a-46e0-a4e1-97cf5dc2f0f5`). Hosted checks passed for health/feed reads,
an accepted feedback upload and its idempotent retry. Admin HTML, scripts and
APIs currently return the configuration error above. The report bucket's
`r2.dev` public access is disabled. Complete Access setup and browser review
before calling administration ready.

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
