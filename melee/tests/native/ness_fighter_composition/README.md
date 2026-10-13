# Ness fighter composition (unregistered)

This namespace assembles the accepted Fire/Magnet/Flash/Thunder fighter and
article source with smash44 and Bat/YoYo source. `compose.py` reads those immutable
inputs and produces a complete candidate only in the chosen output directory.
The default example and accepted component namespaces remain unchanged. There
is no independent declaration, shipped package, installation or gameplay claim.

Use a fresh output directory and the immutable snapshots:

```powershell
python tests/native/ness_fighter_composition/run.py --snapshot .local/baseline47 --reference .local/baseline45 --output .local/ness-fighter-composition47-review --iso "C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso"
```

The runner verifies SDK/compiler/tool hashes for both snapshots, performs normal
stock asset cloning and parameter import, emits actual generated C, and compiles
it with the supplied SDK and Clang, VC14.44, no FP contraction. It never invokes
snapshot `script-build`, whose compiled include paths could select live headers.
`--prepare-only` stops after import. Receipts contain source, module, parameter,
compiler, reference, ISO and result hashes; no binary outputs belong here.

The SDK47 candidate has 247 named parameters, 45 ordinary state fields, 90
helpers and 11 article logics. Helpers and typed state replace private unions;
immutable rig (51 mappings/63 parts), demo roots and 11 asset bindings remain
copied definition metadata. The generated whole module is compiled before any
fixture; each fixture opens this same module and loads the normal imported FMP.

`ResourceDamage` is the sole damage/death2 policy. Its exact original order is
YoYo removal, Flash detach, Thunder detach/effect reset, then Bat removal. Flash
and Thunder clear dynamic damage callbacks during this chain. Death reset hides
group0 and forgets four handles/effect flag without removing items. Fresh load
explicitly initializes absent native handles to -1; original pointer storage was
zeroed. Attribute reload preserves ordinary state. YoYo installs named pause and
resume callbacks at construction, including allocation failure, rather than
installing them globally at load. Magnet independently supplies effect callbacks.

The accepted `YoyoCheckTimedRehit` explanation and correction remain: clear
victim history then enable through the generic operation which latches motion
cleanup. Stock delayed refresh omitted that bookkeeping, causing lingering
hitboxes. This is an intentional glitch correction, not original-bug parity.
The previously accepted smash44 cleanup/latch comparison remains its evidence.

Composition corrects two earlier constructor-boundary assumptions without
editing accepted sources: smash Bat uses the supplied current position with Z
flattened; YoYo uses its supplied attachment point. Both use owner-origin=false.
A true origin option would replace the position with the owner's ECB midpoint.
The new constructor matrix executes actual generic `nativeArticle_Create` and
actual ECB-origin bodies against original Bat and YoYo allocation inputs. The
positive control demonstrates the wrong true-origin option really differs.
Presentation Bat already uses the same correct false-origin choice.

The bounded SDK47 source comparison covers:

| Matrix | Source pairs | Scope |
| --- | ---: | --- |
| CPU attack/travel/recovery | 2,007 | Queue values/order/counters; empty, one and exact-fit capacity |
| CPU special candidate | 25,920 | Commands, handled result, actual RNG seed/draw count |
| Four-resource lifecycle | 8,464 | Damage/death2/reset, reflection, 0/20/90 nodes, pause/resume flags |
| Aerial jump | 792 | Entry/facing/reverse boundary/worn item; saved drift and adjusted vertical velocity |
| Common KO/ice/sleep presentation | 60 | Actual old45/new47 common bodies, allocation failure, commands, scale and order |
| Constructor allocation input | 36 | Bat/YoYo facing/ECB/Z; actual generated creation helper and generic constructor |
| Common projectile prediction | 32,400 | Entire old45/new47 loops, collision probe vectors/order, geometry and RNG |
| Projectile classification query | 594 | Frames0..32/levels/unblockable; query21+ outside normal Ness prediction horizon |
| Load/reload/item/render notifications | 149 | Actual original bodies, shared primitive order, heavy/held-item cases |

There are 1,135,039 checks with zero unexpected failures. `census.py` pins every
copied Ness stock slot, all36 extended motions and all original Ness fighter and
article source hashes. The compiled ownership fixture checks all142 non-NULL
gameplay phases, all8 special entries, original non-NULL lifecycle hooks and
immutable metadata. Original empty IASA callbacks are executed against generated
empty callbacks. The two originally NULL Fire IASA slots become authored no-op
callbacks because whole-special declarations require all phases; their no-op
behavior is asserted explicitly. The fixture's donor placeholder fails if called
so identical-code folding cannot mistake an authored empty body for inheritance.

CPU tuning is owned binary32 FMP data. At three rounded original double angle
thresholds, 624 candidate decisions differ in the exactly specified endpoint
branches: 192 +192 ground Fire-window endpoints and 240 high Flash endpoints.
Maximum threshold distance is 3.7252902984619141e-08 radians. The neighboring
Floats, unrelated branches and remaining RNG/probe/queue order must match. At
high Flash equality the lower branch may make one original-equivalent random
choice and retain the old command when it fails; this explicit behavioral
judgment is not exact decision parity at those endpoint Floats.

Engine boundaries are controlled allocator/registry/resource-release, renderer,
audio, stage collision and motion dispatch operations. Lifecycle executes actual
original cleanup bodies and generated article signals, but does not reproduce
the full live allocator/destruction dispatcher. Constructor failure inputs run
actual Bat constructor, the exact YoYo prefix through allocation, and full actual
generic constructor; successful YoYo initialization/solvers/entries retain their
separate accepted comparisons. Common jump/presentation comparisons control the
motion engine continuation, not the full fighter scheduler. These new matrices
do not rerun every accepted special/article branch, prove pixels or replace
hidden muted drawn full-fighter acceptance. Kirby-copy behavior is deferred.

SDK45 evidence is retained separately in `.local/ness-fighter-composition45-final`;
it predates the common46 contracts. SDK47 is the coherent whole-source receipt.
