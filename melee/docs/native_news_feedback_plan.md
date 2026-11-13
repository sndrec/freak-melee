# Workshop deletion, news, announcements and feedback

Updated 2026-11-13. This is the implementation plan for the next goal.
Implementation is underway. The milestones below distinguish implemented work
from deployment and review that still remain.

## Goal

Let players delete their fighters, read current and historical news, receive
announcements at a convenient time, and send feedback pinned to a screenshot.
Give Twilight an authenticated admin website to write and edit news, send
announcements, and review feedback.

The work is ready for implementation as a goal. Finish with a working local
client, deployed services and admin page, and a user review of the complete
flow. Record the tested build and any remaining acceptance. Obtain Twilight's
admin identity before opening access to the hosted administration page.
Twilight confirmed `sndrec32exe@gmail.com` as that identity. Once the full goal
is complete and the admin page is deployed, publish the new client build to the
existing auto-updater channel; Twilight has authorized that release.

## Confirmed requirements

- Add a discoverable Delete fighter action beside the Workshop's existing
  Clone, Rename, Send fighter and Portrait & tile actions.
- Use the empty strip in the upper right of the main hub menus for a scrolling
  news ticker. Selecting it opens a standalone News page.
- News comes from the hosted service and can change while the client is open.
  The News page supports scrolling through history.
- Administrators can send game-wide announcements through the existing
  trophy/unlock-style notice system. Display them at the earliest convenient
  opportunity after active play.
- Provide one admin website page for writing news, editing earlier news and
  sending announcements. Provide access to submitted feedback there too.
- F2 captures the entire interface exactly as it appears when pressed. Show
  that captured image while the player adds feedback.
- Capture program context, gameplay memory, script state and recent diagnostic
  history at the F2 press as well. Preserve that capture throughout annotation,
  confirmation, retries and upload.
- Show the instruction "Click anywhere on the screen to indicate feedback.
  Press ENTER to send."
- Clicking places a pushpin with a rounded text bubble and a triangular point.
  Enter commits its text. The bubble quickly shrinks and fades into the pin.
  Hovering a pin reveals its note. Clicking it allows editing or removal.
- Enter while a text field is inactive opens submission confirmation. After
  submission, return to the interface or gameplay that was running.
- During online annotation, keep the match running and send neutral local
  fighter input. This choice was confirmed by the user.
- The diagnostic dump contains gameplay state and diagnostics. Exclude account
  private keys, session credentials and authentication data. This scope was
  confirmed by the user.

## Existing implementation to build on

| Area | Current source and behavior |
| --- | --- |
| Workshop catalog | `native_hub_catalog.cpp` joins installed packages and editable projects into the visible collection. |
| Workshop controls | `native_hub_pages.cpp` draws the controls. `native_hub.cpp` handles their actions and mouse/controller input. |
| Installed membership | `nativeContentStore_Uninstall` removes local selectability while retaining cached content used by peers. |
| Editable fighters | `nativeFighterFamilyProject_ListLocal` discovers projects and paired fighter families. A family is one Workshop entry. |
| CSS | `native_css_original.h` provides layout replacement, tile removal and installed-fighter refresh. Layout belongs to the installation. |
| Notices | `native_hub_notice.cpp` already draws animated modal notices with stock menu assets and consumes dismissal input. Its current queue has four entries. |
| Screenshots | Aurora's screenshot path exports the complete presented window to PNG. Requests currently capture a subsequent rendered frame. F2 needs an explicit capture of the frame visible at activation. |
| Event loop | `gm_1A45.c` dispatches native UI input and distinguishes normal rendering from rollback resimulation. |
| Diagnostics | Replay state diagnostics, netplay reports and script state inspection already expose useful parts of the runtime. Feedback needs a capture with its own lifetime. |
| Hosting | The repository has Cloudflare Workers for releases and signaling. The release Worker stores archives and private desync reports in R2. |

Read these entry points before editing. Reuse their actual content, scene and
input operations. Keep new responsibilities in separate, reasonably sized
files and use their APIs directly.

## Workshop deletion

Place Delete fighter in the selected fighter's action area, readable at the
usual window sizes and reachable with the Melee hand. Confirm using the fighter
name and a concise explanation that its project and roster tile will be removed.
Cancel should be the initial safe choice in the confirmation.

Deleting a draft removes the whole family project, including both members of
Zelda/Sheik or Popo/Nana. Deleting an installed fighter removes its installed
membership. For an entry containing both, perform both operations. Remove every
CSS tile for the deleted package and clear any local selection referencing it.
Refresh the collection and select a neighboring entry, or show the empty state.

Treat deletion as one recoverable operation. Validate that the project resolves
inside the configured projects directory before touching files. Use a pending
deletion record and a staged directory move, or an equally simple transaction,
so a locked file, interrupted write or restart can finish or undo the operation.
Persist the CSS and installed membership consistently with that operation.
Retain package cache objects while matches, outfits or received packages still
reference them. Keep unrelated projects, costumes and other package revisions.

