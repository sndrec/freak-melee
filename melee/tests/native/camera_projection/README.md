# Native camera projection and quake regression

Run `run.py --compiler <clang.exe> --output <fresh A: archive>` from a Visual
Studio developer environment. The executable uses AddressSanitizer, with its
runtime DLL directory added only to the child environment.

The test compiles exact extracted `lbVector_WorldToScreen`, `GXProject`,
`Camera_ApplyQuake`, and the translation setter, plus Aurora's actual matrix
and vector sources. Camera accessors and stage parameters are controlled leaves.
Both cached and reconstructed viewing matrices exercise perspective and
orthographic projection. Quake checks cover zero and nonzero offsets, including
single-player scaling, using the actual descriptor directly.

An intentional negative control restores the old 3x4 projection allocation and
must report a stack overflow from the 4x4 perspective writer. The corrected
functions must pass all numeric checks with no sanitizer error. This is not a
full camera lifecycle, gameplay, or aggregate savestate test. The receipt pins
inputs, generated sources, executables, and command logs.
