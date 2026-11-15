# Fox/Falco interpreter accessory callback checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The shared template now binds named root helpers through the existing generic
`callbacks.accessory(helper)` API at its authored transition points. No
move-slot callback lookup, stock special handler, compatibility layer or new
fighter-specific intrinsic was added.

The actual effect logic lives in `fire_accessory`, `side_accessory` and
`reflect_accessory`. Blaster callbacks bind the existing `blaster_fire` helper
directly. The three one-shot effect helpers clear themselves with
`callbacks.accessory(None)` after installing the standard effect-hitlag callbacks.

## Comparison with stock source

| Behavior | Authored operation | Original source |
| --- | --- | --- |
| Charge effect | Effect 1163 on mapped TransN | `ftFx_SpecialHi_CreateChargeGFX` |
| Travel effect | Owned FireTravel recipe 1164 on mapped Hip | `ftFx_SpecialHi_CreateLaunchGFX` |
| Side dash effect | Effect 1165 on raw TopN part 0, with facing scalar | `ftFx_SpecialS_CreateGFX` |
| Reflector effects | Selected effect 1160/1161/1162 on raw HipN part 5 | `ftFx_SpecialLw_CreateLoopGFX`, `CreateStartGFX`, `CreateReflectGFX` |
| Blaster shot callback | Install when entering/restarting the firing loop | `ftFox_SpecialN_StartAnimation`, `ftFox_SpecialN_FinishLoopTransition` |

Stock source is under `src/melee/ft/kinds/ftFox`. Charge/travel explicitly map
semantic roles; side/reflector preserve the stock raw-part lookup. The template
uses `effects.spawn_on_part` and `bones.index` for the former, rather than the
unsupported old `effects.spawn_on_bone` form.

Removing move-level accessory declarations is significant: those declarations
would automatically arm a callback on the move's initial states. Stock side-B
startup has no dash-effect callback, and stock neutral-B startup has no firing
callback. Explicit assignments preserve that distinction and allow later
ground/air, hit and loop transitions to bind the same helper.

## Evidence and limits

Fresh isolated stock copies, parameter import and all-body audits are recorded
in `A:/FreakMelee-test-archives/fox-falco-callbacks-oct10-final/receipt.json`.
Both forms pass metadata, and all 256 Fox / 254 Falco bodies are visited.
Reported body failures decrease from 47 to **32 per form**. One net function
is removed because four move callbacks become three substantive helpers and a
direct reference to the existing firing helper.

Neither package is admitted yet. Each body still reports only its first error;
the audit collects those errors across every body. Remaining groups include
article factories/signals, bone and physics queries, radio-taunt stage calls,
item operations and receiver properties. These counts are preparation evidence,
not whole-fighter gameplay acceptance.

The connected cohort in
`A:/FreakMelee-test-archives/script-callbacks-oct10-final/receipt.json` passes
**102,551 checks / 9,661 cases**. The extended accessory case uses the real
compiler, actor and VM to verify nongated execution, native-callback takeover,
self-clear, hitlag callback installation and rearming. Its asset/effect leaves
remain controlled. It does not draw Fox/Falco effects or execute a complete
special-move match.

No native production C changed in this checkpoint, so no client rebuild was
needed. Audits use the existing content tool with its executable hash recorded;
the connected fixture recompiles its changed test and current engine sources.
All runs are console-only, and normal installed packages and user drafts remain
unchanged. Receipt inputs are independently rehashed before the local commit.
The preliminary `-a` receipts are superseded: later documentation edits changed
their README/template byte hashes, including line endings. The final runs use
the current files and the same callback logic, rather than relabeling those
older input hashes as current evidence.
