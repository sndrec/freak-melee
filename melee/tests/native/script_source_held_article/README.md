# Direct source held-article dispatch

Run from the repository with the frozen compiler:

```powershell
python tests/native/script_source_held_article/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/source-held-article-new
```

The output directory must be fresh. Compiler and fixture processes use
`CREATE_NO_WINDOW`; no game, render, sound, installation, or global build runs.
The receipt records exact commands and input hashes.

This links the actual held dispatcher, parser/compiler, event contracts, Item
owner/storage/VM, receiver, GObj pools and lifecycle. Immutable publication specs
and native Item userdata are controlled; there is no Fighter authored view or
`NativeFighterDefinition`. The shared Item-owner boundary supplies a controlled
position write for state change/removal, plus explicit holder mutation injection.
No held callback is replaced with native machine code.

Cases cover typed status, request acceptance/refusal and exact F32 payloads,
ordinary state change, absent hooks, live faults, detached/pending-removal Items,
neutral removal, changed holder binding/generation, and actual holder slot reuse
with the same native spawn ID. The latter proves the native pool generation
rejects an otherwise matching pointer and spawn generation. Generation zero is
accepted. Ordinary empty-held operations remain unclaimed.

The actual common Fighter held callsites are source-audited in
`docs/native_source_entity_definition_audit.md`; this fixture executes their
dispatcher rather than the entire Fighter animation/state path. The separate
legacy `held_article_backend_test` continues checking C-policy behavior with
interpreted owners explicitly absent.

Acceptance: `A:/FreakMelee-test-archives/source-held-article-final-review/receipt.json`
has 88 checks / 14 cases. The separate legacy receipt at
`A:/FreakMelee-test-archives/source-held-legacy-review/receipt.json` has 354 checks.
