# Binding inventory fixture

Run from the melee repository with an isolated output directory:

```powershell
python tests/native/script_binding_inventory/run.py --output A:/FreakMelee-test-archives/script-binding-inventory-oct07
```

The runner builds only `lexer_census.c` and the shared production
`native_script_expression.c`; `clang` is a host C compiler for this diagnostic
executable. Ordinary fighter loading does not invoke that compiler. The fixture
uses the actual lexer, including strings/comments, rather than a regexp lexer.

The structural probe retains resource and joint selector arguments, typed-query
output destinations, array indices and recipient context. It separates authored
helpers/tables/state/parameters from engine operations and leaves an unknown
operation explicitly unresolved. It checks that comments/quoted names cannot
create fake operation entries and mismatched delimiters produce a diagnostic.
Its unsupported helper context is intentional: lexical census does not prove
semantic acceptance.

The corpus pass includes the ten selected full templates and six Rook files.
The initial frozen output reports 723 native operation families, 9,406 uses,
63 source-checked used contracts and 660 unresolved signatures, with zero
executable-line lexing/framing failures. `manifest.json` carries each source
hash, each selector chain and occurrence location, source-reference candidates,
contract source hashes and explicit unresolved status. `receipt.json` records
the narrow passing scope. These files live in the output archive, not the repo.

`contracts.json` is the source-checked initial family described in
`docs/native_script_bindings.md`. Reference-only legacy return-type hints are
labelled; the compiler's old default-to-Float behavior must not become a typed
catalog rule. No test here claims all operations are type-checked, installed,
executed in the game or covered by source-retirement runtime fixtures.
