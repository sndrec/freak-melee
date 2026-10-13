# Canonical source family packet

The actual FMSC writer/parser owns one portrait and one or two independent source
member payloads under one content hash. Single, alternate-form and simultaneous
companion modes use the same 80-byte header and two 64-byte directory entries.
Each member has eight chunks: Fighter DAT, animation DAT, costume DAT, asset
facts, FMS source, parameters, CPU profile and resources. There is no nested
packet or old-format parser, and no generated C or DLL member payload.

The fixture checks byte order/spans for all three modes, distinct member keys
and source/resource payloads, every truncated length, duplicate keys, mismatched
mode/count, corrupted second-member facts/source, unused directory padding and
failure preservation of both parser outputs and builder outputs. Facts/resources
use actual production codecs with synthetic valid numeric facts; DATs are opaque
transport bytes here. This fixture does not import DAT graphs, compile FMS,
publish actor programs or establish gameplay parity.

```powershell
python tests/native/fighter_family_package/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/source-family-packet-review
```

The current CMake target `native_fighter_family_package_test` passes 5,877 checks
across 26 cases. CTest also runs the existing `native_content_store` fixture.
See `tests/native/source_family_project` for actual DAT/source preparation,
stock asset authoring, two-member project roundtrip and CLI failure isolation.
Runtime paired publication and complete form/companion behavior remain separate
gates.
