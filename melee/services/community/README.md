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

The public routes are `/v1/feed`, `/v1/news`, `/v1/news/<id>` and
`POST /v1/feedback`. News history uses 25-entry pages and the returned `next`
cursor. Feed and article reads support `If-None-Match`.

Administration requires a Cloudflare Access assertion even in local development.
Tests sign their own temporary assertions and supply a test verification key;
the service has no alternate login path.

## Deployment

Create the D1 database and private R2 bucket named in `wrangler.jsonc`. Replace
the database's `local` ID with the resulting remote ID. Set `ACCESS_TEAM_DOMAIN`
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
