#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/lb/lbanim.h>
static NativeFighterPoseAssets part_pose_assets;
static unsigned part_restore_calls,part_costume_calls;
static Fighter_Part part_restored_start;
static FigaTree* part_restored_clip;
static HSD_Joint* part_restored_costume;
static float part_restored_frame,part_restored_speed;
static unsigned part_retire;
bool nativeFighterRuntime_PoseAssets(Fighter_GObj* g,NativeFighterPoseAssets* out)
{CHECK(g&&out);*out=part_pose_assets;return out->character&&out->costume;}
HSD_Joint* nativeFighterRuntime_PoseGraph(Fighter_GObj* g,int32_t i)
{(void)g;(void)i;return NULL;}
void ftAnim_8006FA58(Fighter* fp,Fighter_Part start,HSD_Joint* graph)
{CHECK(fp&&graph);++part_costume_calls;part_restored_start=start;part_restored_costume=graph;}
void ftAnim_8006EED4(Fighter* fp,Fighter_Part start,FigaTree* clip,float frame,float speed)
{CHECK(fp&&clip);++part_restore_calls;part_restored_start=start;part_restored_clip=clip;
 part_restored_frame=frame;part_restored_speed=speed;
 if(part_retire==1)++fp->x8_spawnNum;
 if(part_retire==2)++capture_package.program.identity;
 if(part_retire==3){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(fp->gobj,fp->x8_spawnNum,&view));view.storage->live=false;}
 if(part_retire==4)fp->x8AC_animSkeleton=NULL;
 if(part_retire==5)fp->x108_costume_joint=NULL;
 if(part_retire==6)fp->x590=NULL;
 if(part_retire==7)((FtCostumeAssets*)part_pose_assets.costume)->import.arena_used=0;
}

