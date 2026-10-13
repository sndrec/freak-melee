# Sheik copied base and independent activation census

This is an unregistered preparation checkpoint. The new
`src/melee/ft/kinds/ftSeak/ftsheik_clone.c/.h` supplies concrete copied metadata;
it is not included in a registry. The existing CMake source glob will compile
the new unit on its next coordinated build; no CMake edits were made. Its raw catalog fallback intentionally
uses `Ft_Kind_Seak`, as other clone bases do. A generated fully authored module
must eventually select `Ft_Kind_None`; the present partial module does not.
No default/example, accepted component, installed package or game was changed.

## Actual owned facts

Fresh USA v1.02 retail decoding verifies 52 semantic mappings, 58 runtime parts,
zero omission records and 58 physical joints in **each** Nr/Re/Bu/Gr/Wh costume.
All five material-animation roots are present. Stock tables give 317 normal
action records, 14 demo records, effect bank 17 and sound bank 27 (the actual
character sound source record, not decimal SFX inference). The base copies the
four demo root strings and animation family `Ft_Kind_Seak` independently of
future gameplay inheritance.

Article slots are NeedleThrow 0, NeedleHeld 1, Vanish 2 and Chain 3. The immutable
recipe is article mask `0x00F`, graph mask `0x030`, typed `Chain` importer, no raw
byte spans or visibility lookup. Two sorted named pose declarations own slots
4/5, child path `"0"`: ChainGroundRest hash 305010971 and ChainAirRest hash
1363947600. The probe recomputes both hashes and verifies the concrete base.
Raw Chain attachment part 26 and pose traversal start 1 remain decisions of
the accepted Chain script, separate from semantic map aliases.

The retail census and source hashes are reproduced by:

```powershell
python tests/native/sheik_clone_activation/audit.py `
  --root C:/gayms/freak-melee/melee --snapshot .local/baseline65 `
  --output A:/FreakMelee-test-archives/sheik-activation65-audit-root
```

No `.local` audit or old cached rig JSON is needed to reproduce the facts.
`census.json` includes every stock table slot, all 24 motion rows, costume hashes,
the typed asset recipe and exact live source hashes. Metadata assertions are
source/import preparation evidence, not costume registration or renderer tests.

## Supported composition and remaining callbacks

`compose.py` reads accepted Needle/Vanish/Chain files without modifying them,
including the accepted Chain cleanup and corrected typed rehit decision.
`lifecycle.fms` adds walljump and attribute-copy load policy, attribute reload,
death needle-count/held/Chain reset, model groups 0=0 and 1=-1, saturated-count
color86, both pickup/drop slots, visibility and knockback presentation. Its
legacy `move SheikLifecycle` label is a callback container in the supported
`base ...` grammar; it does not create an extra gameplay motion.

The partial source owns 80 phase callbacks across motions 341–360: Needle32,
Chain24, Vanish24. All four articles are included. Transform361–364's 16 phases,
two entry callbacks and destination finish are deliberately absent, so the
candidate inherits that stock slice and must not be activated as independent.
The six neutral/side/up entries already compile, as do the new lifecycle
handlers. Whole assembled callback equality, lifecycle comparisons, import and
live package binding remain pending; component source acceptance is not silently
extended to those paths by compilation alone.

Stock NULL slots stay absent: landing, user-data remove, absorb, additional
demo callbacks and model/matrix hooks. OnDeath/OnLoad, color-animation reset,
attribute reload, item pickup/drop including extended counterparts, item
visibility/invisibility and knockback enter/exit are nonnull stock decisions
and are accounted for in the new component. Dynamic damage/death2 composition
remains exactly Needle then Chain. Sheik has no private continuous EF updater;
the shared particle constructors for Needle/Vanish/Transform need no gameplay
delegate. Kirby-copy behavior is outside the user-authorized scope.

Compile the owned base and partial generated DLL with immutable SDK65:

```powershell
python tests/native/sheik_clone_activation/probe.py `
  --root C:/gayms/freak-melee/melee --snapshot .local/baseline65 `
  --output A:/FreakMelee-test-archives/sheik-activation65-probe-root
```

The probe uses frozen `script-c`, Clang and SDK with MSVC14.44. Quoted stock
headers beside the new base must hash-identically to the snapshot before the
syntax check. It performs explicit typed big-endian tuning staging to an owned
FMP, not a falsely claimed normal clone/import. Normal `clone-stock Sheik` cannot
run yet: no Sheik catalog definition/name, article-slot catalog rows or workshop
template mapping currently exists. Root owns those integrations.

## Paired transformation is the actual closure blocker

The exact reusable contracts and original ordering are in
[paired_contract.md](paired_contract.md). The live player mapping supports only
one native kind and sets no companion. Stock creates both forms before play;
the dormant form sleeps, and common transfer activates that persistent entity.
Transform must not reset per-form module state or substitute stock Zelda.

The accepted Zelda slices cover neutral/Din and separate travel policies,
not Zelda's complete transform/CPU/lifecycle closure. Their source receipts do
not make a custom partner currently selectable or independent. An owned Zelda
partner and atomic pair package/launch validation are prerequisites for a faithful
Sheik down-B, not optional cosmetic polish.

Other proven closure boundary: two raw-kind CPU gates consume pending `xF8_b5`
and enqueue shared command38. Existing authored CPU hooks are elsewhere and do
not reproduce that exact priority. Sheik otherwise uses common recovery; the
Zelda-specific Din steering/recovery decisions belong to the partner's later
composition. No ordinary Sheik jump/shield/roll/normal entry special decision was
found beyond stock data and shared callbacks.

Next coherent batch is the generic paired persistent-entity foundation with
actual original common-transfer comparison, then authored Transform and complete
assembled lifecycle/CPU comparison. Default promotion, independence and hidden
muted drawn acceptance must wait for that closure.
