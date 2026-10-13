# Model-less Article contract

Retail Zelda's DinFire and DinFire explosion intentionally have no imported model graph. They are not missing an indexed model or losing a graph during import. The native Item constructor already creates the runtime JObj anchor used by their position, scale, rotation, hitboxes and attached particle effects.

This is a data/import and source audit, not acceptance of the complete Zelda source template or a game/render test. No production or template changes were made for this audit.

## Actual data and importer evidence

The original v1.02 ISO's `PlZd.dat` is 187,304 bytes, SHA256 `e523bb0fe551f64aa6ae5b3ae59cd1e62434baeded076d4a5e1ea993e122a79a`. `ftDataZelda` is at data offset `0xA120`; its Article table is at `0xA118`. Offsets below are DAT data offsets, not file offsets.

| Owned Article slot | Article | Model descriptor | Raw model words | Imported result | State descriptors |
| --- | --- | --- | --- | --- | --- |
| 0: DinFire | `0xA0C8` | `0xA098` | Four zero words | Root NULL, bone count 0, dynamics NULL | 2, all animation/material/shape graphs NULL, 2 command scripts |
| 1: DinFire explosion | `0xA100` | `0xA0E0` | Four zero words | Root NULL, bone count 0, dynamics NULL | 1, animation/material/shape graphs NULL, 1 command script |

Neither raw root field has a relocation entry. The actual `HSD_DatResolve` returns `HSD_DAT_REF_NULL`, rather than a reference to data offset zero. Both imported Articles retain their common attributes, model descriptors and state arrays. Descriptor attachment ID and flags are also zero in the original raw model words.

The isolated probe links the already-built native importer/library, imports the complete owned Zelda character data through `ftCharacterAssets_Import`, then independently checks the raw reference and imported Article fields. It passed **18 checks**, using 271,304 arena bytes and 1,038 import records. Receipt: [actual importer proof](A:/FreakMelee-test-archives/model-less-article-import-review/receipt.json). Its source and dependency hashes were unchanged during the run. No Ninja/global build, Item constructor or game was run.

As a raw-data control, Sheik's `PlSk.dat` has nonzero, relocated roots in slots 0/1 (`0x109E8`/`0x119C8`), with bone counts 2/0. That control is a raw descriptor observation, not a second native import acceptance.

## Actual native lifetime

| Source/function | Relevant contract |
| --- | --- |
| `itarticle_data.c::itArticle_IsDescriptor`, `import_model`, `itArticle_Import` | A NULL model-root reference is accepted. `import_model` preserves NULL through the graph importer while importing the descriptor's scalar fields. Missing descriptor and NULL descriptor root are different states. |
| `item.c::Item_80267AA8` | A source owner resolves its prepared Article before model construction; `xC8_joint` can legitimately be NULL. It does not need the stock Article bank for that source path. |
| `item.c::Item_802680CC` | For a NULL descriptor root, calls `HSD_JObjAlloc`, sets identity matrix and `scl = NULL`, and attaches that JObj to the GObj. This is the existing native anchor path; no authored dummy descriptor is needed. |
| `Item_8026814C`, `Item_8026849C` | Material traversal finds no DObjs on an empty anchor; scaling still mutates the actual runtime root. |
| `Item_802682F0`, `Item_80268560` | Zero bone count produces no dynamic-bone table. NULL dynamics produces zero dynamic-bone state. These are the exact Zelda shapes proved above. |
| `Item_80268D34`, `Item_80268E5C`, `Item_802694CC` | State animation and commands still execute using the runtime root. Descriptor restoration checks `xC8_joint` before using its children. NULL animation/material/shape descriptors are supported; present command streams remain meaningful. |
| `itdraw.c::it_8026EB18` and normal Item GX/proc installation | Root display traverses no mesh geometry, but the ordinary full Item scheduler/render path remains installed. A NULL imported root is not a reason to skip callbacks, animation, physics, hitcaps, particles or draw side effects. |
| `Item_OnUserDataRemove` and GObj object retirement | Item resource/actor cleanup and the native attached-JObj lifetime remain required. The anchor is mutable native state, distinct from the resident NULL asset pointer. |