Perform destructive catalog changes at a menu boundary. If the selected fighter
is readied in a lobby, clear that readiness and cancel a pending match countdown
before completing deletion. Verify the existing lobby content-refresh path.

## News ticker and reader

The ticker uses the highlighted empty header strip across ordinary hub menus.
Reuse the native menu renderer, fonts, clipping and hand hit testing. Measure
the strip against the actual scene at narrow and large window sizes. Keep its
text inside the existing border, with a useful hover/selection treatment.

Show the latest published headline or the item's optional short ticker text.
Long text scrolls smoothly and pauses while hovered. Selecting with the mouse
or A opens that article on the News page. A new feed revision updates the
ticker without disturbing text entry or resetting the page being read.

The reader has a prominent title, publication date, body and Back control.
Support paragraphs, headings, lists and ordinary web links through a limited
text format. Escape content before rendering. Mouse wheel, scrollbar and
controller input can scroll the page. Provide a history list and load older
entries through stable pagination.

Proposed defaults for implementation are 30-second refresh, conditional GETs,
25 history entries per page, and retention of published news until the admin
archives it. Keep a bounded client cache and load additional pages on demand.
Refresh immediately on returning to the hub when the last refresh is overdue.
Use the last valid cached feed when offline. A failed or malformed response
preserves the previous valid content. A valid empty feed clears the ticker.

The service supplies JSON. The admin page owns edits, so changing a headline or
article does not require rebuilding the client or manually uploading JSON.

## Game-wide announcements

An announcement has a stable ID, revision, title, short body, publish time,
optional expiry, optional linked news article and active/withdrawn status.
The admin previews its appearance before sending it.

Fetch announcements alongside feed refreshes. Queue them outside gameplay
state. Display in the existing native notice system when the player is in an
ordinary menu and free of text editing, calibration, another confirmation or
the F2 overlay. Keep them pending through matches, stage selection/countdowns
and editor operations. After a match or operation completes, display the next
pending notice at the first suitable menu tick.

Persist seen IDs per installation, including guest installations. Mark a notice
seen when dismissed. An ordinary edit updates pending content. An explicit
Resend action creates a new delivery ID so corrections can be announced again
intentionally. Withdrawn or expired notices leave the pending queue. Bound the
queue, fetch further notices after dismissal, and preserve the existing local
notices when the remote queue is full.

## F2 capture and annotation

### Capture at the keypress

Create one immutable report capture when F2 is pressed. Bind the screen image,
gameplay dump, script values and diagnostic history to one capture ID and
timestamp. Record both the last displayed frame and the simulation frame, which
can differ during rollback. Describe that difference in the report metadata.

The renderer must retain or explicitly capture the last completed full-window
image visible at the press. Show that image at its captured aspect ratio even
if the window is resized. Avoid advancing or rebuilding the scene to produce
the screenshot. GPU readback and PNG compression may finish asynchronously,
but their input must already belong to the F2 capture.

Copy the relevant diagnostic state on the simulation owner at the same input
boundary, before further simulation mutates it. Complete capture before
neutralizing local input or pausing offline play. Upload code reads only these
owned copies. Editing notes or sending later must never recapture program state.

Use explicit state serialization or a documented selection of gameplay memory
regions. Account state and arbitrary process memory are outside this dump.
Capture enough layout/build information to interpret owned binary state, and
include readable values for common debugging. A raw rollback blob alone cannot
serve as the portable report because it contains pointers and restoration
assumptions. Report partial capture failures visibly and preserve the usable
image and notes.

### Overlay and controls

Use a full-window annotation layer above the frozen image, including Workshop
panels and the original 4:3 game viewport. The underlying interface receives
its normal input again after the overlay closes. Consume the activation press
and pending key/button releases so they cannot trigger actions underneath.

Clicking the image adds a numbered pin and opens its text bubble with keyboard
focus. Enter commits a nonempty note. Empty notes can be cancelled. Animate
commit by shrinking/fading the bubble toward the pin. Hover shows its text;
click reopens editing with Save and Remove available. Focused text editing
consumes Enter before the report-level submit action. Escape closes the current
note first, then offers to discard an unsent report. Use the same interaction
rules when focus changes or F2 is pressed repeatedly.

Pins store coordinates normalized to the captured image, their stable IDs,
order and UTF-8 text. Letterboxing and window resizing must preserve their
positions. Proposed limits are 64 pins and 2,048 characters per note. Enforce
the same limits in the client and service and show clear messages at a limit.

