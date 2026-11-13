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

The first send saves the notes beside the captured attachments. **Retry** sends
those same files with the same report ID. A successful receipt removes the
local files. Returning after an upload failure keeps them under the content
store's `feedback` directory, named with the report UUID:

- `<id>.png`: original full-window screenshot.
- `<id>.context.json`: diagnostic capture from activation.
- `<id>.notes.json`: submitted notes and capture metadata.

Cancelling an unsubmitted report removes its local attachments.

Diagnostics include build and scene details, fighter/item state, script
variables and callback IDs, recent resolved controller inputs, and a filtered
warning log. The collector reads an explicit set of gameplay fields and keeps
its history outside rollback state. Account storage and authentication buffers
are excluded from the collector.

The existing render worker reads and encodes the reserved image. The community
worker streams the report to `/v1/feedback` and publishes upload progress at
half-second intervals. The service stores report metadata in D1 and attachments
in its private R2 bucket. Administration requires Cloudflare Access.

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
