# Edited items during rollback — 2026-10-18

`tools/review_native_script_items.py` checks the received source-only Fox and
Zelda fixtures in a native lobby match. Each package has edited model and
animation data plus a custom held-item recipe, owned image and item blueprint.
The recipe creates a checkerboard billboard item, adds upward speed when thrown
and expires after 180 active ticks. The fixture's neutral special pulls that item.
The earlier [family editing check](native_family_content_checkpoint.md) records
the edits and package hashes.

Two hidden, muted clients use copied profiles and an isolated loopback service.
One client delays outbound input by six frames. The check performs two cycles
of pulling, holding, throwing and waiting for removal, then completes the match.

## Local result

The run passes. Both clients observe item IDs `0` and `2` held by the two
fighters in the first cycle. Both items enter their thrown state and age, then
the item list becomes empty. The second cycle creates new IDs `4` and `6`, throws
them and again reaches an empty list.

The first cycle advances the authority's restore count from 274 to 670 and its
replay-tick count from 2,466 to 5,998. The second advances those counts from
685 to 1,063 and from 6,124 to 9,535. Confirmed checksum comparisons increase
on both clients through both cycles. Item semantic checksums include their
custom recipe, age, collision state and script scratch state.

Both peers reach Results at frame 1,342 with fighter digest `283ddb6d61524890`
and RNG `ee59b0ef`. No checksum mismatch, VM failure or native panic is reported.
The held-item screenshots were inspected and show the packaged billboard images.

The existing `netplay-state` inspection now includes each live item's identity,
state, holder, scripted-program presence, recipe index, age and motion values.
It reads those values only when requested. Gameplay and snapshot data are unchanged.

Receipt: `.local/native-script-item-rollback-3/receipt.json`.
Independent identity/state audit and source/binary hashes: adjacent `audit.json`.
Build: `.local/native-script-items-build-2.log`.

The first attempt completed one cycle but its test required both clients to
restore; one-sided delay only requires the receiving client to do so. The test
now checks increasing peer comparisons and replay work across the pair.
The second attempt missed one two-frame diagnostic throw pulse and left that
item held on both clients. Eight-frame pulses complete the intended inputs.

This closes the current targeted acceptance for edited script-owned object
creation, removal and recreation under corrected inputs. It covers the two
fixture recipes and their normal throw/lifetime paths; other custom behaviors
can receive focused checks when they change.
