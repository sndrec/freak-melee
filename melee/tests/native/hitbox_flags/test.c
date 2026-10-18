#include <melee/ft/ftaction.h>
#include <melee/ft/ftaction_data.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ft_part_layout.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_hitbox_lifetime.h>
#include <melee/lb/inlines.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"case %u line %d: %s\n",cases,__LINE__,#x);exit(1);}}while(0)
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
void ftAction_800715EC(Fighter_GObj*,CommandInfo*);
void ftCommon_80080484(Fighter*);
// Common damage/angle calculations are independent fixture boundaries. The
// source-derived command handler and victim/group history run unchanged.
void ftColl_8007ABD0(HitCapsule* hit,u32 damage,Fighter_GObj* object)
{CHECK(object&&object->user_data);hit->damage=(float)damage;hit->unk_count=damage;}
void ftColl_8007AC9C(HitCapsule* hit,int angle,Fighter_GObj* object)
{CHECK(object&&object->user_data);hit->kb_angle=angle;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part part)
{CHECK(fighter&&fighter->parts);return (Fighter_Part)(((unsigned)part+3)%MAX_FT_PARTS);}
void ftColl_8007AD18(Fighter* fighter,HitCapsule* hit)
{(void)fighter;(void)hit;CHECK(false);}
#include "bodies.inc"
static Fighter initial,actual,expected;
static FighterBone parts[MAX_FT_PARTS];
static HSD_JObj joints[MAX_FT_PARTS];
static uint32_t random_word(uint32_t* seed)
{*seed=*seed*1664525u+1013904223u;return *seed;}
int main(void)
{
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 uint32_t seed=0x61d940ac;
 for(unsigned sample=0;sample<8192;++sample){
  memset(&initial,0,sizeof(initial));initial.parts=parts;initial.smash_attrs.x2138_smashSinceHitbox=-1;
  union CmdUnion old[6]={{0}},owned[6];
  const unsigned slot=sample%4,mode=sample/4%8;
  old[0].create_hitbox_0.opcode=11;old[0].create_hitbox_0.id=slot;
  old[0].create_hitbox_0.hit_group=random_word(&seed)%8;
  old[0].create_hitbox_0.only_hit_grabbed=random_word(&seed)%2;
  old[0].create_hitbox_0.use_common_bone_ids=sample/32%2;
  old[0].create_hitbox_0.bone=random_word(&seed)%(old[0].create_hitbox_0.use_common_bone_ids?FT_COMMON_PART_COUNT:MAX_FT_PARTS);
  old[0].create_hitbox_0.damage=random_word(&seed)%1024;
  for(unsigned word=1;word<5;++word){uint32_t value=random_word(&seed);memcpy(&old[word],&value,4);}
  old[3].create_hitbox_3.ignore_thrown_fighters=sample/64%2;
  initial.x1064_thrownHitbox.owner=sample/128%2?(void*)(uintptr_t)0x1000:NULL;
  old[5].spawn_gfx_0.opcode=10;old[5].spawn_gfx_0.boneId=sample/256%140;
  for(unsigned n=0;n<4;++n){
   initial.x914[n].state=(mode&(1u<<(n%3)))?HitCapsule_Enabled:HitCapsule_Disabled;
   initial.x914[n].x4=n%2?old[0].create_hitbox_0.hit_group:(old[0].create_hitbox_0.hit_group+1)%8;
   memset(initial.x914[n].victims_1,0x35+n,sizeof(initial.x914[n].victims_1));
   memset(initial.x914[n].victims_2,0x74+n,sizeof(initial.x914[n].victims_2));
   initial.x914[n].x44=(u8)(n+1);initial.x914[n].x45=(u8)(n+2);
   initial.native_hitbox_lifetimes[n]=(NativeHitboxLifetime){17.25f,1,1};
  }
  memcpy(owned,old,sizeof(old));owned[0].create_hitbox_0.opcode=NATIVE_HITBOX_EXPLICIT_OPCODE;
  owned[0].create_hitbox_0.only_hit_grabbed=ftActionData_NextHitboxFlag(old+5);
  owned[5].spawn_gfx_0.boneId=old[5].spawn_gfx_0.boneId^2;
  CHECK(ftActionData_NextHitboxFlag(old+5)!=ftActionData_NextHitboxFlag(owned+5));
  actual=initial;expected=initial;HSD_GObj a={0},b={0};a.user_data=&actual;b.user_data=&expected;
  CommandInfo ac={0},bc={0};ac.u=owned;bc.u=old;
  original_hitbox(&b,&bc);ftAction_8007121C(&a,&ac);
  CHECK(ac.u==owned+5&&bc.u==old+5);CHECK(!memcmp(&actual,&expected,sizeof(actual)));
  // The retail opcode must also retain its original following-word behavior.
  actual=initial;ac.u=old;ftAction_8007121C(&a,&ac);
  CHECK(ac.u==old+5&&!memcmp(&actual,&expected,sizeof(actual)));
  // Timed spawning uses the same initializer before arming its lifetime.
  owned[0].create_hitbox_0.opcode=NATIVE_HITBOX_TIMED_OPCODE;
  actual=initial;ac.u=owned;ftAction_8007121C(&a,&ac);
  CHECK(ac.u==owned+5&&!memcmp(&actual,&expected,sizeof(actual)));++cases;
 }
 CHECK(ftActionData_SourceCommandWidth((uint32_t)NATIVE_HITBOX_EXPLICIT_OPCODE<<26)==5);
 puts("PASS");printf("{\"cases\":%u,\"checks\":%u}\n",cases,checks);return 0;
}
