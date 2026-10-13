# Item airborne assignment

`item.airborne = Bool` changes only `Item.ground_or_air` to `GA_Air` or `GA_Ground`. This is the direct assignment emitted by `tools/native_fighter_item_script.c`; it does not enter an Item motion, clear owners, or run animation setup. The native Game & Watch Panic picked-up routine also stores `ground_or_air` directly before its separate animation-state selection. This fixture compares the field setter, not that complete routine.

The assignment requires a live source-owned Article, exact scene/generation, Article/kind/author/owner, current program bank, and live matching storage layout/parameters. A removed, faulted, missing, or mismatched source rejects before mutation. Bool remains strict. Workshop `It_Kind_NativeHoldable` rejects assignment: its existing airborne getter reads motion state 2/3, so writing `ground_or_air` would disagree with the getter. No holdable transition is invented.

The runner compiles actual source/index/body/bytecode/VM, callback registry, fixed object pool, storage, catalog, outer property dispatcher, and Item backend. Healthy writes compare the complete Item payload against direct native assignments; source/storage/class/identity failures preserve the payload. The actual `item.airborne` assignment and getter execute through the VM. NativeHoldable and invalid-source VM failures preserve the caller result.

Actor Spec/View/Status publication is controlled. Asset import, the real Item actor/constructor/scheduler, Panic picked-up animation changes, and live gameplay are outside this receipt. Unrelated engine families reject explicitly; the reused Item primitive fixture provides native leaf dependencies but is not claimed as additional tested behavior.

Run in a fresh `A:` directory:

```powershell
python tests/native/script_item_airborne/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/script-item-airborne-final
```

Compilation and execution use hidden processes. The receipt records commands, source hashes, results, and boundaries.
