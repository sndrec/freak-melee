#undef NDEBUG
#include <melee/ft/native_fighter_rig_references.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_hitbox_lifetime.h>
#include <melee/ft/ftaction_data.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/lb/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long checks;
static unsigned cases,commands;
static char error[256];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error);exit(1);}}while(0)
typedef struct Bytes {uint8_t* data;size_t size;} Bytes;
static Bytes read(const char* path)
{
 FILE* f=fopen(path,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>0&&!fseek(f,0,SEEK_SET));
 Bytes b={malloc((size_t)n),(size_t)n};CHECK(b.data&&fread(b.data,1,b.size,f)==b.size&&!fclose(f));return b;
}
static unsigned mapped(int value,const uint16_t* map)
{return (unsigned)value<MAX_FT_PARTS?map[value]:(unsigned)value;}
static void import(const NativeFighterProject* p,FtCharacterAssets* result,void* arena)
{CHECK(ftCharacterAssets_Import(p->fighter_dat,p->fighter_size,&p->facts.layout,p->fighter_root,p->action_count,p->demo_count,1,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,result));}
static unsigned compare(const NativeFighterProject* p,const uint16_t* map,const NativeFighterRigReferences* out,bool owned_flags)
{
 unsigned promoted=0;
 FtCharacterAssets* a=calloc(1,sizeof(*a)),*b=calloc(1,sizeof(*b));
 uint8_t* arena=calloc(2,FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(a&&b&&arena);
 import(p,a,arena);NativeFighterProject changed=*p;changed.fighter_dat=out->fighter;changed.fighter_size=out->fighter_size;
 import(&changed,b,arena+FT_CHARACTER_IMPORT_ARENA_SIZE);
 CHECK((unsigned)b->common_attributes.camera_zoom_target_bone==mapped(a->common_attributes.camera_zoom_target_bone,map));
 ftCo_DatAttrs attributes=b->common_attributes;attributes.camera_zoom_target_bone=a->common_attributes.camera_zoom_target_bone;
 CHECK(!memcmp(&attributes,&a->common_attributes,sizeof(attributes)));
 CHECK(b->model_data.x10==mapped(a->model_data.x10,map)&&b->model_data.x11==mapped(a->model_data.x11,map)&&
       b->model_data.x12==mapped(a->model_data.x12,map)&&b->model_data.x13==mapped(a->model_data.x13,map)&&b->model_data.x14==mapped(a->model_data.x14,map));
 CHECK((unsigned)b->scale.x0==mapped(a->scale.x0,map)&&b->scale.scale==a->scale.scale);
 CHECK(b->hurtboxes.count==a->hurtboxes.count);
 for(int n=0;n<a->hurtboxes.count;++n){ftHurtboxInit actual=b->hurtboxes.inits[n],expected=a->hurtboxes.inits[n];
  CHECK((unsigned)actual.bone_idx==mapped(expected.bone_idx,map));actual.bone_idx=expected.bone_idx;CHECK(!memcmp(&actual,&expected,sizeof(actual)));}
 CHECK(a->dynamics.descriptor_count==b->dynamics.descriptor_count&&a->dynamics.x4==b->dynamics.x4);
 for(unsigned n=0;n<a->dynamics.descriptor_count;++n){
  const BoneDynamicsDesc* x=&a->dynamic_bones.array[n];const BoneDynamicsDesc* y=&b->dynamic_bones.array[n];
  CHECK((unsigned)y->bone_id==mapped(x->bone_id,map)&&x->dyn_desc.count==y->dyn_desc.count);
  CHECK(!memcmp(&x->dyn_desc.pos,&y->dyn_desc.pos,sizeof(Vec3)));}
 for(int n=0;n<a->dynamics.x4;++n){struct ftData_x38 actual=b->dynamics.x8[n],expected=a->dynamics.x8[n];
  CHECK((unsigned)actual.x0==mapped(expected.x0,map));actual.x0=expected.x0;CHECK(!memcmp(&actual,&expected,sizeof(actual)));}
 for(unsigned n=0;n<2;++n){struct ftData_x38 actual=b->collision_bubbles[n],expected=a->collision_bubbles[n];
  CHECK((unsigned)actual.x0==mapped(expected.x0,map));actual.x0=expected.x0;CHECK(!memcmp(&actual,&expected,sizeof(actual)));}
 const int16_t* before=&a->collision.unk0;const int16_t* after=&b->collision.unk0;
 for(unsigned n=0;n<6;++n)CHECK((unsigned)after[n]==mapped(before[n],map));
 CHECK(a->collision.unkC==b->collision.unkC&&a->collision.ledge_snap_height==b->collision.ledge_snap_height&&a->collision.ledge_snap_x==b->collision.ledge_snap_x&&a->collision.ledge_snap_y==b->collision.ledge_snap_y);
 for(unsigned n=0;n<5;++n)CHECK((unsigned)b->x54[n]==mapped(a->x54[n],map));
 if(a->data.x58){struct ftData_x58_t actual=b->inverse_kinematics,expected=a->inverse_kinematics;
  CHECK(actual.x0==mapped(expected.x0,map)&&actual.x1==mapped(expected.x1,map)&&actual.x8==mapped(expected.x8,map)&&actual.x9==mapped(expected.x9,map)&&actual.x10==mapped(expected.x10,map)&&actual.x11==mapped(expected.x11,map));
  actual.x0=expected.x0;actual.x1=expected.x1;actual.x8=expected.x8;actual.x9=expected.x9;actual.x10=expected.x10;actual.x11=expected.x11;CHECK(!memcmp(&actual,&expected,sizeof(actual)));}
 for(unsigned n=0;n<5;++n)if(a->part_animation_groups[n]){
  const struct ftData_x1C* x=a->part_animation_groups[n];const struct ftData_x1C* y=b->part_animation_groups[n];
  CHECK(y&&y->x0==mapped(x->x0,map)&&x->x2==y->x2);
  for(unsigned i=0;i<x->x2;++i)CHECK(y->x4[i]==mapped(x->x4[i],map));
 }
 uint8_t* visited=calloc(p->fighter_size/4+1,1);CHECK(visited);
 NativeFighterActionCommand* rows=NULL;size_t row_capacity=0;
 for(unsigned action=0;action<p->action_count;++action){
  size_t count;CHECK(nativeFighterAction_Commands(p,action,NULL,0,&count));
  if(count>row_capacity){rows=realloc(rows,count*sizeof(*rows));CHECK(rows);row_capacity=count;}
  if(count)CHECK(nativeFighterAction_Commands(p,action,rows,row_capacity,&count));
  for(size_t n=0;n<count;++n){
   const NativeFighterActionCommand* row=&rows[n];unsigned index=row->source_offset/4;
   CHECK(index<p->fighter_size/4+1);if(visited[index])continue;visited[index]=1;
   const union CmdUnion* x=a->action_stats.commands+(row->source_offset-a->action_stats.source_begin)/4;
   const union CmdUnion* y=b->action_stats.commands+(row->source_offset-b->action_stats.source_begin)/4;
   switch(row->opcode){
   case 10:CHECK(y->spawn_gfx_0.boneId==(!x->spawn_gfx_0.useCommonBoneIDs&&!x->spawn_gfx_0.useUnkBone?mapped(x->spawn_gfx_0.boneId,map):x->spawn_gfx_0.boneId));++commands;break;
   case 11:case NATIVE_HITBOX_TIMED_OPCODE:case NATIVE_HITBOX_EXPLICIT_OPCODE:
    CHECK(y->create_hitbox_0.bone==(x->create_hitbox_0.use_common_bone_ids?x->create_hitbox_0.bone:mapped(x->create_hitbox_0.bone,map)));
    if(row->opcode==11){CHECK(y->create_hitbox_0.opcode==(owned_flags?NATIVE_HITBOX_EXPLICIT_OPCODE:11));promoted+=owned_flags;}
    if(row->opcode==11&&y->create_hitbox_0.opcode==NATIVE_HITBOX_EXPLICIT_OPCODE)
     CHECK(y->create_hitbox_0.only_hit_grabbed==ftActionData_NextHitboxFlag(x+5));
    else CHECK(y->create_hitbox_0.opcode==x->create_hitbox_0.opcode);++commands;break;
   case 28:CHECK(y->set_hurt_state.bone_idx==mapped(x->set_hurt_state.bone_idx,map));++commands;break;
   case 58:CHECK(y->wind_fx_0.bone==mapped(x->wind_fx_0.bone,map));++commands;break;
   default:break;
   }
   if(row->opcode==11&&y->create_hitbox_0.opcode==11)CHECK(ftActionData_NextHitboxFlag(x+5)==ftActionData_NextHitboxFlag(y+5));
   // Restore the bone field in the actual imported native command union.
   // Every other field, operand and relative call address must remain exact.
   union CmdUnion copy[8];CHECK(row->words<=8);memcpy(copy,y,row->words*sizeof(*copy));
   switch(row->opcode){case 10:copy[0].spawn_gfx_0.boneId=x->spawn_gfx_0.boneId;break;
    case 11:case NATIVE_HITBOX_TIMED_OPCODE:case NATIVE_HITBOX_EXPLICIT_OPCODE:copy[0].create_hitbox_0.bone=x->create_hitbox_0.bone;break;
    case 28:copy[0].set_hurt_state.bone_idx=x->set_hurt_state.bone_idx;break;
    case 58:copy[0].wind_fx_0.bone=x->wind_fx_0.bone;break;default:break;}
   if(row->opcode==11&&y->create_hitbox_0.opcode==NATIVE_HITBOX_EXPLICIT_OPCODE){
    copy[0].create_hitbox_0.opcode=11;copy[0].create_hitbox_0.only_hit_grabbed=x->create_hitbox_0.only_hit_grabbed;
   }
   CHECK(!memcmp(copy,x,row->words*sizeof(*copy)));
  }
 }
 free(rows);free(visited);free(arena);free(a);free(b);return promoted;
}
int main(int argc,char** argv)
{
 CHECK(argc==3);Bytes facts=read(argv[1]),bytes=read(argv[2]);NativeFighterProject project={0};NativeScriptDiagnostic diagnostic={0};
 CHECK(nativeSourceAssets_Read(facts.data,facts.size,&project.facts,&diagnostic));memcpy(project.fighter_root,project.facts.fighter_root,sizeof(project.fighter_root));
 project.action_count=project.facts.actions;project.demo_count=project.facts.demo_count;project.fighter_dat=bytes.data;project.fighter_size=bytes.size;
 uint16_t map[MAX_FT_PARTS],inverse[MAX_FT_PARTS];
 // Four slots preserve the incidental retail next-command bit while changing
 // every real bone number. The one-slot case below exercises that dependency.
 for(unsigned n=0;n<MAX_FT_PARTS;++n){map[n]=(uint16_t)((n+4)%MAX_FT_PARTS);inverse[map[n]]=(uint16_t)n;}
 NativeFighterRigReferences out={0};CHECK(nativeFighterRig_RemapReferences(&project,map,&out,error,sizeof(error)));
 compare(&project,map,&out,false);++cases;
 NativeFighterProject changed=project;changed.fighter_dat=out.fighter;changed.fighter_size=out.fighter_size;
 NativeFighterRigReferences restored={0};CHECK(nativeFighterRig_RemapReferences(&changed,inverse,&restored,error,sizeof(error)));
 CHECK(restored.fighter_size==bytes.size&&!memcmp(restored.fighter,bytes.data,bytes.size));++cases;
 free(out.fighter);free(restored.fighter);out=(NativeFighterRigReferences){0};
 // A one-slot move may alter a GFX/hurt-state field read by the preceding
 // retail hitbox. Refuse it atomically rather than changing hit eligibility.
 unsigned affected=0;
 for(unsigned a=0;a<project.action_count;++a){
  size_t count;CHECK(nativeFighterAction_Commands(&project,a,NULL,0,&count));
  NativeFighterActionCommand* rows=count?malloc(count*sizeof(*rows)):NULL;CHECK(!count||rows);
  if(count)CHECK(nativeFighterAction_Commands(&project,a,rows,count,&count));
  for(size_t n=0;n<count;++n)if(rows[n].opcode==11){
   u32 following;HSD_Dat dat;CHECK(HSD_DatOpen(&dat,bytes.data,bytes.size)==HSD_DAT_OK);
   CHECK(HSD_DatReadU32(&dat,rows[n].source_offset+20,&following));
   unsigned op=following>>26,bone=(following>>18)&255;
   if((op==28||(op==10&&!(following&((1u<<17)|(1u<<15)))))&&bone<MAX_FT_PARTS&&
      ((bone^((bone+1)%MAX_FT_PARTS))&2))++affected;
  }
  free(rows);
 }
 for(unsigned n=0;n<MAX_FT_PARTS;++n)map[n]=(uint16_t)((n+1)%MAX_FT_PARTS);
 bool accepted=nativeFighterRig_RemapReferences(&project,map,&out,error,sizeof(error));
 CHECK(accepted==!affected);
 if(accepted){compare(&project,map,&out,false);free(out.fighter);out=(NativeFighterRigReferences){0};}
 else CHECK(!out.fighter&&!out.fighter_size&&strstr(error,"hitbox flags"));
 ++cases;
 // Own the flag without moving any commands or inventing a duration. The
 // actual importer must observe the old following-word flag in each header.
 NativeFighterProject owned=project;owned.fighter_dat=malloc(bytes.size);CHECK(owned.fighter_dat);
 memcpy(owned.fighter_dat,bytes.data,bytes.size);unsigned converted=0;
 CHECK(nativeFighterAction_OwnHitboxFlags(&owned,&converted,error,sizeof(error))&&owned.fighter_size==bytes.size);
 for(unsigned n=0;n<MAX_FT_PARTS;++n)map[n]=(uint16_t)n;
 NativeFighterRigReferences explicit={owned.fighter_dat,owned.fighter_size,0,0};CHECK(compare(&project,map,&explicit,true)==converted);++cases;
 uint8_t* once=malloc(owned.fighter_size);CHECK(once);memcpy(once,owned.fighter_dat,owned.fighter_size);
 unsigned again=99;CHECK(nativeFighterAction_OwnHitboxFlags(&owned,&again,error,sizeof(error))&&!again&&!memcmp(once,owned.fighter_dat,owned.fighter_size));
 bool edited=false;
 for(unsigned action=0;action<project.action_count&&!edited;++action){
  size_t count;CHECK(nativeFighterAction_Commands(&project,action,NULL,0,&count));
  NativeFighterActionCommand* rows=count?malloc(count*sizeof(*rows)):NULL;CHECK(!count||rows);
  if(count)CHECK(nativeFighterAction_Commands(&project,action,rows,count,&count));
  for(size_t n=0;n<count&&!edited;++n)if(nativeHitbox_IsCreate(rows[n].opcode)){
   unsigned users;CHECK(nativeFighterAction_Users(&owned,rows[n].source_offset,&users));if(users!=1)continue;
   NativeFighterHitbox before,after;
   CHECK(nativeFighterAction_GetHitbox(&project,action,rows[n].source_offset,&before));
   CHECK(nativeFighterAction_GetHitbox(&owned,action,rows[n].source_offset,&after));
   CHECK(before.only_hit_grabbed==after.only_hit_grabbed&&before.damage==after.damage&&before.bone==after.bone&&before.timed==after.timed);
   after.damage=(uint16_t)((after.damage+1)%1024);
   CHECK(nativeFighterAction_SetHitbox(&owned,action,rows[n].source_offset,&after,NULL));
   NativeFighterHitbox changed;CHECK(nativeFighterAction_GetHitbox(&owned,action,rows[n].source_offset,&changed));
   CHECK(changed.damage==after.damage&&changed.only_hit_grabbed==before.only_hit_grabbed);edited=true;
  }
  free(rows);
 }
 CHECK(edited);memcpy(owned.fighter_dat,once,owned.fighter_size);++cases;
 free(once);++cases;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)map[n]=(uint16_t)((n+1)%MAX_FT_PARTS);
 CHECK(nativeFighterRig_RemapReferences(&owned,map,&out,error,sizeof(error)));CHECK(compare(&project,map,&out,true)==converted);++cases;
 free(out.fighter);free(owned.fighter_dat);out=(NativeFighterRigReferences){0};
 for(unsigned n=0;n<MAX_FT_PARTS;++n)map[n]=(uint16_t)n;map[1]=0;
 CHECK(!nativeFighterRig_RemapReferences(&project,map,&out,error,sizeof(error))&&!out.fighter&&!out.fighter_size);++cases;
 HSD_Dat dat;u32 root,scale,bone;const char* external;
 CHECK(HSD_DatOpen(&dat,bytes.data,bytes.size)==HSD_DAT_OK&&HSD_DatFindRoot(&dat,project.fighter_root,&root));
 CHECK(HSD_DatResolve(&dat,root+52,&scale,&external)==HSD_DAT_REF_LOCAL&&HSD_DatReadU32(&dat,scale,&bone)&&bone<MAX_FT_PARTS);
 map[1]=1;map[bone]=UINT16_MAX;
 CHECK(!nativeFighterRig_RemapReferences(&project,map,&out,error,sizeof(error))&&!out.fighter&&!out.fighter_size);++cases;
 --project.fighter_size;
 CHECK(!nativeFighterRig_RemapReferences(&project,inverse,&out,error,sizeof(error))&&!out.fighter&&!out.fighter_size);++cases;
 printf("PASS %u cases %u commands %llu checks\n",cases,commands,checks);
 free(facts.data);free(bytes.data);return 0;
}