When no text field is active, Enter shows a submission confirmation over the
same image. Confirm submits screenshot, pins and the original diagnostic
capture. Cancel returns to annotation. Give the player clear sending/sent/error
feedback. On failure, retain the report for retry or save it locally, with its
capture time unchanged. Retried submissions use the same ID and create one
server record. Make the choice to resume play explicit if sending fails.

### Play and rollback

Offline simulation pauses while the overlay is active. Online simulation,
network processing and rollback continue. Local gameplay pads become neutral
before packet submission, while required profile metadata remains valid.
Apply this to every local participant controlled on this PC. Annotation input
must not become gameplay input. Resume with fresh hardware samples and consume
the overlay's dismissing press.

Keep report capture state and annotation UI outside the rollback arena.
Resimulation must neither open overlays nor repeat a capture, encode, request
or upload. The service and annotation layer operate independently of match
checksums. Test F2 through CSS, practice, an actual match, Workshop and startup.

### Report contents

| File or section | Contents captured at F2 |
| --- | --- |
| Screenshot | Full-window PNG, pixel dimensions and displayed-frame identifier. |
| Context | Capture ID/time, release and source/build identity, platform, renderer, current page/work mode, focused control and scene-transition status. |
| Gameplay | Match frame and rules, stage, seed/RNG state, fighter/package identities, actions, animation frames, positions, velocities, collision/hitbox state and relevant items. |
| Scripts | Program/content identities, owned variables, active callbacks and runtime diagnostics for the fighters involved. |
| Recent history | Bounded controller-input history, relevant runtime errors, sanitized diagnostic logs, and available netplay timing/checksum context. |
| Pin metadata | Notes and normalized positions added after the original capture. |

Choose precise size budgets after measuring a representative four-player match
with scripts and items. Compression and serialization happen outside the hot
simulation path after the required data has been copied. Capture queues have
explicit capacities and one report is annotated at a time. Keep networking
bounded and asynchronous. Follow the project's required threading guidance
before adding workers, and use existing task infrastructure where available.

## Admin website

Build a single responsive admin page with three sections or tabs.

| Section | Controls |
| --- | --- |
| News | List/search past entries, create drafts, edit title/ticker/body, preview, publish, unpublish/archive and show publication/update times. |
| Announcements | Compose, preview the native notice, send immediately, set optional expiry, edit pending notices, withdraw, and explicitly resend. Link to a news entry when useful. |
| Feedback | List reports by time/build/status, open the screenshot, hover/select pins, read original notes and diagnostic context, and download report files. Mark reports New, Investigating or Resolved. |

The admin page previews the same restricted news formatting as the client.
Preserve drafts while switching sections. Use save revisions to detect an edit
from another tab, show server errors, and require confirmation before sending
a game-wide notice or discarding an article. Publishing an article and sending
an announcement are separate explicit actions.

Serve administration assets from the repository, with reproducible build and
deploy commands. A proposed small dedicated Cloudflare Worker owns this page
and its API, keeping the runtime service contract straightforward.

Use Cloudflare Access with an allowlisted admin identity as the initial login
proposal. Validate the Access JWT's signature, issuer, audience and expiry in
the Worker on every admin route, including report images/downloads. Check
request origin and protect state-changing requests against CSRF. Restrict
administrative access on every exposed hostname. Confirm the admin identity
and hostname with Twilight before enabling hosted access.

Public client routes expose only published news, active announcements and
feedback submission. Report reads and admin writes require authentication.
Guest clients can submit feedback. Give the client a receipt ID after storage.

## Service and storage

Proposed storage is D1 for news, announcements, publication revisions and
feedback metadata, with a private R2 bucket for screenshot/dump objects. Use
native Worker bindings. Reuse the repository's deploy conventions and shared
code where it fits; keep the client endpoint configurable for local review.

| Route group | Purpose |
| --- | --- |
| Public news | Read published headlines/revisions and fetch article bodies/history pages. |
| Public announcements | Read currently active announcements and their delivery IDs. |
| Public feedback submission | Accept an identified report and its bounded attachments, then finalize a receipt. |
| Authenticated administration | Read/write news and announcements, inspect feedback and fetch private attachments. |

Use stable pagination, bounded response/request sizes, rate limits and safe
content rendering. Store creation/update times server-side. Check revisions
when updating records to prevent accidental overwrites. Enforce the same
content constraints at public and admin entry points.

Feedback uploads need explicit size budgets for metadata, image and state.
Bound request bodies before reading them. Store attachments first, then commit
the report metadata after all required objects are verified. Clean abandoned
uploads and make finalization idempotent. Keep object IDs independent of user
supplied file paths. Show incomplete/failed uploads to the submitting client
and keep them out of the admin's completed-report list.

Start with manual admin deletion of reports and a visible storage summary.
Choose automatic retention with Twilight after actual report sizes are known.
News can remain in the database while only a page of it is held by each client.
Persist a small last-valid feed and seen-announcement set locally.

