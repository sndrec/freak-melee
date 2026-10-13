This grouped hidden fixture connects the real compiler, preparation/publication,
Item/Fighter actors, physical GObj pools, storage and VM to the attachment matrix
reader. It exercises raw part selection for same-source, foreign-source and stock
creators, immutable creator versus mutable attachment/damage owners, healthy
zero fallback, stale relations, output publication and admission failures.

The reader is compared with the unchanged native HSD_MtxGetRotation kernel on
the independently selected raw Fighter part. Known Euler matrices also check
the native math independently. Dirty flags, SRT and matrix payloads remain
unchanged. Unselected observation/native/render families abort or are explicitly
unavailable. Native creator bootstrap and HSD class admission are mechanically
extracted unchanged; actor relation resolution and execution are production TUs.

Class metadata initialization provides a controlled admission mutation boundary.
This does not claim authored callbacks in matrix arithmetic or arbitrary dangling
JObj detection. Graphs and import assets use fixed fixture storage; no game,
renderer, DAT constructor or full attachment gameplay is launched.

Run with `python run.py --compiler <clang.exe> --output <fresh A: directory>`.

Final receipt: `A:/FreakMelee-test-archives/script-attachment-rotation-oct10-final/receipt.json`.
The cohort contains 24,147 checks, 467 cases, 1,150 native comparisons, 946 VM
calls, 34 negative cases and 24 controlled admission mutations. Direct backend
errors preserve argument values; actual VM errors preserve authored output
storage, including logical and physical retirement during class initialization.
