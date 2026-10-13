# Full Parasol producer: unregistered candidate

Source-only comparison passes against frozen SDK41: **3,840 actual/generated
pairs, six separate safety cases and 96,384 checks**. No default Peach behavior
or complete fighter activation is changed.

The candidate translates the entire `ftPe_SpecialHi_8011D424` producer. It consumes
the Capsule request before querying raw part109, samples its world position before
saved-item hide/pause/drop, creates the article at that position, attaches it with
`items.attach(handle, raw_part, true)`, and only then publishes Parasol/held slots.
The actual drop body clears the previous held slot before construction/pickup;
the pickup animation-command sink checks that the slot is still empty. A drop
callback changes the root position and facing in half the cases, checking that
position was sampled before drop while facing is read afterward.
It clears accessory4 and installs authored pre/post-hitlag callbacks even when no
construction is requested. The accepted cleanup fixture is unchanged.

The original `it_802BDA64` constructor performs `Item_InitSpawn`, airborne B18,
and `Item_AttachToParent`. The inline performs AB54 and then `db_80225DD8` without
auxiliary binding. AB54 itself reaches another debug notification through
`it_802742F4`, before attachment/pickup. Both notifications must be retained;
`notify_holder=true` adds the later notification without binding another slot.

`fixture.py` extracts 54 complete unchanged bodies, including the full producer,
constructor, Spawn/Attach inlines, B18/CreateAuthored entrypoints, AB54,
`it_802742F4`, debug body, authored constructor, pose reset, pickup and animation
helpers. Real native item owner, live registry and export dispatch units are
linked. The original and generated paths use distinct low-level behavior
selectors; the allocation sink assigns the corresponding stock/generated pickup
table. A fresh allocated object is separate from every still-live previous item.

Controlled boundaries are private `Item_8026862C` allocation/initial instance
construction, HSD animation and item commands, attachment matrix update, sounds,
model/animation state transition internals, pure engine lookups and statistics.
The fixture checks every Spawn field initialized by these constructor bodies;
unused uninitialized Spawn storage is not passed to a full allocator or compared.
No controlled
dependency supplies the omitted debug notification or delegates a script to a
stock Peach handler. The authored pickup callback reproduces state2/rate16/
accessory, model+command advancement, then rate1. Full asset/model construction
and subsequent Parasol travel phases remain outside this fixture.

The healthy matrix crosses three request kinds, existing/missing resource,
five previous held references, allocation success/failure, four full-width IDs,
both facings, four holder collision-display modes and two drop-pose mutation
modes. It compares ordered events,
saved/resource/held callbacks, initialized Spawn position/history/velocity, item
state/rate/frame and hold kind, owner/attachment/debug fields, pose/scale and auxiliary slot
preservation. Missing asset/rig and callback removal/recycling probes are separate
safety tests where the original lacks a valid-pointer contract.

For an invalid attachment, the candidate explicitly discards the new unattached
article and restores a live saved item. A recycled foreign object is not removed
through the old handle. Source removal/recycling stops the generated continuation.
These are authored safety decisions outside the healthy fixed-retail-rig parity
claim; allocation failure retains the original empty-held/saved-reference result.

The owned attachment parameter is imported using the real parameter importer and
retail Peach DAT. The fixture has one motion slot only to host the authored
hitlag callbacks; it does not claim coverage of Parasol travel or full Peach
specials/lifecycle. Callback slot presence and independently tested cleanup
semantics are compared, not equality of native and generated callback addresses.

Reproduce with frozen runtime/tool/compiler/headers, without launching a game:

```powershell
python tests/native/peach_parasol_creation/run.py --root . --baseline .local/baseline41 --output .local/peach-parasol-creation41
```

`result.json` records commands, the 54 unchanged complete production bodies and
SHA-256 hashes of all frozen SDK files, compiler/tool, candidate/importer,
generated source, parameter data, executable and bounded logs. `source.json`
records the extracted function inventory. Earlier harness diagnosis incorrectly
expected the prior held slot during pickup; the real drop body's clearing write
was retained, and only that test expectation was corrected.
