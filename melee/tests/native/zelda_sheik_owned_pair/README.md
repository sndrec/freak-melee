# Owned Zelda/Sheik pair: proposed integration, console closure gate

This namespace does not register bases, change Workshop defaults, install a
package or run a game. The proposed generated modules declare independence;
the copied catalog bases deliberately retain stock fallback for raw, unscripted
clones. These are different contracts. A successful generated-module entry is
not evidence that the real paired asset/package pipeline accepted the modules.

## Reproduction on immutable SDK70

```powershell
python tests/native/zelda_sheik_owned_pair/probe.py --snapshot .local/baseline70 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/zelda-sheik-owned70-root
```

The runner checks every snapshot hash before and after. It invokes the actual
frozen `clone-stock` command for **both** members. SDK70 rejects each with exit1
and `Unsupported stock clone base`; no project is fabricated. It separately
stages named owned parameters from actual retail DAT bytes, generates the entire
independent member source, compiles each DLL and console probe using the matching
snapshot Clang/SDK and vcvars14.44, and executes the module ownership/lifecycle
gate. Snapshot `script-build` is intentionally avoided: it embeds its original
build-time include paths. Accepted dependencies are hashed and left unchanged.

The `census` subdirectory contains every original ftData row, every extended
motion row, all costume receipts, proposed base source, actual rejected command
and source hashes. The runner asserts the generated retail bases exactly match
the four checked-in sources in `bases/`. Those are proposed production sources,
not runtime delegates added to the candidate.

## Exact metadata and callback closure

| Member | Retail root | Runtime parts | Semantic mappings | Actions | Demos | Articles |
|---|---|---:|---:|---:|---:|---:|
| Zelda | ftDataZelda | 118 | 53 | 311 | 14 | Din0, Explosion1 |
| Sheik | ftDataSeak | 58 | 52 | 317 | 14 | NeedleThrow0, NeedleHeld1, Vanish2, Chain3 |

Both omission lists are empty. All five Nr/Re/Bu/Gr/Wh costume graphs and their
material roots are independently found and hashed; their complete physical
joint counts match the common part table. Both sound-bank mappings come from
actual `lbl_803BB3C0`, bank27, with copied mute mask0x44; the actual effect bank
is17. Four demo archive-root strings are owned literals and demo count14 is
copied immutable data. Sheik imports pose descriptors in slots4/5 using the
existing Chain format, joint mask0x30 and named path`"0"`; raw part1 remains
mapped, not omitted. These declarations are immutable construction facts.

The probe seeds **every** callback field in the exact SDK Definition with a
failing donor sentinel, then opens the actual generated DLL. Every surviving
Definition callback, article callback/state/signal/export and recipient export
must reside in that member DLL. All72 Zelda and96 Sheik extended gameplay phase
slots are replaced; original animation ID/flags/move ID and common camera slots
are preserved. Owned parameter counts are83/103. Article logics cover all2/4
bindings. Inherited item tables and optional neutral delegates are cleared.

The stock table's nonnull decision rows map as follows:

| Original rows | Authored policy |
|---|---|
| OnLoad / per-kind LoadSpecialAttrs | copy/reload attributes; Sheik wall-jump flag; load-only owned handle initialization |
| OnDeath | ordered group0/group1 visibility plus owned handle/count reset |
| Eight special entries / entire extended motion table | complete accepted Neutral, Side, Up and Transform components |
| PickupExt/DropExt and ordinary Pickup/Drop | same original flag and1/1 arguments |
| Invisible/Visible | original presentation argument1 |
| KnockbackEnter/Exit | original argument1 |
| Sheik UnkMotionStates4 | editable needle saturation6/color86 policy |

Both original landing, user-data-remove, absorb, late-update, matrix and demo
callback rows are NULL; they do not require invented handlers. The independent
clear removes donor optional callbacks. `ftSk_Init_80110198` is the dynamically
bound damage/death2 cleanup, **not** a late-update row: Needle cleanup precedes
Chain destruction. Zelda's dynamic damage/death2 cleanup owns Din detachment.
These ordered decisions remain in the accepted components. Static article
registration formerly performed by OnLoad becomes the owned immutable asset
binding table, established by runtime package registration before OnLoad.

The new fixture executes unchanged complete original OnLoad, OnDeath,
LoadSpecialAttrs, item/visibility/knockback and Sheik color-reset bodies against
the generated callbacks. Shared copying/presentation operations are controlled
boundaries; the fixture validates exact calls/arguments/order. Original NULL
article pointers map to the authored invalid handle -1, while retained counts
remain exact. The stock OnLoad registration tuples are checked against the
immutable binding table rather than imitated by script registration calls.
The two visibility groups are a controlled valid fixture model, not acceptance
of actual costume DObj registration. Destructive lifecycle behavior and all
move/article matrices retain their separate accepted source receipts.