static void part_cases(void)
{
 static struct {HSD_AnimJoint nodes[3];HSD_AObjDesc anims[3];HSD_AnimJoint* variants[8];} arena;
 static HSD_JObj joints[4],copies[4];static FighterBone parts[4];
 static FighterPartsTable* tables[Ft_Kind_CustomFirst+1];
 static struct Fighter_804D6540_t* omissions[Ft_Kind_CustomFirst+1];
 memset(&arena,0,sizeof(arena));memset(parts,0,sizeof(parts));
 for(unsigned i=0;i<3;++i){arena.anims[i].end_frame=(float)(12+i);arena.nodes[i].aobjdesc=&arena.anims[i];
  parts[i].joint=&joints[i];parts[i].x4_jobj2=&copies[i];}
 parts[3].joint=&joints[3];parts[3].x4_jobj2=&copies[3];
 arena.nodes[0].child=&arena.nodes[1];arena.nodes[1].next=&arena.nodes[2];arena.variants[2]=&arena.nodes[0];
 NativeSourceImportedAssets* owned=&capture_package.assets;FtCharacterAssets* assets=&owned->character;
 owned->facts.joints=3;owned->facts.omission_count=0;owned->parts.parts_num=3;
 owned->parts.part_to_joint=owned->part_to_joint;owned->parts.joint_to_part=owned->joint_to_part;
 for(unsigned i=0;i<3;++i)owned->part_to_joint[i]=owned->joint_to_part[i]=(u8)i;
 ftPartsTable=tables;tables[Ft_Kind_CustomFirst]=&owned->parts;Fighter_804D6540=omissions;
 omissions[Ft_Kind_CustomFirst]=&owned->omissions;
 owned->omissions.x0=owned->omission_rows;owned->omission_rows[0].x0=1;
 assets->import.arena=(u8*)&arena;assets->import.arena_used=assets->import.arena_size=sizeof(arena);
 assets->part_animation_groups[3]=&assets->part_animation_group_data[3];
 assets->part_animation_group_data[3].x8=arena.variants;assets->data.x1C=assets->part_animation_groups;
 assets->data.x8=&assets->model_data;
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;fp->parts=parts;owner->hsd_obj=&joints[0];
 NativeScriptRuntimeContext c={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=3},{.type=NativeScriptType_I32,.as.i64=2},{.type=NativeScriptType_F32,.as.f32=2}},out;
 NativeScriptDiagnostic d;
 for(unsigned mode=0;mode<4;++mode){
  owned->facts.omission_count=mode==3;owned->parts.parts_num=3+(mode==3);owned->omissions.x4=mode==3;
  args[2].as.f32=mode==0?0:mode==1?2:-2;parts[1].flags_b0=mode==2;
  pose_count=0;ftAnim_ApplyPartAnim(owner,3,2,args[2].as.f32);
  struct Fighter_x8B0_t expected=fp->x8B0[3];unsigned expected_count=pose_count;
  HSD_JObj* expected_joints[8];HSD_AnimJoint* expected_anims[8];
  memcpy(expected_joints,posed,sizeof(posed));memcpy(expected_anims,pose_anims,sizeof(pose_anims));
  memset(&fp->x8B0[3],0,sizeof(fp->x8B0[3]));for(unsigned i=0;i<4;++i)parts[i].flags_b5=false;pose_count=0;
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsApplyPartAnimation,NULL,0,args,3,&out,&d));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(&expected,&fp->x8B0[3],sizeof(expected))&&pose_count==expected_count);
  for(unsigned i=0;i<pose_count;++i)CHECK(posed[i]==expected_joints[i]&&pose_anims[i]==expected_anims[i]);
  CHECK(parts[0].flags_b5&&parts[2].flags_b5&&parts[1].flags_b5==!(parts[1].flags_b0||mode==3));
  if(mode==3)CHECK(parts[3].flags_b5&&pose_count==3&&posed[1]==&copies[2]&&posed[2]==&copies[3]);++cases;
 }
 owned->facts.omission_count=0;owned->parts.parts_num=3;owned->omissions.x4=0;
 parts[1].flags_b0=false;args[2].as.f32=2;
 for(unsigned invalid=0;invalid<12;++invalid){
  HSD_AnimJoint saved=arena.nodes[2];float end=arena.anims[0].end_frame;
  HSD_JObj* joint=parts[2].x4_jobj2;size_t bytes=assets->import.arena_used;
  if(invalid==0)args[0].as.i64=5;if(invalid==1)args[1].as.i64=8;
  if(invalid==2)args[1].as.i64=0;if(invalid==3)args[2].as.f32=NAN;
  if(invalid==4)arena.nodes[2].next=&arena.nodes[0];
  if(invalid==5)arena.nodes[2].next=(HSD_AnimJoint*)(uintptr_t)1;
  if(invalid==6)arena.anims[0].end_frame=INFINITY;
  if(invalid==7)parts[2].x4_jobj2=NULL;
  if(invalid==8)assets->import.arena_used=1;
  if(invalid==9)assets->part_animation_group_data[3].x0=3;
  if(invalid==10)args[2].as.f32=FLT_MIN;
  if(invalid==11)omissions[Ft_Kind_CustomFirst]=NULL;
  pose_count=0;struct Fighter_x8B0_t prior=fp->x8B0[3];out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=999};
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsApplyPartAnimation,NULL,0,args,3,&out,&d));
  CHECK(!pose_count&&!memcmp(&prior,&fp->x8B0[3],sizeof(prior))&&out.type==NativeScriptType_U64&&out.as.u64==999);
  args[0].as.i64=3;args[1].as.i64=2;args[2].as.f32=2;arena.nodes[2]=saved;arena.anims[0].end_frame=end;
  parts[2].x4_jobj2=joint;assets->import.arena_used=bytes;assets->part_animation_group_data[3].x0=0;
  omissions[Ft_Kind_CustomFirst]=&owned->omissions;++cases;
 }
 HSD_GObjFree(owner);
}

