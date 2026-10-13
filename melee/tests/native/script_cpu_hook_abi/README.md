# CPU hook ABI

This bounded console fixture runs the actual source frontend, index/bytecode
compiler, metadata preparation, program publication, VM, shared runtime,
receiver scopes and Fighter actor. It drives every one of the 27 CPU static
hooks through `nativeScriptFighter_Run`, alongside all 28 original contracts
(two of those original contracts are CPU hooks). Independent expected rows
check all 53 names, argument shapes and return types.

```powershell
python tests/native/script_cpu_hook_abi/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-cpu-hook-abi-fresh
```

Every hook runs authored source that records its identity and increments actual
owned state. Typed payloads cover Bool flags, I32, F32, two F32 values, I32+Bool
and Object receivers. The CPU Prepare callback round-trips six native U32
history bit patterns through the current scheduler's I32 representation,
including `0x80000000`, `0xFFFFFFFF` and `0x87654321`. This preserves signed bits;
it is not an overflow-checked numeric conversion.

The test rejects wrong call arity/types and absent argument pointers without
changing output, state or actor health. Preparation rejects changed formal
counts, types and results without mutating its output candidate. An independent
program without hooks returns the exact typed neutral values and reports
`HasHook == false`; malformed calls still reject.

Object inputs use actual external-token capture, receiver import, same-program
Fighter views and owned state. Cases cover a live peer, a typed missing peer,
native generation change during the callback, physical deletion during the
callback, a stale token after same-slot replacement, and the replacement's new
token. Nullable `other.exists` correctly becomes false after retirement; a stale
external token rejects before publication. Caller VM fault and physical removal
stop the reached callback and preserve output.

The fixed GObj pool, allocation/free/reuse, Fighter actor storage and callback
execution are substantive. The existing frozen `script_fighter_scheduler`
boundary supplies ordinary position writes, native source context and explicit
unavailability of unrelated engine operations. Its extracted scheduler bodies
are compiled as shared substrate, but their execution is outside this fixture's
coverage. There is no fake Run, successful metadata placeholder or substituted
authored callback.

Both compilation and the console run use `CREATE_NO_WINDOW` with a 120-second
bound and a fresh private A: archive. Source/header/compiler hashes are recorded
and verified at completion. This proves the connected CPU callback ABI and
ownership behavior; it does not prove an AI scene, native decisions, assets,
rendering or gameplay parity. No game or global build is launched.

After the shared lifetime contract's direct rename, the fixture still checks
the first 53 contracts independently. Later non-CPU hook additions are covered
by their own connected scheduler fixtures, not this CPU test.

Latest receipt: `A:/FreakMelee-test-archives/script-cpu-hook-abi-generic-lifetime/receipt.json`: 1,786 checks, all 27 CPU hooks, 28 original contracts and 246 rejection cases.
