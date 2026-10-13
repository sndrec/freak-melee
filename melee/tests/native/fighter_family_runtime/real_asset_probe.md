# Proposed next gate: real two-member asset preflight

This is a bounded proposal after the controlled-import orchestration matrix.
It is not executed evidence and requires root coordination/build ownership.

## Existing actual route

`src/melee/ft/native_fighter_runtime.c:prepare_family` parses one complete family
packet, then calls actual `prepare_member` for every declared member. The latter
opens that member's DAT, identifies its copied base, opens a member-indexed DLL,
applies that member's FMP, validates the Definition and imports owned character,
animation and costume arenas, article/model metadata, omissions and CPU data.
`nativeFighterModule_OpenCached` in `native_fighter_module.c` uses distinct
`modules/<family SHA>-member0.dll` / `member1.dll` paths. Pairing requires both
real libraries and `source_kind == Ft_Kind_None`; AlternateForm additionally
requires each Definition's `on_form_activated` callback.

`tools/native_content_tool.c:verify_package` already invokes real
`ValidateCached` and whole-family `Load` after loading common data from the ISO.
However, its positive table/Definition/animation assertions and final cleared
table assertions inspect only `Ft_Kind_CustomFirst`. That is a coverage limit,
not proof of a failed secondary import. The current flat project authoring path
builds Single packets and explicitly rejects paired package import. A family
authoring format is not needed for the following first asset gate.

## Minimal coherent console gate

1. Use two existing genuinely independent, supported normal clone projects
   with distinct rigs and FMP schemas, for example Link and Game & Watch. Build
   each normally against one coordinated immutable SDK. Preserve their exact
   package bytes and all source/DAT/FMP/DLL hashes.
2. Parse the two Single packets with the actual codec. Construct one
   SimultaneousCompanion packet with member keys `main` / `partner`, copying
   each actual `.members[0]` payload intact and retaining each code key. Use the
   actual family writer/hash/parser, one portrait and one identity. Do not
   manufacture Definitions, DAT attributes or helper callbacks.
3. In one console process linked to real runtime/import/module/store code,
   cache it under an isolated A: store, load common ISO data, call detached
   validation, then StageCached and ActivateStaged. Assert both member
   Definitions are independent and distinct; both actual tables/parts/costume,
   motion/animation/CPU bindings exist; the CSS mapping names both actor kinds
   with alternate=false; each module has the intended member-indexed path;
   the active sound mask is the union of both Definitions.
4. Assert each member's article author/metadata and FMP values bind to that
   member, never the primary package layout. Check physical versus runtime rig
   counts from the actual imported Definitions/omissions, not guessed equal
   counts. Link and Game & Watch exercise differing rigs and omission recipes.
5. In the same bounded gate, test malformed second-member FMP, mismatched second
   costume rig and invalid second DLL. Require actual import rejection and no
   partial mapping; retain an outgoing real family during failed staging and
   verify its tables/Definition/parameters remain unchanged. Clear/reload using
   the real runtime path and check both actor tables and mappings are retired.

These are actual asset/preflight and publication checks, without creating
fighters, running the renderer, installing into the human store or launching a
game. Use fresh A: output and a separate scratch store. The compiler/runtime
inputs must match one frozen snapshot; do not combine live headers with67.
Root owns any linked gate target/CMake/core changes. This proposal introduces
no new package schema, wrapper or compatibility reader.

## Separate later evidence

The artificial Link/Game & Watch family is a generic staging probe, not a claim
of Zelda/Sheik or Ice Climbers behavioral closure. AlternateForm should be
tested separately with real authored form-completion policies, rather than
inventing no-op completion handlers for this gate. Player role swapping,
secondary Sleep/startup, shared-life results/death rules, companion control,
costume selection, rollback and drawn gameplay remain separate evidence.
