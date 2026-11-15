# Interpreted single-fighter special-entry checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. All 176 selected grounded/airborne special-entry scenarios now have
passing stock/source comparisons across the initial batch and focused reruns.
This is consolidated evidence from successive builds, not one final sealed
176-case run. It supplements the [22-fighter CPU checkpoint](native_source_roster_cpu_checkpoint.md)
and the separate [paired-fighter checkpoint](native_source_family_gameplay_checkpoint.md).
Kirby remains excluded.

## Workload

`tests/native/fighter_roster_special_parity.py` attempts every selected pair
despite earlier failures. Each fighter starts from a fresh ordinary stock clone,
imports its shipped source parameters, builds interpreted bytecode, creates a
canonical package, verifies it and accepts it into a private collection on A:.
No generated behavior C or DLL, installed package or user draft participates.

Each scenario consumes 480 actual controller samples, seed `0x1BADB002`, Final
Destination, four stocks and neutral human Zelda P2. P1 receives neutral, side,
up or down B; airborne cases first receive jump input. Stock and independent
source run separately, fully rendered, hidden, muted and unpaced. The input
does not call a move handler or manufacture a Fighter parent.

The shared strict comparator observes both fighters, companion reports,
actions, damage/contact, item state/events, joint poses, materials, linked
resources and RNG. Only independent fighter kind/behavior identity is normalized.
Coverage additionally requires full replay consumption, the intended starting
ground/air state, a stock-appropriate special entry and no source/fighter/item
VM fault. DK and Peach deliberately have no aerial down-special entry; some
grounded specials immediately switch to air. Those are explicit coverage rules,
not ignored comparison differences.

The harness pins itself, executables, ISO, comparison tools, templates, owned
source, project files, canonical packets, portrait and replays before/after.
`--retry-failed-from` freshly prepares only failing pairs from an earlier receipt.
It never edits that receipt or reuses its package as the new accepted input.

## Batch corrections

The initial 176-case batch passed 157 cases and reported 19 failures. Five were
coverage mistakes, ten exposed layout/dispatch/rounding issues, and four exposed
stock-only scratch observation in Game & Watch articles. They were addressed
together before the grouped rerun.

- Jigglypuff's unused disc attribute words at `0xE8`/`0xEC` were declared as
  pointers. On Windows x64 this shifted Pound attributes at `0xF0`/`0xF4`.
  They are now 32-bit words with compile-time offset/size assertions. Pound's
  script behavior was unchanged.
- Marth, Roy and DK need six single-joint attached-model recipes whose variadic
  dispatcher uses `void*`, but whose actual consuming helper requires a JObj.
  Generic signature admission now follows that typed helper; synchronous joint
  spawns also validate the creator root used for scale and facing updates.
- DK's named Hand Slap effect had fallen through to numeric dispatch using its
  declaration index as the engine effect ID. Named deferred position effects
  now retain the creator/program/layout/parameter identities and selected bone,
  then construct the model and bind its interpreted update at normal queue flush.
  Unsupported named call shapes reject instead of becoming numeric IDs.
- Code review then found the new deferred path still used Fighter position.
  It now calls the original bone-world-position helper after queue flush selects
  the live joint. Immediate position-on-part construction samples the joint too.
  Stale generations, scene/program/layout/parameter changes, root/part replacement
  and queue clearing drop requests before construction. Queue storage uses the
  existing fixed allocator and retains neither VM-stack nor bank pointers.
- Both arrow scripts preserve stock binary32 multiplication, addition and
  degree-to-radian conversion during embedding wobble. Detailed Young Link
  joint output exposed the differing rotation bits; no tolerance was added.
- Chef variant is observed through its real `item.state.variant` declaration in
  interpreted storage. Rescue control ownership is observed through its live
  attachment-owner relation. Stock uses its original union fields. Source items
  do not write fake stock scratch values to satisfy parity. Missing/retired
  source state stays a mismatch, and changed variants/owners remain detectable.

The added scripting support remains general: named deferred position recipes,
raw owned bone selection and typed attached-model dispatch. There is no new
fighter-specific behavior primitive.

## Evidence

Artifacts under `A:/FreakMelee-test-archives/`:

| Receipt or log | Result |
| --- | --- |
| `source-roster-specials-oct11-a/receipt.json` | Initial 157 PASS / 19 FAIL; all 22 fresh packages prepared; unchanged pins. |
| `source-roster-specials-oct11-b/receipt.json` | All 15 non-Game & Watch failures pass after grouped corrections; four observer mismatches remain; unchanged pins. |
| `source-roster-specials-oct11-c/receipt.json` | Both Chef and both Rescue scenarios pass after observing real source state; unchanged pins. |
| `source-link-arrow-oct11-final/receipt.json` | Link's grounded/airborne arrows pass after the shared rounding correction; unchanged pins. |
| `source-donkey-deferred-world-oct11-final/receipt.json` | Fresh DK grounded down-special still passes after the bone-position correction; unchanged pins. |
| `source-younglink-bow-detail-oct11-a/` | Original frame-172 rotation/matrix difference used to locate float rounding. |
| `script-joint-effect-roster-oct11-accept/receipt.json` | 65,536 effect IDs, 524,674 checks, 55 actual helper-copy cases; 100 admitted Joint IDs. |
| `script-deferred-effect-oct11-world-c/receipt.json` | 43 connected cases, 5,280 checks, 1,551 model frees; actual source lowering, constructors, interpreted updates and queue flush/clear. |
| `source-specials-oct11-build.log` | Grouped runtime/client/content-tool build passes. |
| `source-specials-observer-oct11-build.log` | Observer/client/content-tool/semantic-test build passes. |
| `source-specials-deferred-world-oct11-build.log` | Final bone-sampling client/content-tool build passes. |
| `source-specials-final-oct11-build.log` and `source-specials-final-oct11-ctest.log` | Final runtime/status-propagation build and three focused CTests pass. |

The connected fixture adds 13 immediate/deferred/retirement/cancellation/failure
cases and verifies all 13 queue allocations are freed. Immediate constructor
failure returns an execution error, rather than just logging and reporting
success to the script. The native queue and model
constructor bodies are extracted from current source; allocator, model loading,
world-position sampling and platform leaves are controlled. This is not retail
particle or pixel acceptance. The earlier 30-case receipts remain historical.
The first expanded fixture failed a stale five-effect count assertion; that
failed receipt is retained and not counted as passing. The 42-case world-b
receipt precedes the added immediate-failure status propagation check.

`native_semantic_state` passes with real source storage reads plus controlled
actor lookup. It rejects changed source variants, missing declarations, wrong
owners, retired generation/storage, changed/cleared attachment owners and verifies
observation does not overwrite native scratch. Live Game & Watch comparisons
provide the separate actual actor/ownership evidence.

The refreshed `native_script_compile` and `native_samus_template_source` CTests
also pass. Existing native compiler warnings are not a warning-free build claim.

## Remaining boundaries

The subsequent [follow-through checkpoint](native_source_roster_followthrough_checkpoint.md)
adds 42 selected charge/release/chaining/Teleport cases across five bases.

These scenarios exercise entry and ensuing playback; they do not exhaust
charge/release, chaining, aiming, reflection/absorption, captures, interrupts,
death/retirement, all seeds/stages/opponents or every move. Item geometry and
resources are compared; the replay trace does not enumerate every live effect
pose, which is why deferred effect positioning also needed its connected check.
Pixels/audio, complete results flow, a complete mutable-state definition,
restored whole-match rollback correctness and the 15-ticks frame budget remain
separate acceptance work. The overall goal stays active; commits remain local.
