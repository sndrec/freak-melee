# Independent Ness activation preparation

This is an unregistered candidate and console/package gate. It does not change
`examples/ness_wip.fms`, the copied base, prior accepted components, an installed
package or a game. It reuses the frozen `ness_fighter_composition` assembler and
adds the normal `fighter: behavior independent` directive to its scratch output.
The source remains organized in the accepted component namespaces; no duplicated
3,000-line handwritten template or forwarding gameplay handler is added here.

Run with a fresh output directory:

```powershell
python tests/native/ness_fighter_activation/prepare.py --snapshot .local/baseline49 --output .local/ness-fighter-activation49-review --iso "C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso"
```

The runner hashes SDK/compiler/tool/binary snapshot inputs, normally clones Ness
assets, imports editable parameters, emits C and compiles a real DLL against that
SDK using Clang/VC14.44. It avoids frozen `script-build`'s embedded live include
paths. It loads that actual DLL with the actual copied-base metadata and a strict
unused Host boundary; every non-NULL Definition and article gameplay callback is
verified with `VirtualQuery` to belong to the DLL. Common cameras and immutable
metadata are explicitly allowed outside it. Required unused Host calls fail if
executed. Finally `bundle-project` and `verify-package` use the frozen native
Host/runtime and a new isolated store. There is no `accept` or install call.

The SDK49 gate passes 856 ownership checks: 31 Definition callbacks, all142
original non-NULL gameplay phases and 118 article callback pointers. The native
package validator accepts `source_kind=None` against the raw Ness catalog base,
then registers and releases the package. Animation binding reports 285 own-rig
clips and four borrowed clips. Borrowed clip data and `animation_source_kind=Ness`
are animation/retargeting dependencies, not a stock gameplay callback family.
Normal project metadata remains `base=Ness`, `code=ness_clone`.
The normal clone's fighter/animation/costume DATs and editable CPU profile are
package-owned data and are independently hashed in the receipt. Their stock
origin is not stock executable behavior inheritance.

## Closure audit

`audit.json` pins the actual SDK sources for these decisions and records the
minimal proposed production changes. `census.json` records every copied stock
callback slot, both original NULL model callbacks, all36 motions, and original
fighter/article source hashes. The full source comparisons previously accepted
in `ness_fighter_composition` remain their behavioral evidence; this gate adds
independence/physical callback provenance and actual package validation.

- Load/reload/reset are authored. Immutable 11 article registrations replace
  original OnLoad registrations. The original NULL user-data/death1/model/demo
  slots stay absent. Fresh native handles explicitly initialize to -1.
- One damage/death2 chain owns YoYo -> Flash -> Thunder -> Bat cleanup. It retains
  callback clearing order and independent creator/holder identity. YoYo pause and
  resume install at creation, even on allocation failure. Death reset forgets
  references after ordinary shared damage cleanup, without extra removal.
- Fire/Magnet/Flash/Thunder own eight special entries, all phases and all eleven
  article tables/signals/constructors. Every retained callback physically belongs
  to the DLL. Original empty IASA bodies are empty authored bodies; original NULL
  Fire IASA slots become explicitly tested no-ops. No raw Ness union is emitted.
- Three full smash entry hooks replace ftNs_AttackS4/Hi4/Lw4_Enter; all seven
  states and 28 callbacks are owned. Bat/YoYo use their supplied creation origins.
  The explained yo-yo timed-rehit cleanup-latch correction remains intact.
- Both pickup/drop pairs, visibility and knockback use authored common selector
  decisions. Ness has no stock auxiliary attach/visibility callback. Standard
  retained item slot notifications/destruction remain common machinery.
- Aerial entry and physics own the Ness decision, with generic Jump Begin,
  saved-accumulator DriftFrom and adjusted animation-Y. The authored worn-item
  sound predicate supplies false to common ItemScrew and multi-jump routes.
- CPU attack/travel/recovery/special-candidate/projectile-contact decisions own
  all Ness-specific branches in ftcpuattack/ftCo_0A01. Travel count increments
  remain common. Unhandled decisions continue ordinary shared planning; no raw
  Ness fallback runs when the authored hook exists. Candidate endpoint Float
  judgments are preserved from the accepted source comparison.
- `presentation_item` owns star-ice, camera-ice and sleep Bat construction and
  star scaling after shared setup. Ordinary down/right KO constructs none.
- Ness has no special action-metadata/status, idle-variant, angled-motion,
  dynamics-wind/interaction or model-hook decision to reproduce. Shared generic
  fallbacks retain the same ordinary policy for a custom independent kind.
- Effect1262/1263 are standard joint-follow model constructors;1264 uses ordinary
  asynchronous attachment;1030 is rebound presentation. Effect sources contain
  no Ness union/kind updater. Generic construction/follow/lifetime are engine
  presentation contracts, not retained fighter handlers.
- Results/intro/ending/viwait root strings and clips remain owned copied metadata;
  no additional Ness demo callback exists. Kirby-copy behavior is deferred. A
  Sheik call to the collision utility named ftNs_AttackHi4_YoyoCheckEnvColl is
  unrelated to this Ness callback family and is not modified.

## Proposed activation changes

After root's independent gameplay acceptance, replace only
`examples/ness_wip.fms` with the generated full independent source, updating its
completion comments and the Ness translation ledger. Existing
`native_shell_workshop_editor.cpp` already chooses that filename for Ness, and
CMake already ships it to `ui`; neither needs a new entry. No catalog, rig,
article-slot or runtime registration change is needed. Keep the raw copied base's
`source_kind=Ness` fallback for untouched clones; generated independent modules
set their own `source_kind=None` and do not retain donor gameplay pointers.

Remaining acceptance is real hidden/muted/drawn whole-fighter behavior and its
resource interactions, lifecycle/stock loss, costume, CPU and common item routes.
This gate does not prove pixels/audio, results rendering, Workshop interaction,
rollback/netplay execution or every stage trajectory. Do not install or advertise
this candidate as complete based on console validation alone.

## Default template activation

Root reproduced this gate on SDK49 and SDK50, then applied the reviewed
independent template to `examples/ness_wip.fms`. The SDK50 candidate and shipped
template have identical non-comment statements. Fresh Workshop clones use it;
saved drafts remain authoritative. The runner still creates only an isolated
unregistered package and does not install it. Whole-fighter gameplay acceptance
remains pending. Root receipts: `.local/ness-fighter-activation49-root/receipt.json`
and `.local/ness-fighter-activation50-root/receipt.json`.
