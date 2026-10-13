# Typed script constants

`native_script_constants.c` resolves exact shared native identifiers and audited
authored aliases while content loads. `nativeScriptConstants_TryResolve` returns
1 for a literal/namespace, 0 for an unrelated path, and -1 for an invalid owned
constant path. Compose it after owned locals, parameters and metadata symbols.
Scalar results use the shared resolver literal contract: the parser materializes
typed immutable IR, and bytecode contains the value directly. Constants require
no runtime native property lookup or source lifetime guard.

The catalog contains 1,052 names from current `Fighter_Part`, common motion,
`MotionFlags`, item kind/state flag/hold kind/unknown category, hit/hurt status,
collision mask, debug level, fighter animation-family, callback-slot, CPU and
shield-mode declarations. Public
native enum identifiers and the existing authored aliases are distinct entries.
For example, `LeftHand` is `FtPart_L1stNb`, not the adjacent wrist joint;
`Motion.held_fall_special` is `ftCo_MS_ItemParasolFallSpecial`. Alias evidence
points to the old source mapping only as an audit reference; no legacy compiler
or generated C is called by production.

Signed C enums and unsuffixed collision/axis constants stay I32. Native
`MotionFlags` stay U32, including bit 31; item state-change flags are C enums and
stay I32. Namespaces are case-sensitive: `Motion` differs from runtime `motion`.
Namespace IDs occupy `0x3d000001` through `0x3d00000b`; scalar constants have no
runtime semantic ID. Unknown names, members, calls, indices, and constant writes
are rejected. Quoted recipe names or joint paths remain Text values. Fighter
move-specific symbols such as `NeutralSpecial`, resource recipes, and current
runtime math bindings (`pi`, `tau`, `pi_half`) belong to their separate contracts.

The no-argument `test.exe` runs direct Source → ProgramIndex → Statement → typed
IR → Bytecode → VM cases, constant-write/unknown negatives, local shadowing,
signed motion -1 and unsigned high-bit flags. It checks that no host reads occur.
The standalone runner also compiles every catalog value against the actual C
declarations and independently checks authored alias mappings against the source
evidence. It verifies chosen native enum declarations are exhaustively cataloged.

```powershell
python tests/native/script_constants/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output A:/FreakMelee-test-archives/script-constants-oct07
```

`census.json` inspects the `*_full.fms` templates and Rook files with the
actual production lexer. It reports candidate spellings/uses for the supported
constant families, skips comments and string contents, and does not treat
post-dot property members as standalone enum references. This is a lexical
constant census, not body grammar/type checking or package/game acceptance.
The current selected 17-file corpus uses 76 distinct constants in 396 lexical positions;
none of those selected constant spellings is unresolved. Metadata, other binding
families, resource recipes and fighter-owned motion tables still require their
own validation.

`NativeScriptProgramIndex.constant_resolver` and `constant_context` provide the
same load-time literal recipes to owned parameter defaults. The index applies
its own constant-only gate: only numeric/Bool literals and namespaces may pass;
runtime-readable values, calls, indices, side-effect flags and parameter/state
references fail before evaluation. Every operand must belong to the scalar
constant language, while `and`/`or` retain lazy arithmetic evaluation. For
example, `true or (1 / 0 == 1)` succeeds, and an unknown name in the same skipped
operand fails. A missing resolver still allows ordinary numeric/Bool defaults.

Defaults are evaluated in fixed local scratch without modifying the program IR
arena. Only a completely evaluated value is appended to the declaration index;
failure keeps the existing zero-count invalidation contract and leaves the
failing declaration slot unchanged. Publication of an accepted program remains
the compiler bank's separate transaction. Tests cover F32 bit preservation,
integer wrap/sign, U32 high bits, Bool strictness, missing/unknown providers,
nonconstant operands and fixed expression capacity exhaustion.

The `uint` spelling is supported by parameters, helper formals/returns and typed
locals, whose existing value execution supports U32. `state uint` and
`attribute uint` fail explicitly: owned state storage currently excludes U32,
and imported `attribute byte` has a distinct one-byte DAT contract. This slice
does not silently turn that imported byte into an unsigned word.
