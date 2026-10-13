# Main camera snapshot ownership

2026-10-11 checkpoint. This owns actual main-camera scalar/global state, not
a second camera representation or a complete match snapshot.

## State and dependencies

`camera_state.c` owns the original `game_camera`, debug-mode state,
coefficient table, and auxiliary CObj root. It also contains 51 original camera
functions. The 1,424-byte component copies all camera transforms, smoothing,
pause/orbit controls, render masks, quake state and these global values. Actual
camera globals remain in `.mstate$m` during migration. The scene lease lives in
`.data`, outside the broad page oracle.

The primary and auxiliary CObj payloads belong to the ordinary HSD class owner;
their WObjs and shared matrix allocations keep their existing owners. Camera
snapshots do not duplicate them. Generic class-backing references establish the
fixed native CObj spans. The primary GObj uses its existing pool identity and
generation; the optional quake-model GObj has a separate generation reference.

Initialize/enter/exit retire history through the host scene epoch. Save,
Validate and Load require the real GM idle boundary and unchanged storage.
Tagged callback unions are validated only in their callback interpretation;
ordinary vector bits are not mistaken for function addresses. Current resident
pause/boss callbacks are explicitly admitted. Unsupported callbacks reject.
Independent buffers, exact root references and native mode ranges are checked
before writes. Load additionally resolves restored generations and checks live
GObj roles and fixed CObj class identities before changing any camera value.
None of these operations allocate, construct objects, draw, or invoke callbacks.

The class/GObj companions must restore before camera Load. These local checks
do not establish cross-component candidate graph closure: the aggregate must
prove class-block membership, restored object roles, WObj/matrix references,
target borrowers and matching scene/resident leases before any component writes.

## Native layout corrections

Moving methods exposed an old `Camera_ApplyQuake` assumption that separate
callback, WObj-description and camera-description globals were one contiguous
GameCube block. Native layout does not guarantee that arrangement. The split
initially displaced the camera to x=83, then magnifying-glass damage changed RNG
and later gameplay around sample 184. Quake scaling now reads the actual
`cm_803BCB64` descriptor directly. No replacement camera rule was introduced.

Projection also allocated a 3x4 `Mtx`, although Aurora's perspective and
orthographic builders write 4x4 matrices. `lbVector_WorldToScreen` now reserves
a `Mtx44`. The sanitizer negative control demonstrates the overflow; this
independent correction did not explain the observed camera displacement.

The interest callback pointer now has the `bool` return type already used by
its setter and actual boss callbacks. Three unused tail data definitions were
removed. An independent body audit compares all 156 donor camera functions,
allowing only scene retirement, the callback type correction and direct quake
descriptor access. The original coefficient initializer is unchanged.

## Evidence and limits

Under `A:/FreakMelee-test-archives/`:

- `camera-state-oct11-owner-final`: 66,237 checks across connected allocator,
  class, heap, ID and camera fixtures. Camera contributes 114 checks, including
  fourteen malformed candidates, exact scalar/debug/coefficient restoration,
  sixteen repeated controlled updates, callback tags, unaligned buffers,
  generation reuse, class payload/type restoration and lifetime/activity refusal.
  Real class allocation and pool components are used; CObj initialization and
  the idle predicate are controlled. No live camera gameplay is simulated here.
- `camera-state-oct11-gm-b`: the existing real GM/controller/input/RNG/player
  and camera-target restore fixture passes, including sixteen repeated ticks.
  It has no main-camera graph; class-access traps prove the new scene hook does
  not accidentally use a nonexistent root.
- `camera-projection-oct11-owner-final`: exact extracted projection, GXProject,
  quake and setter bodies plus actual Aurora matrix/vector sources. Both viewing
  paths, perspective/orthographic projection and quake scaling pass 75 numeric
  checks with AddressSanitizer. Restoring the old undersized matrix intentionally
  reports a stack overflow. Controlled camera/stage leaves are documented.
- `camera-state-oct11-eight-b`: four hidden, muted, rendered 1,200-sample runs
  with medium items and eight actual actors. All 26 components save/validate at
  all ten sampled boundaries in stock and independent interpreted scenarios.
  Diagnostic on/off and previous/current populated gameplay trace families match
  exactly. This census performs no Load.
- `camera-state-oct11-independent-verification.json`: verifies 1,683 class/camera
  fixture pins, both 1,824-pin GM source sets, 62 live pins, generated projection
  sources/binaries/logs, original camera bodies and actual compiled sections.
  The client/content/source-assets/GObj-consumer targets build; the focused
  GObj-consumer CTest passes.

Main-camera Save averages 1.03 us stock and 0.95 us interpreted; Validate averages
0.10/0.11 us over ten observations each. These are component measurements, not
complete-match save/load or worst-case rollback timing.

Remaining: stage/world/other GM/HUD/effect/dynamics globals, multi-heap payload
ownership, resident leases, cross-component candidate agreement, aggregate
restore/history and full-match differential/performance acceptance. Production
rollback still uses the broad page oracle. Complete state size and the
eight-scripted-actor fifteen-resimulation-tick deadline remain unproven.
