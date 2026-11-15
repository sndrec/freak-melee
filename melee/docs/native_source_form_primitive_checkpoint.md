# Direct form primitives and Mario feedback checkpoint (2026-10-08)

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now supplies generic raw-joint effect spawning, joint-plus-Float
effect spawning, copied-position wind and flag/frame/rate-preserving motion
entry. Semantic bone roles retain their mapping; raw indices use the exact owned
joint. Feedback captures its actual source package/program/data/rig/storage and
pool identity and guards callback-sensitive continuation. No native address is
authored and no dummy Effect program selects a stock native effect.

The effect dispatcher classifies native argument contracts independently of
fighter identity. Every 16-bit ID is compared against the actual efSync/efAlt/
efAsync cases. It admits 82 exact-joint signatures and 32 signatures consuming a
joint and an immediate Float pointer. Other signatures reject instead of reading
an absent vararg. The caller supplies a copied Float local, never a script/native
fighter pointer. Mario's previous `effects.spawn(1146, LeftHand)` lacked the
facing argument required by efAlt's actual 0x47A case; the source now uses
`effects.spawn_with_scalar(1146, LeftHand, facing.direction)`.

The canonical Zelda/Sheik sources no longer declare a borrowed `base` layout.
They use their package's owned facts, distinct raw-joint spawning, and explicit
motion entry retaining original flags/frame/rate and animation-start order.
Entity metadata errors now identify missing bindings, attributes, model
descriptors or root joints without changing validation semantics.

Evidence under `A:/FreakMelee-test-archives/`:

- `script-form-feedback-second/receipt.json`: 66,735 checks / 61 cases, all
  65,536 effect IDs, raw 104/43 versus different semantic mappings, copied scalar
  bit/address isolation, position wind and 39 retirement/identity mutations.
  Catalog/guards/part mapping/pool code are real; selected views and effect/wind
  construction leaves are controlled.
- `script-feedback-contracts-final/receipt.json`: 848 existing source-owned
  reflector/wind/part/Item comparisons, corrected one-joint versus scalar ABI.
- `source-form-transition-final/receipt.json`: 83,771 checks, 3,630 direct/VM
  exact-argument pairs and 3,642 VM runs. Native installation/selected publication
  leaves are controlled; actual ownership validator, catalog and VM execute.
- `script-form-primitive-catalog-oct08-scalar/receipt.json`: 56 typed catalog,
  bytecode and dispatch checks with native leaves deliberately unavailable.
- `source-mario-feedback-private-root/package.json`: complete updated Mario
  source builds, verifies and accepts into a private cache.
- `source-mario-feedback-live-root/receipt.json`: actual updated source Mario
  in the hidden/muted client, 1,900 replay input samples, fireball Article kind
  48 present, normal exit and no actor script fault. There is no per-tick script
  hook in this template, so its completed tick-hook counter remains zero; motion
  callbacks and the fireball cue are the exercised source paths. No framebuffer,
  full move set or exact stock parity acceptance is inferred.

The client/tool rebuild and 35 selected interpreter/ownership/content CTests
pass (`.local/source-form-primitives-ctest.log`). Explicit unavailable feedback
leaves keep unrelated console fixtures honest after the direct dispatch change;
the existing jump catalog passes its 39 checks after that dependency correction.

At this checkpoint, complete form preflight remained incomplete: Zelda's Din
Article model descriptor had no imported root joint; Sheik's pose_graph
declaration was not supported by the direct schema. These were recorded in
`source-form-template-feedback-preflight-root/receipt.json`, with original draft
inputs unchanged. Diagnose the actual stock asset/runtime contract before
changing model requirements or adding pose/resource schemas. Complete paired
gameplay, companions and full explicit mutable rollback state/performance remain
open. No user draft or installed content was replaced.

## Model-less Article correction (2026-10-08)

The retail Zelda DAT and actual owned importer both preserve intentionally NULL
model roots for Din's Fire and its explosion. Their model descriptors are
present, with zero dynamic bones and no dynamics or animation graphs. The
shared `Item_802680CC` constructor allocates a runtime root joint for this case;
the particles use that anchor. Script validation now accepts the same descriptor
shape without synthesizing an immutable mesh or changing the importer/native
constructor. Missing common attributes and missing model descriptors still fail.

The actual retail import probe passes 18 checks, and the entity preparation
fixture passes 283 checks / 45 cases, including model-less Article/Workshop Item
acceptance and required descriptor/attribute rejection. Client and content-tool
builds pass. Evidence and native limitations are in
[model-less Article audit](native_source_model_less_article_audit.md).

Fresh complete-source preflight now reaches Zelda line103's unsupported
`callbacks.track_reflect_hit`; Sheik still rejects its `pose_graph` declaration.
`A:/FreakMelee-test-archives/source-form-template-model-less-preflight-root/receipt.json`
records both results with private draft inputs unchanged. This is a validation
correction, not complete Din/paired gameplay acceptance. The next pose batch must
include typed owned selectors and all shared pose operations together; its
[schema audit](native_source_pose_graph_schema_audit.md) records the contracts.

The subsequent coherent pose batch is implemented and documented in the
[direct pose checkpoint](native_source_pose_checkpoint.md): owned metadata,
closed selectors, all eight shared operations and source-only receiver rules.
The full templates still require remaining callback/entity surfaces before
paired gameplay acceptance.
