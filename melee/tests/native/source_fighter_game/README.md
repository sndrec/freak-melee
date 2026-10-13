# Actual stock/source fighter comparison

Runs a configurable batch of real, rendered native games with identical
synthetic inputs: stock first, then an accepted source-only package in a private
content store. Every launch is hidden and muted. It sends no desktop input and
kills only its own child on the 180-second deadline.

Supply `--exe`, `--iso`, `--source`, `--draft`, `--content-root`, `--package`,
`--cases` and a fresh private `--output` on A:. The runner verifies that the
canonical source is identical to the draft and contained in the packet. It pins
the client, source tree, ISO, packet, copied draft, input configuration and runner.
Receipts are written after each scenario, even if a prior scenario failed.

`ness_ground.json` exercises neutral idle, four grounded specials and all three
smashes against a neutral human Mario on Final Destination. P1 input begins at
sample 100, after normal spawn completion. Each game consumes 360 controller
samples. Motion ranges come from `ftNess/forward.h`; the fixture does not enter
moves by setting state directly.

A pass requires normal exit, input EOF, no reported script fault, live source
actor evidence, complete P1 trace coverage, the requested motion, equal action
timelines and equal emitted Fighter and visible Item fields. Detailed Item digest
differences are also reported, but can include native/source storage differences
and are not a pass gate. The original complete logs remain alongside the receipt.

Inputs-only playback does not verify recorded RNG or state digests. Trace
values can be rounded; equality does not prove every mutable byte, rendered
pixel, restored snapshot or full gameplay scenario. Airborne moves, damage,
interruptions, charged release, CPU decisions and match-end remain separate.

Initial October 10 batches `source-ness-ground-oct10-a` and `-b` correctly fail
all source launches at shared scene preparation, before any actor exists.
Their successful stock runs do not establish source parity. See
[the scene checkpoint](../../../docs/native_source_ness_scene_checkpoint.md).

`source-ness-ground-oct10-c` passes six scenarios but faults both Yo-yo smashes
at the same wrong held-item receiver. After using the native current owner,
`source-ness-ground-oct10-d` passes all eight pairs, including detailed Item
traces. These receipts remain distinct; later changes do not turn an earlier
failed run into acceptance evidence.
