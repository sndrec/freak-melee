# Exact Rook Item recipe corpus

```powershell
python tests/native/script_rook_items/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-rook-items-new
```

Use a fresh private A: directory. Child compiler/console processes use
`CREATE_NO_WINDOW`, with bounded timeouts. No game, global build, user store or
production source edit is performed. Commands, generated actual bodies and input
hashes are retained. The unchanged Item declaration suffix is extracted from
`examples/rook_workshop/moves/005_NeutralSpecial.fms`; all nine declarations,
blueprint/icon metadata and callback bodies remain actual source.

The actual metadata/compiler prepares nine distinct source owner nodes and
recipe indices. All authored native callback pointers and state-table phases
remain absent; the shared native four-state holdable protocol explicitly chooses
Workshop hooks through the real Item owner/VM/catalog. Native pickup, drop,
throw, hitcaps, gravity/spin/vector operations, scheduler Bool-removal paths,
out-of-bounds handling, GObj lifetime and userdata detach use actual units or
mechanically extracted actual bodies. MuscleMilk's actual `item.spawn_food()`
uses the real binding, source factory and public native Food constructor.

Checks include:

* Held age remains neutral; drop/throw reset it and choose the correct state.
* Every exact throw formula and damage value, default/custom tick gravity,
  SportsDrink's reversal, GummyShark's delayed rise, PHYSICS_OBJECT's sine/cosine
  motion, and FortuneCandy's explicit airborne gravity execute in the VM.
* Every lifetime expires through the actual native animation removal path.
* Default hit/ground/side policies use their shared native behavior. MuscleMilk
  returns false on no/side contact, creates Food and returns true on ground/hit,
  and the actual scheduler removes the source while its Food child survives.
* Missing published Spec makes the shared protocol return SourceRetired without
  changing age or publishing callback output; it never selects authored C policy.

The fixture controls imported Article/model descriptors, common attributes,
graphics/audio/platform leaves and scene contact responses. Its common Food
asset/table is explicitly controlled with an empty native policy, so native Food
gameplay/parity is not accepted. The actual eight Rook icons are borrowed for
immutable metadata preparation; no PNG decoding or draw occurs. Source actors
are instantiated with controlled native userdata; Fighter pull/inventory hooks,
actual blueprint graph import, decoded visuals, full scene position integration
and game physics/rendering are separate factory/parent live acceptance work.
Unrelated Fighter, jump, ledge, presentation and primitive families reject or
abort rather than fabricate successful recipients or authored behavior.

Acceptance: `A:/FreakMelee-test-archives/script-rook-items-final/receipt.json`
passes 997 checks / 65 cases / nine recipes, with 57 actual userdata removals.
No production defect was found in this audit. Initial failures diagnosed the
fixture's wrong native collision entrypoint and a wrong CrimeShard expectation
(the source adds 0.3 to vertical speed); no production semantics were changed.
