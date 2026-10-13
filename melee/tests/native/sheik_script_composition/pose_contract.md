# Chain pose operations: source contracts

All references below are frozen SDK59 sources. The policy at
ftseakspecials.c:ftSk_SpecialS_80110610 selects animation305 for ground and308
for air, and the corresponding fighter item graph4/5. It first updates the
smoothed angle/magnitude, then computes frame=0.0556f*angle+4.

When magnitude is nonzero, its exact sequence is:

1. Load the selected common animation into the blend skeleton using
   ftAnim_8006F4C8(fp,true,ftData_80085E50(fp,animation)).
2. Request frame on that skeleton using ftAnim_80070710.
3. Copy costume descriptor pose into blend skeleton using
   ftAnim_8006FB88(fp,raw1,fp->x108_costume_joint->child).
4. Evaluate HSD_JObjAnimAll on the blend skeleton.
5. If magnitude<1, blend the selected imported graph's child pose into the blend
   skeleton using ftAnim_80070108(fp,raw1,1-magnitude,magnitude,graph->child).
6. If phase blend<1, blend live/blend skeleton using
   ftAnim_8006FE9C(fp,raw1,phase,1-phase); otherwise copy blend into live using
   ftAnim_8006FF74(fp,raw1).

When magnitude is zero, no animation loading/request/evaluation occurs. If phase
blend<1, blend the imported graph directly into live pose using
ftAnim_80070010(fp,raw1,phase,1-phase,graph->child). Otherwise copy its pose with
ftAnim_8006FA58(fp,raw1,graph->child).

Each engine operation is already generic. A reusable script surface should
select an owned imported graph by a declared name and choose the destination
live/blend skeleton explicitly. Raw preorder start is bounded by runtime parts;
it is not a semantic bone mapping. Descriptor resolution for nonzero starts
must follow actual ftAnim_8006F994 traversal rather than always selecting child.
Suggested direct Host bindings for the existing operations are:

* load_pose_animation(Fighter*,bool,FigaTree*) -> ftAnim_8006F4C8
* request_pose_frame(HSD_JObj*,float) -> ftAnim_80070710
* copy_blend_pose(Fighter*,Fighter_Part,HSD_Joint*) -> ftAnim_8006FB88
* blend_graph_pose(Fighter*,Fighter_Part,float,float,HSD_Joint*) ->
  ftAnim_80070108 (blend target) / ftAnim_80070010 (live target)
* blend_live_pose(Fighter*,Fighter_Part,float,float) -> ftAnim_8006FE9C
* copy_live_pose(Fighter*,Fighter_Part) -> ftAnim_8006FF74
* copy_graph_pose(Fighter*,Fighter_Part,HSD_Joint*) -> ftAnim_8006FA58

The existing animation_clip Host can supply a selected package animation; HSD
evaluation and blend skeleton pointer selection remain internal compiler/runtime
operations. Imported graphs need owned metadata and validated lookup, not raw
module-visible pointers. These signatures are a source contract proposal only;
root owns actual SDK integration. The script keeps graph/animation choices,
frame arithmetic, smoothing, magnitude and phase branches.
