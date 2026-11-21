# Screenshot feedback

Press **F2** to capture the full interface and gameplay diagnostics. Click the
captured image to add a numbered pin and write a note. Press **Enter** to keep
the note. **Shift+Enter** adds a line break.

Hover a pin to read its note. Click it to edit the text or choose **Remove**.
Press **Tab** while viewing pins to hide or show the header and footer, giving
access to the image beneath them. Press **Enter** with the note editor closed
to open the send confirmation. **Escape** closes the current note or dialog;
from the pin view, it cancels feedback.

Offline play pauses during annotation. A connected lobby or online match keeps
running, with neutral local controller input while feedback is open. On return,
held buttons and triggers are consumed until released. Current stick movement
resumes from fresh controller samples.

## Capture and delivery

At activation, the game reserves the last successfully presented full-window
image and writes the current gameplay diagnostics on the game thread. The
report records the displayed frame ID, presentation time, capture time and
simulation tick. The image stays available across later rendering and window
resizing. Pin positions use normalized coordinates in the original image.

The first send saves the notes beside the screenshot and diagnostics. The game
keeps the original image, diagnostics and confirmed notes in memory for the
upload. **Retry** sends those same bytes with the same report ID. A successful
receipt removes the game’s working copies. You can move a copy to keep it.
Returning after an upload failure keeps the files under the content store’s
`feedback` directory, named with the report UUID:

- `<id>.png`: original full-window screenshot.
- `<id>.context.json`: diagnostic capture from activation.
- `<id>.notes.json`: submitted notes and capture metadata.

Cancelling an unsubmitted report removes its local attachments.

Pressing F2 records the build, scene, fighters, items, script variables,
callback IDs and recent controller inputs. Each engine warning includes its
module and the original message template. Values filled in by the engine,
such as a local filename or a graphics driver message, stay in the console.
The screenshot includes whatever is visible in the game window when you press F2.

Each report can record up to 512 named script values per fighter or item and
8,192 across the scene. A flag on the affected actor identifies an incomplete
list. The diagnostic JSON has an 8 MiB limit. If it cannot fit, the report
includes a smaller capture with the original frame information and marks the
diagnostics as partial.

The render worker encodes the reserved image. When you confirm a report, the
game keeps its screenshot, notes and diagnostics together in memory until you
close it. A retry sends the same bytes under the same report ID. For a saved
Workshop agent report, the game checks the files against their original hashes
and uploads the bytes it checked.

The community worker streams the report to `/v1/feedback` and publishes progress at
half-second intervals. The service stores report metadata in D1 and attachments
in its private R2 bucket. Administration requires Cloudflare Access.

## Download reports locally

From the repository root:

```powershell
python melee/tools/download_native_feedback.py
python melee/tools/download_native_feedback.py --all
python melee/tools/download_native_feedback.py --id 162b2c34-722c-48c0-9c95-2a99a2a7f1ae
```

The default command downloads the newest report. Use `--limit 10` for the ten
newest, or `--status new` to filter the results. `--all` downloads every matching
report. Files go under `.local/feedback/<report UUID>/`; `--output` chooses another
directory. Each report contains `notes.md`, complete metadata in `report.json`,
`screen.png` and `context.json`. Pin positions use normalized screenshot coordinates.
The tool checks each attachment's stored size and SHA-256 hash. Repeating a download
reuses matching local attachments and refreshes the report's status and notes.
Submitted text appears literally in the notes. The screenshot links to the local
image saved with the report.

Run `npm ci` and `npx wrangler login` in `melee/services/community` when setting
up another developer PC. Downloads use that Cloudflare administrator login to read
the existing D1 metadata and private R2 attachments. Reports remain unchanged on
the service. No browser session is needed after login.

The administration website also has authenticated read endpoints:
`GET /admin/api/feedback` lists reports, `GET /admin/api/feedback/<id>` returns
metadata and pins, and `/screen` and `/context` download the attachments. Those
routes use the website's Cloudflare Access sign-in. The local downloader uses
Wrangler's existing administrative access instead.

After every note in a report has been addressed and verified, remove that report
and its stored screenshot and diagnostic capture with:

```powershell
python melee/tools/download_native_feedback.py --delete --id <report UUID> --output .local/feedback-review-2026-11-15
```

Deletion requires one explicit report ID. The tool writes a local removal receipt
before removing the hosted record, deletes both attachments and checks that the
record is absent. If interrupted, repeat the same command and output directory
to finish from the receipt. Downloaded local evidence is kept.

## Reviews

`tools/review_native_feedback.py` runs a hidden, muted client with copied user
data and a loopback HTTP fixture. It checks note creation/edit/removal, resize,
send confirmation, an upload failure and an identical retry. It saves PNGs of
the annotation interface and a report receipt.

`tools/review_native_feedback_online.py` runs two hidden, muted UDP peers using
an installed scripted fighter. A six-frame artificial outbound delay forces
rollback. It checks that both peers advance during F2, the annotating player's
input stays neutral, the remote player can move, and local movement resumes.

Both tools take `--exe`, `--iso`, `--profile` and a fresh `--output` directory.
The profile directory contains the `Freak Melee` user-data directory.

The 2026-11-13 reviews passed locally. The upload review sent identical
757,850-byte reports on its failed attempt and retry, including an 8,362-byte
diagnostic capture. The offline gameplay check captured a scripted fighter and
verified unchanged animation frames/positions while paused, followed by resumed
simulation. The two-peer review passed with rollback and confirmed checksums.
Hosted delivery and browser administration are covered by the community-service
deployment review.
