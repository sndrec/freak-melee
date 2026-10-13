# Source resource codec fixture

```powershell
python tests/native/source_resources/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/source-resource-codec-new
```

Use a fresh A: directory. The runner compiles only the actual new codec and this
fixture, checks its lightweight C++ header, then runs the console executable.
All child processes are hidden and bounded. It hashes the unchanged eight Rook
PNG inputs and records commands/logs/receipt; no game, global build or install.

Acceptance is 12,244 checks / 10 cases at
`A:/FreakMelee-test-archives/source-resource-codec-four-mib/receipt.json`.
This is structural PNG/container and canonical byte ownership evidence, not
pixel decompression, native visual preparation or source package activation.
See `docs/native_source_resources.md` for the exact wire format and lifetime
contract. Opaque blob bytes intentionally have no embedded schema validation.
