# Direct fighter DAT parameter closure

```powershell
python tests/native/script_metadata/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-metadata-oct07
```

The executable runs synthetic-only checks without arguments, suitable for
CTest. Passing DAT path, FMS source path and fighter root additionally verifies
the actual Mario template's root attribute imports against immutable project
DAT bytes. The runner records those inputs' hashes and starts all subprocesses
with CREATE_NO_WINDOW on Windows.

`native_script_metadata.c/h` follows the actual fighter root's special-attribute
pointer at root+4, using the native DAT reader and the next serialized target
as the attribute block bound. Indexed root Attribute declarations become typed
immutable ParameterSupply values: aligned big-endian Int/Float words, Boolean
bytes restricted to 0/1, and unsigned byte imports. Float words preserve their
exact binary32 bits. No source text, DAT data, native behavior table or running
actor is changed; no generated-C importer or legacy parameter parser is called.

The importer validates every root candidate before publishing supplies, count
or report. Tests cover word/byte types, negative integers, end-of-block reads,
bad alignment/ranges, nonfinite Float, invalid Bool, missing roots/pointers,
truncated DAT, malformed declarations and exhausted output capacity. Failures
preserve all previously supplied output bytes/count/report.

The real template has 31 root imports in a 132-byte fighter block. Its four
entity imports remain explicitly unresolved; they require their own article
definition/assets mapping. They never read the fighter block by accident.
Ordinary parameter defaults retain their parsed authored values and are not
overwritten by DAT imports. Metadata/package/motion acceptance and expression
symbol bindings are separate gates; this closure does not claim full Mario
body execution or installation.

Integration uses the compiler's load-time parameter-provider contract: derive
actual declaration IDs from the candidate index, call ImportFighterParameters
into caller-reserved supplies, and copy those values into the immutable table.
The typed values remain pinned during play; no runtime DAT pointer is needed.
