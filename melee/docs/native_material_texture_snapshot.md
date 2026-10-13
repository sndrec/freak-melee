# Material texture roots and frame-zero rewind

Compact component 38 (`material-textures`) owns the actual HSD material shadow
chain head, `tobj_shadows`, and the lazy toon texture root, `tobj_toon`. Its
112-byte record contains the physical scene epoch, two typed retained-class
claims and the bounded shadow count. Payloads and `next` links remain in the
ordinary class snapshot. No texture data or model data is duplicated here.

## Diagnosis

The previous frame-zero test differed by six address bytes. A class block was
mistakenly read as a JObj: offset 16 was described as a parent pointer. The native
debugger's write breakpoint at the *original* zero-input boundary established
that this was a TObj `next` link. Both writes came from the original
`HSD_MObjDeleteShadowTexture`, called by `lbShadow_8000F38C` during normal drawing.

The first draw removed two temporary shadow textures. Class restoration correctly
restored their links, but the unsaved list head stayed empty after rewind. The
same source deletion calls could no longer find the textures, leaving one link
different. Restoring the head fixes the original cleanup without altering source
shadow behavior, deleting textures early, masking bytes or skipping drawing.

## Capture, validation and load

Capture requires an idle scheduler/interpreter and active physical object epoch.
Null roots carry zero claims. Non-null roots must belong to retained class
backing and have known initialized class types of at least `sizeof(HSD_TObj)`.
Capture bounds the live shadow chain at 4,096 nodes and refuses invalid backing,
cycles exceeding that bound, aliases and insufficient output storage.

Aggregate class preflight validates both roots against the immutable saved class
view, then walks the *saved* shadow chain for its exact count. It checks backing,
saved type and terminating null without following live texture links. No write
or allocation occurs during preflight.

Load requires the class companion to have restored payloads/links first. It
checks all root types and the restored chain count before assigning either root.
It never calls add/delete/ref/render routines. These are private in-memory
records, not a persistent or network savestate format.

## Verified evidence

Archives are under `A:/FreakMelee-test-archives/`.

- `stage-zero-owner-write-debug.log`: original-first-frame writes through the
  actual HSD shadow deletion and camera drawing stack. The earlier breakpoint
  installed only after rewind missed these original writes and is not the
  basis of the fix.
- `material-textures-oct12-zero`: 480 inputs per stock/interpreted eight-fighter
  case, compact history at every pre-input boundary from zero, then 15-frame
  frame-zero rewind. All 39 components, raw endpoints, intermediate traces and
  continued original traces match; 496 captures and two loads per case.
- `material-textures-oct12-fountain` and `material-textures-oct12-greens`: the
  same frame-zero eight-fighter history/rewind acceptance passes on Fountain and
  Green Greens. These exercise different stage/reflection/destructible resources.
- `material-textures-oct12-six`: 1,200 inputs per stock/interpreted case and six
  separate 15-frame rewinds starting at 78, 198, 318, 438, 558 and 678. All 234
  owner comparisons and six complete raw endpoints match in each case.
- `material-textures-oct12-correction`: 480 inputs with human P1 correction over
  15 frames, fresh corrected reference and a repeated compact correction. Both
  stock/interpreted cases match corrected traces and complete endpoints; original
  continuation is recovered and duplicate audio starts remain suppressed.
- `material-textures-oct12-owners-b`: 86,952 controlled owner/allocator assertions,
  including 39 new material assertions. Real class allocation and exact extracted
  source shadow add/delete bodies exercise cleanup replay, saved-chain preflight,
  malformed claims/counts, cycle bound, aliases, idle refusal and companion order.
- `material-textures-oct12-independent-verification.json`: receipt statuses,
  component counts, zero raw differences and all retained input pins rechecked.
- Warning-clean syntax check for the new module and focused history, callback
  and GObj consumer CTests pass. The first owner fixture build retained in
  `material-textures-oct12-owners` failed to link unrelated renderer dependencies;
  the accepted fixture extracts only the two unchanged source list operations.

## Remaining work

Frame-zero rewind is accepted for these cases, not every fighter/stage/resource
lifecycle. Full candidate-wide prevalidation and prepared resource retirement
still need work. Production still uses broad page history; the compact ring's
sixteen 8 MiB diagnostic slots are not the final production sizing policy.
Actual HSD drawing and render RNG are retained. The final eight-fighter,
15-resimulation-tick performance deadline is not established by these tests.
