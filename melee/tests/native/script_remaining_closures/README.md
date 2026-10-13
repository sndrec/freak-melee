# Remaining binding closure census

Run `run.py --compiler <MSVC-target clang> --output <A: archive>`. It reuses the
actual production lexer and balanced chain census across all example FMS files,
then compares against an exact compiled extraction of the immutable native catalog
and current event payload names. See [the closure audit](../../../docs/native_script_remaining_binding_closures.md).

Name/context/access/arity presence is not typed or runtime acceptance. The JSON
preserves selector arguments, authored helper/table distinctions and cross-owned
state accesses. Missing paths, argument shapes, recipient contexts and unknown
families remain explicit. No native implementation, game or legacy C generator
executes. Build/run subprocesses are hidden.
