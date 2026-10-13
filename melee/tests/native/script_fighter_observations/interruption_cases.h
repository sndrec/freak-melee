#include <melee/ft/ft_0881.h>
#include <melee/ft/ftcommon.h>
static unsigned interruption_audio,interruption_native_damage,interruption_native_death;
static HSD_GObj* interruption_source;
static unsigned interruption_mode;
int lbAudioAx_80023870(int sound,int volume,int pan,int channel)
{
 Fighter* f=interruption_source->user_data;
 CHECK(sound==0x83D61&&volume==0&&pan==0x40);
 CHECK(channel==(int)((interruption_audio?0x2A:0x1E)+f->player_id*2+f->x221F_b4));
 CHECK(interruption_audio<2);++interruption_audio;return 0;
}
static void interruption_damage(HSD_GObj* object)
{
 CHECK(object==interruption_source&&interruption_audio==2);
 Fighter* f=object->user_data;CHECK(f->x2144==-1&&f->x2148==-1);
 ++interruption_native_damage;
 if(interruption_mode==1)nativeScriptFighter_Detach(object);
}
static void interruption_death(HSD_GObj* object)
{
 CHECK(object==interruption_source&&interruption_audio==2);++interruption_native_death;
}
#include "native_interrupt_engine.inc"
static void interruption_reset(HSD_GObj* object,unsigned mask)
{
 interruption_source=object;interruption_audio=interruption_native_damage=interruption_native_death=0;
 Fighter* f=object->user_data;f->x2144=71;f->x2148=83;
 f->player_id=(mask>>4)%6;f->x221F_b4=(mask>>3)&1;
 CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,
  (mask&4)?function("CleanupDamage"):UINT32_MAX));
 CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Death1,
  (mask&8)?function("CleanupDeath"):UINT32_MAX));
 // A non-null native slot deliberately supersedes its script registration.
 // Exercise native fallback only when that slot has no interpreted hook.
 f->take_dmg_cb=((mask&1)&&!(mask&4))?interruption_damage:NULL;
 f->death1_cb=((mask&2)&&!(mask&8))?interruption_death:NULL;
 CHECK(helper(object,"ResetCleanup"));
}
static void interruption_cases(HSD_GObj* owned,HSD_GObj* stock)
{
 Fighter* f=owned->user_data;Fighter saved=*f;interruption_mode=0;
 for(unsigned mask=0;mask<64;++mask) {
  interruption_reset(owned,mask);Fighter before=*f;
  NativeScriptRuntimeContext c=context(owned);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_DamageInterruptCleanup,NULL,0,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&interruption_audio==2);
  CHECK(interruption_native_damage==((mask&1)&&!(mask&4)?1u:0u));
  CHECK(interruption_native_death==((mask&2)&&!(mask&8)?1u:0u));
  unsigned order=(mask&4)?3:0;if(mask&8)order=order*10+4;
  CHECK(field(owned,"cleanup_order").as.i64==order);
  before.x2144=before.x2148=-1;CHECK(!memcmp(f,&before,sizeof(before)));++pairs;
  interruption_reset(owned,mask);CHECK(helper(owned,"RunCleanup"));
  CHECK(interruption_audio==2&&field(owned,"cleanup_order").as.i64==order);++cases;
 }
 // Replacing a bound script with a native pointer must use that pointer,
 // matching the engine's callback-registration precedence.
 interruption_reset(owned,12);f->take_dmg_cb=interruption_damage;f->death1_cb=interruption_death;
 CHECK(helper(owned,"RunCleanup"));
 CHECK(interruption_native_damage==1&&interruption_native_death==1&&field(owned,"cleanup_order").as.i64==0);++cases;
 NativeScriptRuntimeContext c=context(owned);
 for(unsigned mode=0;mode<8;++mode) {
  NativeScriptRuntimeContext bad=c;
  if(mode==0)bad=context(stock);
  if(mode==1)++bad.generation;
  if(mode==2)++bad.scene_epoch;
  if(mode==3)bad.source_class=NativeCallbackSource_Item;
  if(mode==4)bad.source=NULL;
  NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owned,c.generation,&view));
  bool live=view.storage->live;if(mode==5)view.storage->live=false;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};Fighter before=*f;
  interruption_audio=0;
  CHECK(!nativeScriptBindings_Invoke(&bad,NativeScriptBinding_DamageInterruptCleanup,NULL,0,NULL,mode==6?1:0,
   mode==7?NULL:&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!interruption_audio&&!memcmp(f,&before,sizeof(before)));
  view.storage->live=live;++cases;
 }
 // Native fallback cleanup can detach the source without changing its physical
 // generation. The selected binding must still refuse success publication.
 interruption_reset(owned,3);interruption_mode=1;
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_DamageInterruptCleanup,NULL,0,NULL,0,&out,&diagnostic));
 CHECK(interruption_audio==2&&interruption_native_damage==1&&interruption_native_death==1);
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 CHECK(nativeScriptFighter_Attach(owned));interruption_mode=0;
 CHECK(nativeScriptFighter_BindCallback(owned,NativeScriptFighterCallback_Damage,UINT32_MAX));
 CHECK(nativeScriptFighter_BindCallback(owned,NativeScriptFighterCallback_Death1,UINT32_MAX));
 *f=saved;
 // A fault in an actual interpreted damage hook stops the native kernel before
 // death cleanup and must propagate failure without publishing a Void result.
 HSD_GObj* faulted=actor(0);interruption_reset(faulted,3);
 CHECK(nativeScriptFighter_BindCallback(faulted,NativeScriptFighterCallback_Damage,function("CleanupFault")));
 NativeScriptRuntimeContext fault_context=context(faulted);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindings_Invoke(&fault_context,NativeScriptBinding_DamageInterruptCleanup,NULL,0,NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&interruption_audio==2&&!interruption_native_damage&&!interruption_native_death);
 CHECK(nativeScriptFighter_Status(faulted)==NativeScriptVM_Error);++cases;HSD_GObjFree(faulted);
}
