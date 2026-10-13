# Pinned program parameters

This bounded console fixture uses actual source parsing, `Compile_Build`, typed
bytecode, Item actors, Execution/VM, storage, GObj fixed pools and Item packed
snapshots. `program.attrs.NAME` reads root immutable parameters from the executing
Article's pinned table. Local `attrs` and `item.params` retain their exact owner.

Two independent banks contain identical names but different root/imported values.
Tests cover root/local type shadowing, Article-local helpers, exact quiet-NaN payloads,
negative zero, high-bit supplied U32 parameters, imported byte/U32, signed I32 and
Bool values, surviving Articles after actual creator GObj retirement, read-only
and mutable cross-owner denial, unresolved values, mismatched table identity/layout/count/
slots, retired storage or physical Item generation, output preservation and
rejected preparation. An unresolved root read through an actual Item hook faults
the actor; packed restore recovers its saved healthy state after the immutable
table is repaired.
Cross-owner root helper calls remain rejected; immutable parameter access does not
grant executable helper ownership.
`attribute uint` remains rejected because the source language requires an explicit
word-width import contract; high-bit words use typed supplied `parameter uint`.
Packed snapshot round trips retain each bank's parameter identity; invalid identity
records reject without mutating the active actor registry.

Run from the repository root:

```powershell
python tests/native/script_program_parameters/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-program-parameters-final
```

The runner requires a fresh private A: directory and launches hidden console
processes only. The fixture compiles with `-Wall -Wextra -Werror`; linked production
and reused boundary units compile with `-Werror`. The SDK's existing duplicated
`MTXPerspective` declarations require `-Wno-array-parameter`. Commands, logs and source/header
fingerprints are saved in the receipt, and changed inputs fail acceptance.

Entity metadata, immutable spec publication and model assets are controlled. The
import provider supplies typed bits at preparation; this fixture does not prove
DAT decoding. Native creator capture/getter use complete source-extracted native
function bodies without semantic rewriting. No
creator receiver is read by the test programs; after the physical creator retires,
the real actor-owned creator query returns absent while parameter reads continue.
There is no full Item constructor, render, physics, game or whole-match snapshot
claim. Packed Item restore assumes native peers and both immutable banks remain
pinned; it does not reconstruct those assets or native objects.


`program.entity_params.ENTITY.NAME` also reads immutable declarations from another
entity in the executing package. The second entity has no actor or published
spec. Two banks supply different values, plus exact NaN/zero imports. Reads
continue after creator retirement and packed restore; mutable state, writes,
unknown names, unresolved parameters and foreign local-owner access still reject.
The updated cohort passes 402 checks / 43 cases / 99 actual VM calls.
Receipt: `A:/FreakMelee-test-archives/program-entity-parameters-oct10-c/receipt.json`.
