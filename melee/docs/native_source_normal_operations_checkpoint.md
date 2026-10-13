# Generic normal operations and owned immutable policies

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Five general operations now execute through the direct interpreter:
`attack.enter_jab`, `attack.enter_rapid_jab`, `attack.update_rapid_jab`,
`items.try_ground_pickup` and `collision.air_aerial_landing_callback`.
Numeric motions validate against the actual published common/owned layout;
completion arguments must be same-program root zero-argument Void helpers.

Rapid-jab continuation is a direct common completion query. The native fighter
and interpreted caller invoke their own completion at that point; there is no
callback trampoline or global callback scratch. Current advanced Host callers
and the test-only emitter use the Bool query. The changed Host field was moved
to the struct end so its existing layout fingerprint rejects older binaries.
No version marker or compatibility callback was added.

Dr. Mario's old reset convenience is now an ordinary owned helper using the
existing `bones.root.rotate_x(0)` primitive, registered explicitly for damage
and death2. It resets only the original raw root's X component; no stock handler
or new fighter-specific API is needed. His seven owned signals identify the
Vitamin declaration explicitly.

Owned signals use `articles.signal_owned(handle, DeclaredArticle, "name", value)`.
Names resolve at loading. Runtime requires the exact same-bank declaration,
current damage ownership and a live attachment relation to the caller. Targeted
signals retain their distinct reflection-friendly contract. Caller data identity
is now retained across both forms; replacing userdata cannot publish success.

Immutable sound channel, friction, visibility, sound remap and KO-cue recipes
normalize into independently owned facts before the real DAT importer. The
second metadata pass revalidates against those exact prepared facts. Actual
shared sound-policy validators and mapping run; known names are not placeholders
for accepting an unimplemented policy. Pure asset/layout validation bodies were
moved into their own units, with direct callers preserved and no forwarding shim.

## Verified scope

Receipts under `A:/FreakMelee-test-archives/`:

- `script-normal-attack-primitives-review5`: 1250 checks/58 cases, 64 frozen
  native rapid comparisons and 104 actual source invocations. Native entry/kernel
  bodies, compiler, actor, helper calls and packed snapshot boundaries execute;
  ordinary geometry and package-table publication remain controlled.
- `script-root-rotation-frozen`: 72 checks/four native comparisons. Actual
  setters, Euler/quaternion-copy handling, callback binding/clearing/native
  takeover and actor snapshot restoration. Rig metadata is controlled.
- `script-owned-article-signals-second`: 195 checks through compiler, real
  Fighter/Item actors, signals and fixed GObj pools. Ownership/type rejection,
  deletion/reuse, VM faults and caller userdata replacement are included. Native
  DAT construction, spawning, drawing and unrelated primitive leaves are absent.
- `source-owned-declarations-ctest-7ca0c3668e6549d39efd031cb630afb3`: 404 checks/
  69 cases/four actual DAT imports and minimal source packages. Full stock move
  bodies, gameplay and paired activation are not covered.

Client, content tool, test-only reference exporter and the selected native test
targets build. All 37 selected interpreter/content/pool/lifecycle CTests pass.
Old state/transition primitive fixtures now register their fixed payloads in the
real match pool for collision guards; authored policy actors remain explicitly
absent. They retain 277 and 451 checks respectively.

## Remaining work

The four canonical legacy base directives and their C-oracle construction hint
are still pending a direct caller/API migration. Common-motion policy declarations
remain rejected until their actual shared phase semantics are connected. Sheik's
resources/secondary models and additional native schedulers/engine primitives
also remain open. Dr. Mario's actual owned-asset check still stops at
`items.clear_held()` on line 333; neither his entire script nor the roster is
accepted by this checkpoint. His full C oracle separately stops at Article
metadata. No game or installed/user draft changes occurred; commits remain local.