`itzeldadinfire.c::it_802C3D74` uses `GET_JOBJ`, selects state 0, sets velocity and spawns effect 1272; its later transition uses 1273. `itzeldadinfireexplode.c::it_802C46C4` uses the same anchor, state 0, a 60-frame lifetime and effect 1274. Their native updates write anchor scale/rotation and hitbox values. `efsync.c` cases `0x4F8..0x4FA` forward to `efLib_CreateGenerator_Attach_Scale`; that helper consumes one JObj and copies its scale to generator AppSRT. Particles provide the visible fire while the Item retains a model-less transform anchor.

## Limits of generic admission

Removing an unconditional `modelDesc->root != NULL` requirement is justified. Continue requiring the Article, common attributes and model descriptor, validating imported references/state animation IDs and respecting the selected resource contract. Do not materialize a fake joint into the immutable assets.

Integration subsequently removed only that unconditional root rejection. The metadata fixture passed 283 checks/45 cases, including NULL-root Articles/Workshop Items and missing-descriptor/common-attribute rejection. The complete native client and content tool rebuilt successfully. The real Zelda source preflight now advances to unsupported `callbacks.track_reflect_hit` at line 103; Sheik remains blocked at line 60 by `pose_graph`. These checks establish the admission correction, not gameplay acceptance.

A NULL root alone does not prove arbitrary bone/dynamics metadata safe. `Item_802682F0` allocates a 100-entry table whenever declared bone count is nonzero, then populates entries by traversing the actual runtime tree; it does not bound that traversal by the declared count. A model-less anchor has one actual node. `Item_80268560` indexes that table directly using dynamics bone IDs. Command hitbox bone 0 uses the runtime root; a nonzero bone requires a table and valid populated index (`itanimlist.c`). Thus nonzero bone count, dynamics/collision references and command bone indices need tree/index validation separately. The proven DinFire descriptors avoid these hazards; this audit did not validate every command byte or claim hostile-DAT safety.

State animation graphs need no blanket rejection merely because the descriptor root is NULL. The runtime root exists, and native animation attachment can target it. `HSD_JObjAddAnimAll` walks the actual JObj tree, consuming matching descriptor nodes; extra descriptor children are not constructed as model joints. Any promise to preserve an authored animation tree therefore requires explicit topology/target validation. For these two actual Articles all three animation graph families are NULL, so that extension is unnecessary for initial admission.

## Source closure still required

The current Zelda template's `item.joint(0, "0").visible = true` corresponds to native `it_80272940`, which clears hidden flags on **root->child**. The empty anchor has no child, and `HSD_JObjClearFlagsAll(NULL, ...)` is a no-op. Do not rewrite that child path to the root: that changes the target. A future direct binding should preserve optional-subtree no-op semantics while still rejecting a stale/retired owner; root rotation uses the distinct empty path.

`it_8026B3A8` only clears `xDC8.flags.x15`, the automatic-lifetime flag. The source already calls `item.use_manual_lifetime()`. This call supplies no evidence of an omitted world-interaction write.

Direct `item.joint`, `item.spawn_root_effect` and `item.inherit_owner_debug_display` are still separate catalog/runtime closure tasks. Admission of valid model-less assets does not make those source operations available. The current strict generic effect ABI classifier excludes forwarded `va_list` cases; the audited `efLib_CreateGenerator_Attach_Scale` signature gives a concrete generic follow-up for effects 1272–1274, with lifecycle guards and no stock-authored callback delegation.

## Prioritised acceptance

1. Keep the actual retail import regression; reject missing descriptors/common attributes and preserve an accepted candidate when preparation fails.
2. Exercise the actual NULL-root constructor path, state commands, scale/rotation, teardown and failed-construction cleanup. Verify one live runtime anchor, no imported mesh/extra bones and no resource leaks.
3. Validate optional-child no-op versus root writes, guarded root particle spawning, exact source/native callback ordering and retirement during creation/update/removal.
4. Compare a bounded hidden game/render run against the stock fire and explosion, including collision, reflection and destruction. Roundtrip the Item/JObj/animation/particle/GObj owner peers together; an Article asset pointer alone is not a complete rollback snapshot.
