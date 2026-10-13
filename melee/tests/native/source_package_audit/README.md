# Source package loading census

See [the loader audit](../../../docs/native_source_package_loading_audit.md) for
the actual dependency chain, callback-pointer inventory and proposed transaction.
Run `run.py --compiler <MSVC-target clang> --output <A: archive>` from any working
directory. It indexes `examples/mario_full.fms` using the production Source and
event-signature frontend, then records native callback-bearing declarations and
current scheduler-name classes from source.

This does not compile callback bodies, validate package/entity metadata, install
a program, load a DLL or start a game. The output retains that boundary explicitly.
