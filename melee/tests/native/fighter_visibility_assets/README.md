# Fighter visibility asset backend fixtures

This namespace exercises implemented generic backend contracts. It does not
activate a fighter or validate shader/GX rendering.

`fixture.py` extracts actual visibility descriptor decoding, layout validation,
costume ordinal validation, runtime load contexts, private slot retirement and
table publication. `runtime_test.c` uses the real DAT view/graph importer and
real native creation-ID registry. Its little synthetic DAT includes two actual
imported DObj descriptors, slot15, duplicate ordinals and an empty variant.
No fake costume pointer substitutes for the imported descriptor graph.

The private package/cache binding boundary is controlled. Actual release_slot
executes, with its resource-cancellation, holdable cleanup and module-close sinks
observed rather than loading/unloading a DLL. Runtime DObj entries are explicit
controlled non-NULL objects; their visibility and material renderer is not run.
The full package loader and Fighter_Create allocation/continuation are outside
this fixture; only their actual backend functions are extracted.

Coverage includes all16 mask slots and disjointness, format reservations,
count/descriptor-span rejection, empty/duplicate selections, actual descriptor
ordinal rejection, live DObj NULL checks, high-bit source generation, absent
registry, retired PAGE_NOACCESS userdata, recycled generation, changed kind/
Definition/ft_data/group count, nested contexts, mismatched leave, package
retirement/rebinding, and balanced shared64 dispatch saturation. Every failed
installation preserves the existing table pointer and cleared flag; successful
installation preserves both initial cleared states.

Standalone console command (explicit compiler, current candidate source/header
hashes; this is not described as frozen59 SDK evidence):

```powershell
python tests/native/fighter_visibility_assets/run.py --compiler .local/baseline59/compiler/clang.exe --output .local/fighter-visibility-assets-candidate
```

The initial candidate receipt reports277 checks,0 failures. Inputs, actual
extracted bodies, executable and logs stay in the supplied ignored output
directory. `run.py` rejects inputs changed during compilation/execution.

`import_test.c` is a separate actual full import against the supplied USA1.02
ISO. It imports PlGw with the public stock mask and all four actual costume DAT
graphs, verifies the one-allocation typed/legacy alias, validates all original
lookup ordinals, and rejects an edited out-of-range ordinal on each graph.
It creates no display or live HSD model, and does not invent a stand-in costume.
The SDK61 full build and grouped CTest run pass. This target executes 74 checks
across the four actual costume imports; the runtime target passes 277 checks.
Evidence: `.local/sdk61-build-ready.log`, `.local/sdk61-tests.log` and the normal
CTest `Testing/Temporary/LastTest.log`. No rendered-pixel acceptance is claimed.

Root CMake integration requirements:

1. Generate `visibility_actual.inc` with Python `fixture.py --root SOURCE
   --output BINARY_DIR`. Dependencies: fixture.py, item_owner_fixture.py,
   ftcharacter_assets.c/h, ftcharacter_asset_layout.h,
   native_fighter_runtime.c/h, native_callback_lifetime.c/h and fighter.c.
2. Console target `native_fighter_visibility_runtime_test`: runtime_test.c,
   native_callback_lifetime.c, dat.c, dat_import.c, dat_model.c, dat_geometry.c,
   dat_material.c, dat_animation.c, dat_scene.c, dat_texture.c. Include src,
   extern/dolphin/include and generated directory. Definitions MELEE_NATIVE=1,
   _USE_MATH_DEFINES, _CRT_SECURE_NO_WARNINGS. No melee_runtime link because the
   extracted actual private slot functions replace those translation-unit
   definitions in this scoped fixture. Add normal executable CTest.
3. Full-import target `native_fighter_visibility_import_test`: import_test.c,
   linked exactly like native_fighter_character_assets_test (melee_runtime and
   its Aurora dependencies/runtime DLL copies). Run with `${MELEE_ISO}`.

Production public additions: `FtCharacterAssetLayout.visibility_mask` (u16);
`FtCharacterAssets.visibility_assets[16]` (FtPartsVisLookup*);
`ftCharacterAssets_ValidateVisibility(assets, mask, costume)` Bool;
runtime EnterLoad Bool, LeaveLoad Void and InstallOutlineVisibility Bool.
The existing dispatch-limit64 constant moves to its actual public lifetime
header. Fingerprint the new layout field/size in the coordinating module batch.
