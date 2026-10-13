# Interpreter bone, item-state and family checkpoint

This batch strengthens existing bone and article-state operations and adds
generic family queries. It does not activate simultaneous companions or claim
complete fighter acceptance. The existing decompilation remains the native
implementation; no stock special handler or generated-module host is used as
an interpreter fallback.

## Canonical source and native behavior

The preceding stage/effect checkpoint misidentified two preparation diagnostics.
Link and Young Link stopped at obsolete `cue.consume(0)`, before the existing
bone query. They now call `cue.article.consume()`; their old bit-three spelling
also becomes `cue.reverse.consume()`. The actual setup binding consumes exactly
native throw bits zero and three respectively. Sheik's article callback used
root `attrs.needle_held_part`; it now explicitly reads
`program.attrs.needle_held_part`. No aliases or additional resolvers were added.

[Bone queries](scripting/bone_queries.md) retain the physical fighter, owned
program/storage and exact selected rig before sampling the native joint matrix.
They retain raw joint identity and publish all three position outputs only
after the native operation completes with the same source. Semantic-part lookup
still uses the native part-to-joint mapping. Integral index admission is explicit
at both the compiler and runtime boundary.

[Article state transitions](scripting/article_state_transitions.md) retain the
actual Item, source program, immutable assets and root JObj across
`Item_80268E5C`. The real native body and its revision guard preserve a newer
nested state transition. A normal owner/state change remains valid; retirement,
fault or replacement of owned identity stops continuation. The root JObj is
required by the native transition even when an article has no visible geometry.

[Family queries](scripting/family_queries.md) call the actual current member
lookup and expose the shared Sleep retirement bit. Wide integral indices are
not narrowed into valid members, and all 32 spawn-ID bits are preserved.
The current member lookup resolves only the caller in entity-zero role. It
does not implement immutable member indices across an alternate-form swap;
simultaneous-companion activation also remains rejected. Configuring the Sleep
policy neither creates a companion nor proves paired gameplay.

## Build and preparation

The native client, content tool, source-assets fixture and selected integration
targets build. All 12 selected CTests pass: heap, object allocation, match pool,
DAT, combat, Item, Item actor/scheduler/program, resource metadata, move entry
and article resource compiler. Unrelated engine-free consoles explicitly reject
the unavailable family operations. No game window, GPU or audio was launched.
The older presentation CMake target also builds and passes its CTest after its
world-query fixture was migrated to actual physical pool references and native
JObj class ancestry. Its unprepared source-package scenario rejects honestly;
the new actor fixture supplies the positive owned-source evidence.

The rebuilt tool checks ten retained sources against private copied drafts.
Mario, Dr. Mario, Peach, Game & Watch and Zelda prepare. Exact subsequent
diagnostics are Link 383/Young Link 387 (`articles.create_held_at_center`),
Sheik 1217 (write to read-only `item.grounded`), and Popo 467/Nana 501
(`targets.read_bone_position`). The source bodies remain intact.

Receipt: `A:/FreakMelee-test-archives/source-family-bone-census-oct10-final/receipt.json`;
SHA256 `dc6a53817fa65b14bd5c5781c50ae50521179fc7d0a3841734767b4d734ae386`.
Root verified the rebuilt tool and all ten source hashes. Preparation is not
whole-fighter behavioral acceptance.

## Connected verification

Bone queries pass 1,364 checks / 80 cases / 54 bit-exact native comparisons,
11 continuation mutations and 65 actual actor/VM calls. Actual source compiler,
program preparation, Fighter actors, storage, catalog routing and physical pools
execute the queries. Unchanged HSD/Dolphin matrix kernels sample controlled
root/parent graphs. Rig metadata and graph construction are controlled, as are
class/RObj continuation injections. Arbitrary native callbacks, IK, imported
retail rigs and full Link behavior are outside this proof. Native matrix code
can lazily allocate scale storage; the fixture preconstructs that storage and
does not prove every imported rig is allocation-free.

Receipt: `A:/FreakMelee-test-archives/script-bone-queries-oct10-final/receipt.json`;
SHA256 `f7747fc830c68f73f64ab95afc39e62fb4c7a08b99c0d4a2ca049fcfaa8220a2`.
Root independently verified all 1,706 inputs. The older CMake presentation
regression passes; its private direct runner has historical omitted engine-family
dependencies and is not claimed as passing.

Article transitions pass 7,278 checks / 24 cases / 513 native comparisons,
including all 512 defined flag combinations, nested transitions, actual VM
calls and actor fault/removal/reuse. Actual compiler, VM, actor, storage,
receiver and match-pool implementations are linked. Selected package/assets
and graphics/audio/animation/geometry leaves are controlled; no DAT/model load
or complete Sheik behavior is claimed.

Receipt: `A:/FreakMelee-test-archives/script-article-state-transitions-oct09-final/receipt.json`;
SHA256 `386cf41708fbe338fcf5be67a774a00bc9cf535d47383ca13537f80d84b6523c`.
Root independently verified all 1,675 inputs.

Family queries pass 303 checks / 90 cases / 48 native comparisons and 23 VM
calls. The real compiler, program preparation, Fighter actor, storage and GObj
pool execute the calls; the unchanged native member-query body is extracted
mechanically. Package publication and Player inputs are controlled. Tests cover
all 32 handle bits, wide indices, absent members, neighboring policy bits,
logical/physical invalidation and context rejection. This proves the current
self/entity-zero query, not alternate member mapping, companion history or
actual Sleep entry.

Receipt: `A:/FreakMelee-test-archives/script-family-queries-oct10-final/receipt.json`.
SHA256 `feb5d49343357c55405cc0201dc50ace609f82df1fc9d4287915af2b78373e6c`.
Root independently verified all 1,696 inputs.

## Remaining scope

Subsequent shared bindings, actual paired gameplay/member mapping and complete
fighter behavioral acceptance remain required. Public resource-animation
application and compact whole-match save/load are unfinished; Regions/Pages
still owns broad snapshots. The full project goal remains active.

The next read-only audits identified an existing held-at-center Item factory
that needs its public prepared-Article route and stronger callback retention.
An Article declaration must select its owner directly; a runtime physical-kind
lookup is ambiguous when multiple declarations share a kind. Recipient bone
queries must use raw imported indices, stage outputs together and retain both
caller and target; native matrix callbacks and lazy allocation remain explicit
eligibility boundaries. These audits are not implementation acceptance.
