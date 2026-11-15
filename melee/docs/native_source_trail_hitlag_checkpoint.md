# Interpreted output records, trail descriptions and linked hitlag

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch connects three generic contracts required by the existing authored
fighter translations. It introduces no stock-handler delegate or fighter-specific
binding. Ordinary packages continue to run interpreted bytecode.

## Runtime changes

Mutable event output records have a descriptor-owned scalar schema, with at most
16 fields and 128 native bytes. The schema is validated once when entering an
event. Reads and writes retain the current payload frame and source identity;
helpers cannot implicitly access a caller's output record. Native bytes are
staged through Execution and Fighter dispatch and published only after successful
completion and retained actor/program/storage checks. Caller padding is preserved.
All callers use the updated signatures directly, without compatibility wrappers.

The first record consumer is `on trail`. Its fourteen fields feed the actual
afterimage reader, which retains the source package, owned raw rig slot and native
JObj class before publishing a usable anchor. It rejects invalid descriptors,
unavailable rigs and failed script actors without a stock fallback. The held
Beam Sword and ordinary stock paths remain separate. See [weapon ribbons](scripting/trail.md).

`hitbox.clear_all()` calls `ftColl_8007AFF8`: disable all four capsules and clear
the Fighter transition-cleanup marker. Victim history, counters, damage and
coordinates remain untouched. This differs from individually disabling capsules,
which preserves the marker. See [Fighter hitboxes](scripting/fighter_hitboxes.md).

`hitlag.active`, `hitlag.linked_id`, `hitlag.link(handle)`, `hitlag.unlink()` and
`hitlag.request_release()` expose the shared native relation and release request.
Peer IDs retain their native 32-bit bits; stock and interpreted peers are admitted
through their actual live identities. Linking is one-sided and does not recursively
dispatch hitlag callbacks. The existing scheduler owns propagation. Relations are
native Fighter snapshot state, separate from interpreted variables. See [linked hitlag](scripting/hitlag.md).

These tick/callback paths allocate no memory. Schema records and publication
scratch use fixed bounded storage; program preparation remains a loading transition.

## Verification

The actual client, content tool, source-assets and selected test-target build
passes. Eleven allocator, compiler, Item, metadata and move-entry CTests pass.
No game, GPU or audio process was launched for this batch.

The generic event-record cohort passes 197 checks / 16 grouped cases through the
actual frontend, statement compiler, bytecode VM and payload kernel. It covers
schema bounds, exact field widths, source scope, read-only callback restrictions,
payload lifetime and transactional export. Root verified all 1,580 pinned inputs.
Receipt: `A:/FreakMelee-test-archives/script-event-record-oct09-final/receipt.json`,
SHA256 `30eddfa0d8c33dbabd34492fa08d21631dd52bdaba0890e0eaa8de703aa05696`.

The connected trail cohort passes 255 checks / 57 cases with six complete
native-record comparisons and 22 explicit Fighter/Execution calls. It extracts
the unchanged Link and Young Link trail bodies, supplies owned named parameters
from controlled native sword attributes and compares all fourteen fields plus
the raw joint anchor with the actual afterimage reader. It exercises callback
failure/retirement, malformed record IO, invalid descriptors, missing hooks and
rig/class admission. Source classification is checked at the reader boundary;
the VM's lifetime provider has its own narrower identity contract. Initialized
class metadata, selected package/data and unrelated native operations are
controlled. No graphics, full Link preparation, Beam Sword path or whole-fighter
behavioral acceptance is claimed.
Root verified all 1,687 pinned inputs. Receipt:
`A:/FreakMelee-test-archives/script-fighter-trail-record-oct09-final2/receipt.json`,
SHA256 `be195784d6a74f818ae88e99474bec11c463a332f61e99d53736b77add29129d`.

The connected hitbox cohort passes 357 checks / 151 cases, including 62 native
comparisons and 64 VM calls. Twenty new comparisons cover whole-capsule cleanup
across capsule states. The unchanged state/damage regression passes 11,344 checks
/ 3,250 cases with 3,207 native comparisons. Root verified all 1,698 pinned inputs.
Receipt: `A:/FreakMelee-test-archives/script-fighter-hitbox-clear-all-oct10-final2/receipt.json`,
SHA256 `1b52095aa2fe957e8b6f6e95baf04b26ad711601dbce7480991dd6790670bf03`.

The connected linked-hitlag cohort passes 142 checks / 73 cases with 30 exact
native comparisons and 51 VM calls. It exercises the actual interpreter, Fighter
actor, physical pool and binding, including stock/interpreted peers, high-bit IDs,
retirement/reuse, malformed inputs and explicit actor plus native-state restore.
The selected package/data and unrelated native leaves are controlled. Root verified
all 1,686 inputs. It does not claim recursive hitlag scheduler or whole-match history
acceptance. Receipt: `A:/FreakMelee-test-archives/script-linked-hitlag-oct10-review1/receipt.json`,
SHA256 `08e73a3e2971a12ef2ef1d1c5d723923371ec700b1dba0f1720464a75e0a353e`.

## Source preparation and remaining work

The rebuilt content tool checks the same ten unchanged translation sources against
private copied drafts. Mario, Dr. Mario, Peach, Game & Watch and Zelda still prepare.
Link/Young Link now reach `effects.spawn_between`; Sheik reaches `stage.sweep_box`;
Popo/Nana reach `callbacks.clear_hitlag`. Their unsupported bodies were retained.
Receipt: `A:/FreakMelee-test-archives/source-trail-hitlag-census-oct10-final/receipt.json`.
Preparation alone does not establish gameplay or complete fighter support.

The Popo/Nana cleanup spelling should migrate directly to the existing
`callbacks.pre_hitlag(None)` and `callbacks.post_hitlag(None)` operations. There
is no need for a compatibility alias. The subsequent `effects.pause_during_hitlag`
already has a catalog/backend path; this census stopped before it and establishes
neither its rejection nor its execution.

Remaining boundaries include the subsequent missing generic bindings, public
resource-animation application, whole-fighter behavioral acceptance and compact
whole-match save/load. Regions/Pages remains the current broad snapshot owner.
The full project goal stays active.