## Implementation milestones

Current checkpoint (2026-11-13):

- The native deletion review passed for installed fighters, draft-only fighters
  and a Zelda/Sheik family. Cancellation, a locked file, restart recovery,
  unrelated fighters and retained cache objects were checked in copied profiles.
- Seven Worker checks pass, covering publication, edit conflicts, announcement
  resends, signed admin access, write origins, feedback retries and attachment
  cleanup. Type checking and the deployment dry run pass.
- The native news review passed for ticker/article selection, article scrolling,
  25-entry history pages, a live feed change, announcement dismissal, saved seen
  IDs and an offline restart. Screenshots were inspected at two window sizes.
- News links, remaining queue/input cases, hosted administration and the full F2
  capture/annotation path still need implementation or integration review.

Reusable reviews are `melee/tools/review_native_fighter_deletion.py` and
`melee/tools/review_native_community.py`. Both use hidden, muted clients with
copied user data; pass an existing test profile and a new output directory.

1. **Workshop deletion**
   - [x] Implement confirmed deletion for installed, draft and paired fighters.
   - [ ] Clean CSS and local selections consistently, including lobby readiness.
   - [x] Verify cancellation, file failure and restart recovery.
2. **Service and administration**
   - [x] Define bounded JSON/report formats and storage tables/objects.
   - [x] Implement public reads, safe report submission and authenticated admin APIs.
   - [ ] Build the single-page news editor, announcement composer and feedback inbox.
   - [ ] Configure and verify Twilight's admin access.
3. **Ticker, News page and announcements**
   - [ ] Implement refresh/cache/history and native menu presentation/input.
   - [ ] Reuse notices with convenient display, expiry, withdrawal and seen tracking.
   - [ ] Verify a live admin edit appears in a running client.
4. **F2 capture and annotation**
   - [ ] Capture the displayed image and diagnostic state together at activation.
   - [ ] Implement pins, bubbles, editing/removal animations and submit confirmation.
   - [ ] Implement offline pause and online neutral-input behavior.
   - [ ] Upload/retry the immutable report and show it correctly in administration.
5. **Integration and user review**
   - [ ] Batch service/input/report checks and hidden, muted native reviews.
   - [ ] Verify deployment and report read/write permissions through hosted routes.
   - [ ] Commit coherent changes and push the private repository.
   - [ ] Offer the complete local build/admin URL for the user's visual and usability pass.
   - [ ] Publish the client through the updater after the full goal and admin deployment are ready (authorized).

## Acceptance

- Delete a fighter, restart and confirm its project and CSS selection are gone,
  with other fighters and referenced content intact. Check a paired family too.
- Use mouse and controller to open News from the ticker on every main hub page.
  Verify clipping, history navigation, empty feed, offline cache and later edits.
- Publish/edit news from the admin page and observe it in a running client
  within the refresh interval. Concurrent admin edits produce a clear conflict.
- Send a notice during a match. It appears at the next convenient menu and stays
  dismissed after restart. Expiry, withdrawal and explicit resend work.
- Press F2 while an animation is visibly changing. Wait, place pins and submit.
  Verify the screenshot and diagnostic values still describe the original
  activation, including after a failed upload and retry.
- Annotate the full Workshop UI, resize the window, reopen/edit/remove notes,
  and verify the admin viewer places pins on the original pixels.
- Confirm Enter's note/submit behavior, Escape cancellation and input consumption.
- Open feedback in a two-client online match. The local fighter receives neutral
  input while peers keep playing, then controls resume cleanly. Check rollback.
- Verify unauthenticated users can read published content and submit a bounded
  report, while admin writes and private feedback downloads require admin access.
- Review diagnostic contents for accidental credentials and verify oversized or
  malformed submissions fail with a useful client message.

## Open choices and proposed defaults

- Admin identity: `sndrec32exe@gmail.com`. Verify the hosted admin route and
  Access policy before enabling access.
- Thirty-second polling provides prompt updates initially. Revisit push delivery
  if a demonstrated requirement needs lower latency.
- The first reader uses restricted text formatting. Plan images/attachments only
  after the core news and feedback flow works.
- Show seen announcements once per installation, matching guest support.
- Measure diagnostic payloads before fixing final upload and retention budgets.
- Visual feedback after the first complete implementation determines final
  pin/bubble animation, ticker speed and News/admin layout refinements.

## Platform references

Use current platform documentation during implementation, especially for
streaming request limits, storage and authentication.

- [Cloudflare Workers best practices](https://developers.cloudflare.com/workers/best-practices/workers-best-practices/)
- [Validate Cloudflare Access JWTs](https://developers.cloudflare.com/cloudflare-one/access-controls/applications/http-apps/authorization-cookie/validating-json/)
- [R2 Workers API](https://developers.cloudflare.com/r2/api/workers/workers-api-reference/)
