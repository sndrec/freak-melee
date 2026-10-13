# Typed Super Scope common motion storage

The fixture strict-syntax checks complete Start/Fire/Rapid/End units, then compiles
their mechanically extracted control/entry/End bodies with the widened native
Fighter and actual Item charge/ammunition kernels. It compares selected control
bodies against pinned pre-change source `2b3d0253b4a5b5503b6516303dca492646ba9606`.
Reference Start bodies deliberately address their historical PPC state words in
a private Fighter image; AirRapid's reference GObj image has its historical
pointer at byte44. The old top overlay itself shifts its charge field under
pointer widening, so its reference input is seeded at its compiled field offset;
the oracle compares the intended input value rather than repeating that defect.
Production uses actual named fields and native user_data.
That reference-image setup is a test oracle, not a production offset adapter.

Checks compare float bits, release latching, threshold boundaries, selected
charge/cost, rapid budgets and count-to-end, empty variants, motion/order traces,
and unchanged native Fighter bytes outside the first8 motion-state bytes and
the explicit motion/callback/throw fields. The actual Fire accessory callback
reads the selected typed charge level into a controlled presentation leaf.
Motion changes, animation frames,
input and presentation leaves are controlled. No real scene, source Actor/VM,
native animation/collision/render or retirement acceptance is claimed.

The original Item kernels' existing return-type warning is suppressed only in
the extracted fixture. Undefined fallthrough inputs (positive ammo1 with a
requested nonzero charge level in this synthetic cost table) are excluded.

Use `run.py --compiler <MSVC-target clang.exe> --output <fresh A: directory>`.
Processes remain hidden; generated source, logs and hashes stay in that archive.