## CPU and common policy census

Zelda's ftcpuattack attack/travel Din branch is replaced by the complete
`zelda_din_cpu` source component. It uses primary-root **LOCAL** translation,
the original nearest eligible fighter3D/list/tie/filter rule and current target
position. The documented binary32 release-threshold deltas remain explicit in
that source receipt. Zelda travel does not increment its counter. Zelda recovery
replaces the original `ftCo_800A949C` choice; both members own the exact pending
action branch in the two common priority points. Sheik has no additional named
CPU decision branch. All other CPU scheduler behavior remains common machinery
with each member's imported own CPU profile, not a raw donor callback.

Common motion/landing/jump/item routines stay engine primitives. No member
requires an authored special jump policy. Zelda's DbLevel_DebugRom dress toggle
is a diagnostic-only taunt branch, deferred; ordinary versus taunt remains
common. Kirby-copy hat/dynamics are excluded as requested.

One real cross-fighter boundary remains: raw Bowser/GigaBowser forward throw in
`ftCo_Throw.c` selects victim motion243 through raw Peach/Zelda kind equality.
Independent custom Zelda no longer has that raw ID. The shipped authored Bowser
already uses the victim animation-family contract and therefore handles the
custom correctly. Raw Bowser versus custom Zelda needs the corresponding
immutable animation-family fallback correction or equivalent generic recipient
policy before claiming all-opponent presentation closure. No core change is
included here.

## Proposed production integration, not applied

`catalog.patch` is generated against the immutable70 inputs; its JSON sidecar
pins original source hashes. Regenerate it for review with:

```powershell
python tests/native/zelda_sheik_owned_pair/integration_patch.py --snapshot .local/baseline70 --census A:/FreakMelee-test-archives/zelda-sheik-owned70-root/census/census.json --output A:/FreakMelee-test-archives/zelda-sheik-catalog-review.patch
```

It adds the actual Zelda/Sheik raw base declarations, two runtime catalog rows,
two project names, six article slot rows and supported CLI help names. Existing
generic import recipes already handle the Chain format and pose slots. Existing
CMake's recursive game-source glob discovers the bases; there is no shim/manual
source-list addition. The existing Sheik unregistered base is replaced directly
by equivalent retail metadata, not aliased or duplicated in production.

After registration and a matching snapshot, the actual intended route is:

1. `clone-stock` each Single member, followed by ordinary `import-parameters`
   of `compose.py`'s corresponding source into that project's `behavior.fms`.
2. Snapshot `script-c` with that actual project's code key, then direct matching
   SDK Clang compilation to `module.dll`; verify parameter/module hashes.
3. `bundle-project` and `verify-package` each member in an isolated content
   store, then `compose-family alternate TITLE ZELDA_ID SHEIK_ID` and
   `verify-package` the result. Both module Definitions must be independent and
   supply `on_form_activated`. Raw copied bases must never pass the independent
   family gate merely because their rigs are owned.
4. Check both own rigs/animations/action/demo/article/pose tables and per-member
   DLL/FMP/CPU bindings, then actual destination-hook/state-preservation transfer
   and reflected/cleanup lifecycle cases. Existing generic family tests exercise
   infrastructure; they do not establish this pair's actual assets or gameplay.

Member0 is the initial Zelda form; swapping order creates a Sheik-first family,
not a claim that role1 is simultaneously active. Each form retains its separate
fixed authored state while inactive; OnLoad seeds it, OnDeath resets it,
`form_activated` completes the owned destination finish. Ordinary transfer does
not copy or reinitialize the other form's state. Costume indices are matched by
the independently decoded retail Nr/Re/Bu/Gr/Wh order; future variants must own
both member payloads, not borrow a costume graph from the other member.

Workshop lists supported Single clone bases via StockBaseName, so the catalog
rows enable discovery. Its default-template switch currently has neither
member. **Do not add default templates yet.** After the real asset gate and
root-coordinated gameplay acceptance, proposed templates would be `zelda_full`
and `sheik_full`, assembled from these components and shipped by the normal
example/UI copy path. Existing `CreateFromPackage` intentionally accepts only
Single packages; editing a composed alternate family as one Workshop draft is
a separate product capability, not proof against the real console composition
route. No gameplay, pixels, family installation or full paired activation is
accepted by this namespace.
