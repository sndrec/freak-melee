This test-only oracle fixture verifies `program.attrs.<root_parameter>` emits
the existing shared `script_parameters[index].i/f` array for Articles and Fighter
code. Integer, float and Bool result types are preserved. It also checks the
ModelEffect name admission and complete source generation.

The real parameter importer runs against a private synthetic DAT: root tuning
is imported into the FMP and retained as an owned literal `parameter`
declaration. Explicit program reads remain byte-preserved, and Article-local
`item.attr` becomes `item.params`. Root and Article declarations remain in
their original scope for ordinary interpreter indexing. Full parameter-name
width is tested separately.

Explicit program attributes cannot fall back to a legacy Fighter DAT offset,
an Article-local attribute, a creator query or an unknown name. Existing `attrs`
and `item.attr` behavior is unchanged. The fixture does not exercise ordinary
source loading, the bytecode VM, native runtime or complete Game & Watch behavior.

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: directory>`.
