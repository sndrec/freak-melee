# Item ground state

Ordinary source-owned Article callbacks may read and assign the Bool property `item.grounded`.

```text
item.grounded = false
```

A true assignment calls the shared native `it_802762B0`: `ground_or_air = GA_Ground`. A false assignment calls `it_802762BC`: `ground_or_air = GA_Air` and clears `xDC8_word.flags.x1F`. Both preserve velocity, position, motion, animation and all other Item fields. These are the exact native helpers used by the reference exporter; Sheik Needle bounce code calls the airborne helper before its separate bounce setup and state change.

The value must be a canonical Bool. The write requires the current physical Item, scene/generation, its attached owned program and live storage, and the selected Article descriptor. A faulted, detached, stale or mismatched actor rejects before changing native state. The pure helpers perform no allocations or authored callbacks. A live Destroyed callback may assign while its retained removal flag and storage remain current; logical retirement afterward follows the normal actor lifecycle.

Workshop holdables expose grounded as `motion == 1`. Their assignment rejects explicitly because changing the native ground/air field would not change that motion projection. Their existing reads remain unchanged. `item.airborne` retains its existing direct ground/air-field assignment contract and its current lifetime restrictions.

The connected `script_item_ground_state` cohort compiles and prepares synthetic owned Article/Workshop assets, publishes actual programs, attaches actual Item actors and executes callbacks through the VM and real GObj pool. It performs 29 complete native Item comparisons in 62 grouped cases (378 checks), including seven actual actor VM calls, and covers strict typing, ownership/storage/generation failures, faulted actors, the live Destroyed window and the holdable projection. Native userdata construction and asset descriptors are controlled; retail DAT construction, collision geometry, rendering and full Sheik behavior are outside scope.