static void part_control_cases(void)
{
 static struct {HSD_AnimJoint animation;HSD_AObjDesc descriptor;HSD_AnimJoint* variants[8];} arena;
 static struct {HSD_Joint joints[3];HSD_DatImportRecord records[3];} rig;
 static struct {FigaTree clip;struct Fighter_WaitAnimData action;} bank;
 static s8 clip_nodes[]={0,0,0,-1};
 static HSD_JObj joints[3],copies[3];static FighterBone parts[3];
 static FtCostumeAssets costume;
 NativeSourceImportedAssets* owned=&capture_package.assets;FtCharacterAssets* assets=&owned->character;
 memset(&arena,0,sizeof(arena));memset(&rig,0,sizeof(rig));memset(&bank,0,sizeof(bank));
 memset(joints,0,sizeof(joints));memset(copies,0,sizeof(copies));memset(parts,0,sizeof(parts));
 owned->facts.joints=owned->parts.parts_num=3;owned->facts.omission_count=owned->omissions.x4=0;
 owned->facts.animation_tag=Ft_Kind_CustomFirst;
 arena.animation.aobjdesc=&arena.descriptor;arena.descriptor.end_frame=12;arena.variants[2]=&arena.animation;
 assets->import.arena=(u8*)&arena;assets->import.arena_used=assets->import.arena_size=sizeof(arena);
 assets->part_animation_groups[3]=&assets->part_animation_group_data[3];
 assets->part_animation_group_data[3].x0=1;assets->part_animation_group_data[3].x8=arena.variants;
 assets->data.x1C=assets->part_animation_groups;assets->data.x0=&assets->common_attributes;
 assets->common_attributes.model_scaling=1;assets->model_data.x10=0;
 assets->data.xC=&bank.action;bank.action.x14=&bank.clip;
 bank.clip.frames=30;bank.clip.nodes=clip_nodes;
 rig.joints[0].child=&rig.joints[1];rig.joints[1].child=&rig.joints[2];
 joints[0].child=&joints[1];joints[1].parent=&joints[0];joints[1].child=&joints[2];joints[2].parent=&joints[1];
 copies[0].child=&copies[1];copies[1].parent=&copies[0];copies[1].child=&copies[2];copies[2].parent=&copies[1];
 for(unsigned i=0;i<3;++i){parts[i].joint=&joints[i];parts[i].x4_jobj2=&copies[i];parts[i].flags_b1=true;parts[i].xC=i;
  rig.records[i]=(HSD_DatImportRecord){.type=DAT_JOINT,.object=&rig.joints[i]};}
 costume=(FtCostumeAssets){.joint=&rig.joints[0],.import={.arena=(u8*)&rig,.arena_size=sizeof(rig),.arena_used=sizeof(rig),
  .records=rig.records,.record_count=3,.record_capacity=3}};
 part_pose_assets=(NativeFighterPoseAssets){.character=assets,.costume=&costume,.action_count=1,
  .animation_bytes=(u8*)clip_nodes,.animation_size=sizeof(clip_nodes),.animation_arena=&bank,.animation_arena_size=sizeof(bank)};
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 fp->parts=parts;owner->hsd_obj=&joints[0];fp->x8AC_animSkeleton=&copies[0];fp->x108_costume_joint=costume.joint;
 fp->x24=&bank.action;fp->anim_id=0;fp->x597_bits=Ft_Kind_CustomFirst;fp->x594_bits=0;
 fp->cur_anim_frame=7;fp->frame_speed_mul=1.5f;fp->x8B0[3].x10=fp->x8B0[3].x11=-1;
 NativeScriptRuntimeContext c={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=3},{.type=NativeScriptType_I32,.as.i64=2}},out;
 NativeScriptDiagnostic d;
 // Selection stores intent only; it neither plays a pose nor changes active selection.
 pose_count=0;CHECK(helper(owner,"SelectPart"));
 CHECK(fp->x8B0[3].x10==2&&fp->x8B0[3].x11==-1&&!pose_count);++cases;
 CHECK(helper(owner,"ShowPart"));
 CHECK(fp->x8B0[3].x10==2&&fp->x8B0[3].x11==2&&parts[1].flags_b5&&pose_count==1&&posed[0]==&copies[1]);++cases;
 for(unsigned mode=0;mode<3;++mode){
  fp->x590=mode?&bank.clip:NULL;fp->x8A4_animBlendFrames=mode==2?4:0;fp->x8B0[3].x11=2;parts[1].flags_b5=true;
  part_restore_calls=part_costume_calls=0;ftAnim_80070CC4(owner,3);
  struct Fighter_x8B0_t expected=fp->x8B0[3];unsigned restore=part_restore_calls,rest=part_costume_calls;
  CHECK((mode&&part_restored_clip==&bank.clip&&part_restored_frame==7&&part_restored_speed==1.5f)||(!mode&&part_restored_costume==&rig.joints[1]));
  fp->x8B0[3].x11=2;parts[1].flags_b5=true;part_restore_calls=part_costume_calls=0;
  CHECK(helper(owner,"HidePart"));
  CHECK(!memcmp(&expected,&fp->x8B0[3],sizeof(expected))&&!parts[1].flags_b5&&fp->x8B0[3].x10==2);
  CHECK(part_restored_start==1&&part_restore_calls==restore&&part_costume_calls==rest);++cases;
 }
 // A different registered animation family uses the native remapping path.
 ftPartsTable[Ft_Kind_Purin]=&owned->parts;Fighter_804D6540[Ft_Kind_Purin]=&owned->omissions;
 owned->facts.animation_tag=fp->x597_bits=Ft_Kind_Purin;fp->x590=&bank.clip;fp->x8B0[3].x11=2;
 part_restore_calls=0;CHECK(helper(owner,"HidePart"));CHECK(part_restore_calls==1&&fp->x8B0[3].x11==-1);++cases;
 fp->x8B0[3].x11=2;fp->x8B0[3].x10=2;parts[1].flags_b5=true;args[1].as.i64=-1;pose_count=0;
 CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsSetPartAnimation,NULL,0,args,2,&out,&d));
 CHECK(fp->x8B0[3].x10==-1&&fp->x8B0[3].x11==2&&parts[1].flags_b5);
 CHECK(helper(owner,"ShowPart"));CHECK(!pose_count&&fp->x8B0[3].x11==2);++cases;
 owned->facts.animation_tag=fp->x597_bits=Ft_Kind_CustomFirst;
 // -1 states are healthy native no-ops with no owned animation group required.
 assets->part_animation_groups[3]=NULL;fp->x8B0[3].x10=fp->x8B0[3].x11=-1;pose_count=part_restore_calls=part_costume_calls=0;
 CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsShowPartAnimation,NULL,0,args,1,&out,&d));
 CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsHidePartAnimation,NULL,0,args,1,&out,&d));
 args[1].as.i64=-1;CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsSetPartAnimation,NULL,0,args,2,&out,&d));
 CHECK(!pose_count&&!part_restore_calls&&!part_costume_calls);++cases;
 assets->part_animation_groups[3]=&assets->part_animation_group_data[3];args[1].as.i64=2;
 for(unsigned invalid=0;invalid<7;++invalid){
  fp->x8B0[3].x11=2;parts[1].flags_b5=true;fp->x590=&bank.clip;fp->x8A4_animBlendFrames=0;
  if(invalid==0)bank.clip.nodes=(s8*)(uintptr_t)1;
  if(invalid==1)parts[2].flags_b1=false;
  if(invalid==2)rig.joints[2].next=&rig.joints[0];
  if(invalid==3)fp->cur_anim_frame=NAN;
  if(invalid==4)fp->anim_id=-1;
  if(invalid==5)fp->x597_bits=Ft_Kind_Purin;
  if(invalid==6)parts[2].joint=NULL;
  part_restore_calls=part_costume_calls=0;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=999};
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsHidePartAnimation,NULL,0,args,1,&out,&d));
  CHECK(fp->x8B0[3].x11==2&&parts[1].flags_b5&&!part_restore_calls&&!part_costume_calls&&out.as.u64==999&&out.type==NativeScriptType_U64);
  bank.clip.nodes=clip_nodes;parts[2].flags_b1=true;rig.joints[2].next=NULL;fp->cur_anim_frame=7;
  fp->anim_id=0;fp->x597_bits=Ft_Kind_CustomFirst;parts[2].joint=&joints[2];++cases;
 }
 for(part_retire=1;part_retire<=7;++part_retire){
  uint32_t generation=fp->x8_spawnNum;uint64_t identity=capture_package.program.identity;
  NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owner,generation,&view));
  fp->x8B0[3].x11=2;parts[1].flags_b5=true;part_restore_calls=0;
  out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=999};
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_VisualsHidePartAnimation,NULL,0,args,1,&out,&d));
  CHECK(part_restore_calls==1&&out.as.u64==999&&out.type==NativeScriptType_U64);
  fp->x8_spawnNum=generation;capture_package.program.identity=identity;view.storage->live=true;
  fp->x8AC_animSkeleton=&copies[0];fp->x108_costume_joint=costume.joint;fp->x590=&bank.clip;
  costume.import.arena_used=sizeof(rig);++cases;
 }
 part_retire=0;
 HSD_GObjFree(owner);part_pose_assets=(NativeFighterPoseAssets){0};
}
