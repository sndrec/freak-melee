# Composed CPU floor policy checkpoint

2026-10-10. Yoshi, Bowser and shared CPU fixtures express native floor-only
blocking through existing `stage.read_floor` and `stage.cpu_floor_excluded`
operations, with the second query short-circuited when no floor is found.
The all-geometry segment query includes walls and ceilings; an unfiltered
floor result alone would miss native stage exclusions.

No binding was added. The obsolete reference-exporter `stage.floor_between`
shortcut and host slot were removed. Reference consumers use the same two
operations. Current scripting documentation describes their composition.

## Verification

The connected stage cohort passes **15,917 checks / 1,584 cases / 1,416 native
comparisons / 734 VM calls / 83 negative cases**. Its 640 new comparisons run
the compiled predicate against `ftCo_800A0FB0` and `ftCo_800A1B38`, pinned to
`76ccf8a70` with only entry/internal predicate names changed. Both segment
directions, 16 contact masks, five stage kinds and four joint identities cover
no contact, accepted floors and stage exclusions. Bool results, native floor
iteration counts and bounding scratch match.

Receipt: `A:/FreakMelee-test-archives/script-cpu-floor-compose-oct10-final/receipt.json`.
SHA-256: `ec9a4cdc9ec10aff1b61034e76bddf017a18ddf55bf6d3684460bb8a22bb2717`.
All 1,753 pinned inputs were independently rehashed unchanged.

The client, content tool, family/package runtime targets, capture enclosure,
test-only C reference exporter and shared-policy fixtures build. CTest
`native_fighter_script` and `native_fighter_common_policy` both pass. Rebuilding
the move fixture DLL after host-slot removal resolved its stale-DLL load failure.

The ten-source preparation cohort retains Captain Falcon, Ganondorf, Marth,
Roy and Luigi success and adds **Yoshi: 263 functions / 87 imports / 6,246
instructions**. No function body was removed to make preparation pass.
Fox stops at entity metadata, Falco at an owned asset binding, Donkey Kong
at a missing binding, and Bowser at an earlier expression conversion.

Source receipt: `A:/FreakMelee-test-archives/source-cpu-floor-compose-oct10-final/receipt.json`.
SHA-256: `4bf3ae67e86a77dcd1c3991dbf4e2524ef26c03c3dcac9081b49fb1306bd3209`.
All 27 pinned inputs were independently rehashed unchanged.

## Limits and remaining work

Native floor iteration and stage exclusion bodies execute. Segment intersection,
endpoint resolution and dynamic response leaves remain controlled. This compares
the compiled shared predicate, not whole authored Yoshi/Bowser CPU scheduling
or real moving-stage solver behavior. Source preparation does not establish
constructor, article, package or rendered gameplay acceptance.

The historical Yoshi and Donkey/Bowser C-reference call sites were migrated but
those complete fixtures were not run: the old closure extractor rejects the
live host layout, and Yoshi's extracted live scheduling bodies need interpreter
and lifetime declarations absent from the frozen fixture. These are separate
from the passing ordinary interpreter cohort and shared-policy CTest.

Remaining roster integration, full-fighter acceptance and compact whole-match
rollback remain open. No game/Dolphin launched, installed content changed or
remote push occurred; the overall goal remains active.
