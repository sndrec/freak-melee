# Fighter setup/catalog fixture

Run from the repository root, with the archived MSVC-target Clang and a fresh
output directory on A:. The runner never opens a game or compiler window:

```powershell
python tests/native/script_setup/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-setup-review
```

Strict C11, `-O2`, `-ffp-contract=off`, warnings-as-errors. The receipt records
compiler commands, source hashes, stdout and exits. Engine dependency bodies
are extracted using the existing state/kinetics fixture extractors; item
presentation is compared with the real `Fighter_OnItemPickup` inline body.
Presentation leaves and callback actor bindings are controlled here. Real
owned callback dispatch is tested separately in `script_fighter`.

Checks include attribute scratch ownership, high-bit signed command values,
wide selector rejection, cues, CodeRef/None, actual source-to-bytecode execution,
read-only command use and rejection, and actor retirement before subsequent
authored writes. This does not claim game parity or model/article closure.
