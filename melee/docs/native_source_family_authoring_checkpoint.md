# Source family authoring checkpoint (2026-10-07)

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Canonical FMSC now carries Single or two-member alternate/companion families.
Every member owns its facts, DATs, source, parameters, CPU profile and resources;
one family identity owns the portrait and stable member keys. The normal family
project reader/creator supports root primary plus the fixed `partner` subtree.
Creating from a package retains both keys and exact payloads; stock authoring
copies both members independently from disc.

`nativeFighterFamilyProject_Build` prepares every member's source/assets at stable
candidate addresses before producing any packet. Per-member `NativeSourcePackage`
still represents exactly one independent Fighter declaration/bank. There is no
need to combine two Fighter declarations or alias their mutable state. A bad
second member leaves the caller's packet/size/ID untouched; candidates do not
publish programs. Preparation and temporary memory reservation occur only at an
explicit authoring transition.

The previous Single-only project builder was removed. Workshop, package CLI and
draft-install identity checking call the family builder directly. Per-member
source checking opens only that member, so both the primary root and secondary
path remain editable independently. Package verification prepares all members
and reports the failing ordinal. This adds no layout change to the approved UI.

Evidence:

- `A:/FreakMelee-test-archives/source-family-project-cli-root/receipt.json`:
  actual native project fixture passes 106 checks with the real private Rook
  assets/resources and a distinct simple partner script, all three family modes,
  identical package/draft byte roundtrips and unchanged inputs. Raw Zelda/Sheik
  and Popo/Nana stock asset construction also passes. CLI packaging, verification
  and per-member source checks pass; malformed second-member source is rejected
  and a failed draft build leaves the cache unchanged.
- The canonical packet fixture passes 5,877 checks / 26 cases. The focused
  content-store and family-packet CTests pass. The native client/tool rebuild.

This remains authoring/transport evidence, not paired gameplay acceptance. The
subsequent [runtime checkpoint](native_source_form_runtime_checkpoint.md) adds
atomic alternate-family preparation/publication and destination source events.
Establish actual actor member identity and complete rendered gameplay before
claiming the paired templates work. Alternate forms still need dynamic accessory
callback identities; companions need typed cross-program
helpers, CPU/history sites and owned rope resources. The related form/companion
contract audits identify those boundaries. No stock-authored callback fallback
or old-format bridge substitutes for them. Complete source paired gameplay and
rollback ownership remain required project work.
