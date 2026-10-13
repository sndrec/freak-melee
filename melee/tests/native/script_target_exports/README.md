# Recipient export fixture

Original zero-argument cohort: **757 checks / 86 cases / 47 top-level VM calls**, three independent
program banks, 18 injected callback mutations. Receipt:
`A:/FreakMelee-test-archives/script-target-exports-final2/receipt.json`.

The actual source frontend, export/name index preparation, native binding catalog,
bytecode, Fighter program Prepare/Publish, Fighter and Item actors, Execution, VM,
storage, receiver scopes and GObj pools run together. Calls execute the captured
recipient's own program: matching `Shared` exports have different local function
IDs and return 111/222/333 in the independent banks. No successful export lookup,
helper execution, actor lookup or metadata-validation substitute is installed.

The grouped cases cover:

- Fighter and Article callers; exact Bool/I32/F32 returns, signed zero, Void/I32
  delivery with discarded return, missing handles/exports, private helpers and
  valid but mismatched typed ABIs.
- Full native 32-bit handles, including signed high-bit representations and
  `UINT32_MAX`; malformed widths/types/counts, Text spans and prepared name tokens
  leave outputs unchanged.
- `costarring` and `liquid`, both FNV-1a key `0x5e4daa9d`, dispatch to distinct
  exports. Repeated literal names at different source offsets deduplicate and
  execute correctly.
- Actual root export rejection for more than 30 parameters, U32 return, duplicate helpers and
  Article ownership, and export declarations longer than 127 bytes. Unsupported `double` syntax is rejected by the existing
  helper type parser. Empty/malformed names and malformed call argument types
  fail compilation without altering the accepted independent bank buffers.
- Self-call and three-bank nested delivery. A dynamic self-cycle exhausts the
  shared fixed VM budget, preserves the output/tail and leaves runtime frames,
  registers, payloads and receiver scopes balanced.
- Recipient removal, same-slot physical reuse, native generation retirement and
  Detach produce typed neutral values or delivered `call=true` while the caller
  survives. The replacement is never reacquired. Userdata/class substitutions,
  initially faulted recipients, recipient VM faults and caller retirement/fault
  terminate publication. Captured state-byte observations confirm caller result
  and subsequent tail writes remain untouched on terminal paths.
- Ordinary native position mutation in a surviving recipient remains accepted;
  malformed recipient export count/function/ABI is an error, not a missing name.

The current argument cohort also executes Bool/Int/UInt/Float parameters through
all four dispatch forms from Fighter and Article callers. The maximum 30-helper-
argument case fills all 32 native call slots. Two 128-case matrices compare
mixed numeric conversions and exact results while preserving caller input
buffers. Wrong arity/type/prepared counts, unsupported values and corrupt
recipient parameter metadata reject before helper execution. The existing 18
lifetime mutations now execute a parameterized `Notify(float value)` export,
whose native position write consumes that argument. Thus the caller/recipient
retirement checks cover parameterized execution rather than only zero-argument
calls. No whole-match or native renderer acceptance is claimed.

Run hidden console processes with a fresh A: output:

```powershell
python tests/native/script_target_exports/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-target-exports-final2
```

The runner fingerprints source/header inputs before and after execution. New test
syntax uses `-Wall -Wextra -Werror`; linked production units use `-Werror`. The
existing SDK duplicate matrix declaration suppresses `-Warray-parameter`.

Asset graphs, selected runtime package getter and physical Fighter/Item userdata
records are controlled. Actual program preparation/publication and actor storage
run; the fixture does not run SourcePackage asset import/runtime loading or the
production Fighter allocation pool. The position-write leaf updates the real
native field then injects explicitly artificial callback removal/reuse/fault
scenarios. Native geometry, rendering and unrelated operations are unavailable.
This is no DAT constructor, gameplay/rendering, native callback scheduling, or
whole-match snapshot acceptance claim.
