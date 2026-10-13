# Ground owner verification

Links the real fixed Ground pool, occupied-record serializer, HSD GObj/userdata
lifecycle and native scene-owned storage. Creates Ground userdata using the same
allocation/publication sequence as production constructors; graph loading and
stage-specific callbacks are controlled and not part of this console fixture.

Checks zero initialization, immediate/standalone/deferred userdata removal,
generation reuse, exact occupied bytes and allocator ordering after restoration,
malformed/truncated/root-out-of-storage snapshots, unchanged state on rejected
loads, callback-boundary and stale-scene rejection, and full shared GObj capacity.
It checks typed GObj and Ground loads before using the broad .mstate oracle to
restore scheduler roots for continued lifecycle operations. This is not a full
explicit match snapshot or a performance result. Rendering and VM callbacks are
absent at controlled boundaries; unexpected graph primitive calls abort.

Run with `python tests/native/native_ground_pool/run.py --compiler <LLVM bin>
--output <fresh A: directory>`. The runner launches only a hidden console fixture,
pins inputs, and retains logs and the receipt outside the repository.
