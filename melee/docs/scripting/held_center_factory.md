# Held-at-center Article factory

Creation handles preserve all 32 native ID bits in the signed integer result. High-bit IDs remain negative, and ID `0xffffffff` has the same `-1` bit pattern as absence, matching the existing native contract.

`articles.create_held_at_center(ArticleName, previous_x, previous_y, previous_z, raw_part, facing)` returns an integer creation handle, or `-1` when a valid owned rig has no selected bone or the native constructor returns NULL while the caller remains live. The Article name selects one exact declaration at preparation; its physical kind comes from that declaration's pinned owned asset. Multiple declarations may share a physical kind. A declaration named `It_Kind_*` may also be a shared numeric ItemKind constant: ordinary expressions retain that number, while factory calls resolve the declaration by name during preparation. The resulting call semantic, not that numeric argument, selects the owned program.

The supplied coordinates are the native previous position. The current constructor position is the Fighter's position plus half the ECB top/bottom Y sum; Z is retained. Velocity, collision group and initial damage are zero. Native initial collision is enabled, and the actual authored preconstructor sets Air. The bone is a raw native part slot, independent of semantic part mapping.

The constructor captures original creator and revocable attachment before Spawned. The factory initializes actor creation value to zero, runs Created, calls native pickup (including PickedUp), then registers the surviving child in the Fighter's held slot and publishes the handle. It preserves ordinary state, damage-owner and attachment mutations; it does not restore revoked control after callbacks.

Retained caller, source program/storage, rig/raw joint and child physical/model/spec identities must survive synchronous hooks. A later fault or retirement withholds the output and stops source continuation. Completed creation side effects are retained. Constructor NULL cannot distinguish allocation failure from an unpublished child cleaned after Spawned failure/removal.

The isolated factory cohort proves compiler/VM/actor/pool ordering and compares native argument initialization. Its model/asset/native attachment leaves are controlled; this is not full gameplay or rendering acceptance.
