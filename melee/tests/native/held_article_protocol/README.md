# Authored held-article compiler protocol

Scoped compiler/generated C/generated DLL proof; no game or Peach/default activation.
Runtime dispatcher, registry/provenance and common-caller continuation checks are
root-owned separate fixtures. No stock handler or donor forwarding is emitted.

## Syntax and contracts

An article declares both fixed callbacks or neither:

```text
    on held_status:
            if item.motion_state == 1:
                return 4
            return -1
    on held_request:
            if event.action != 4:
                return false
            item.animation_rate = event.frame_rate
            return true
```

`held_status` returns Int, missing return defaults-1. It is a read-only query:
conditions, ordinary local computations, item/attribute/state reads, constant
returns and whitelisted pure numeric/held-reference queries are permitted.
Authored state or engine writes, RNG, loops and helper calls are rejected with
`held_status is a read-only query`. Calls are checked by source tokens, not C
substring matching; quoted strings cannot disguise a mutating call.

`held_request` returns Bool, missing return defaultsfalse. Its event.action is
Int, event.frame_rate/event.duration are Float, and these arguments are legal
only in this callback. General request actions retain existing generation guards.
Callback output types are strict; ordinary typed locals preserve existing DSL
assignability, including Bool<-Int truth conversion. Ordinary item/other article
Bool callbacks retain their existing behavior.

Article RO queries `holder.held_id/kind/motion/frame` inspect the current owner;
`targets.held_id/kind/motion/frame(IntTarget)` accepts an explicit full-width
creation ID. First three results are Int; frame is Float. Host resolves live
fighter and actual held item; missingID/kind/motion=-1 and frame0. No creator or
saved-handle substitution, pointer escape or setter. Plain `item Name` callbacks
are outside this article compiler extension.

## Worker evidence

Final successful worker receipt:
`.local/held-protocol53-worker-v4/result.json`.

- Compiler131 checks: paired schema, exact signatures/default returns, strict
  output types, illegal event contexts, readonly mutation/RNG/helper/loop rejection,
  invalid query argument/type/count/member/call/setter context, required Hosts.
- Generated C console639 checks and separately compiled/loaded DLL642 checks.
  Both execute32 rate/observation cases, healthy state/status mapping, high-bit
  target/held IDs, missing observed targets, unsupported requests, default handlers,
  unchanged ordinary Bool truth, and2 removed/recycled source continuations.
- Generation stub validates exact GObj identity, Item generation and class1; it
  is not an always-true liveness stub. Observations and transition are controlled
  Host dependencies, so this does not establish runtime dispatcher policy parity.

Historical failed worker/v2/v3 directories remain as diagnostics: the first
negative incorrectly expected Bool<-Int rejection; that restriction was removed.
Generated void forward declarations were corrected for the appended typed handlers.
The DLL harness then needed actual ItemLogicTable declaration header. Successful
v4 uses current live SDK/compiler sources with the frozen52 compiler executable
as a toolchain only, not a claim that the new ABI belongs to snapshot52.

## Reproduction / root CMake recipe

```powershell
python tests/native/held_article_protocol/run.py --root . --compiler .local/baseline52/compiler/clang.exe --output .local/held-protocol53-root-repro
```

The runner compiles only compiler/console fixture/DLL targets, invokes no game,
and stores exact commands, diagnostics and input/output SHA256 values.

CMake compiler target: compiler_test.c + NATIVE_FIGHTER_SCRIPT_SOURCES +
src/melee/ft/native_fighter_parameters.c; include src and extern/dolphin/include,
MELEE_NATIVE=1. Run `compiler_test fixture.fms output/held_article_protocol_fixture.h`.
Module target: module_test.c with generated include directory and the same SDK
headers/definitions. No game source libraries required. Generated DLL variant
compiles the generated header as C/shared, then module_test.c with
HELD_PROTOCOL_DLL=1 and runs with the DLL path. Compiler52 toolchain is usable
against root's current headers; a new matching frozen SDK is needed for packages.
