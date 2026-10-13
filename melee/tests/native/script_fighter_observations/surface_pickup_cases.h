#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/native_script_binding_item_observations.h>
#include <melee/it/it_26B1.h>

static void surface_signal(HSD_GObj* object,const char* name)
{
 NativeScriptValue result;
 CHECK(nativeScriptItem_RunSignal(object,name_key(name),0,&result)==NativeScriptVM_Ok);
 CHECK(result.type==NativeScriptType_Void);++vm_calls;
}
static void healing_cases(HSD_GObj* target,HSD_GObj* caller,bool self)
{
 Fighter* f=target->user_data;NativeScriptRuntimeContext c=context(target);
 for(unsigned neighbors=0;neighbors<2;++neighbors)for(unsigned initial=0;initial<2;++initial){
  f->x2222_b0=f->can_multijump=f->x2222_b2=f->x2222_b3=f->x2222_b5=f->x2222_b6=f->x2222_b7=neighbors;
  f->x2222_b4=initial;static Fighter before,expected;before=*f;
  NativeScriptValue value={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemsRejectHealingPickup,NULL,0,&value,&diagnostic));
  CHECK(value.type==NativeScriptType_Bool&&value.as.u64==initial&&!memcmp(f,&before,sizeof(*f)));++pairs;
  if(self){CHECK(helper(target,"ObserveHealing"));CHECK(field(target,"reject_healing").as.u64==initial);}
  CHECK(nativeScriptItem_SetAttachmentOwner(caller,target));surface_signal(caller,"ObserveHealing");++receiver_calls;
  CHECK(field(caller,"reject_healing").as.u64==initial&&!memcmp(f,&before,sizeof(*f)));
  for(unsigned desired=0;desired<2;++desired){
   expected=before;expected.x2222_b4=desired;value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=desired};
   *f=before;CHECK(nativeScriptBindingFighterItems_Write(&c,NativeScriptBinding_ItemsRejectHealingPickup,&value,&diagnostic));
   CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
   *f=before;CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_ItemsRejectHealingPickup,NULL,0,&value,&diagnostic));
   CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
   *f=before;surface_signal(caller,desired?"EnableHealing":"DisableHealing");++receiver_calls;
   CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
   if(self){*f=before;CHECK(helper(target,desired?"EnableHealing":"DisableHealing"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
  }
  *f=before;
  const unsigned bad_types[]={NativeScriptType_I32,NativeScriptType_U32,NativeScriptType_I64,NativeScriptType_F32,NativeScriptType_Object,NativeScriptType_Void};
  for(unsigned t=0;t<sizeof(bad_types)/sizeof(*bad_types);++t){value=(NativeScriptValue){.type=bad_types[t]};
   CHECK(!nativeScriptBindingFighterItems_Write(&c,NativeScriptBinding_ItemsRejectHealingPickup,&value,&diagnostic));CHECK(!memcmp(f,&before,sizeof(*f)));}
  value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};
  CHECK(!nativeScriptBindingFighterItems_Write(&c,NativeScriptBinding_ItemsRejectHealingPickup,&value,&diagnostic));CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 }
}
static void floor_rejected(NativeScriptRuntimeContext* c)
{
 for(unsigned axis=0;axis<3;++axis){NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingItemObservations_Read(c,NativeScriptBinding_FloorNormalX+axis,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
}
static void surface_pickup_cases(HSD_GObj* own,HSD_GObj* foreign,HSD_GObj* stock,HSD_GObj* caller)
{
 healing_cases(own,caller,true);healing_cases(foreign,caller,false);healing_cases(stock,caller,false);
 Item* item=caller->user_data;NativeScriptRuntimeContext c=context(caller);
 const uint32_t words[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
 for(unsigned contact=0;contact<2;++contact)for(unsigned w=0;w<sizeof(words)/sizeof(*words);++w){
  item->x378_itemColl.env_flags=contact?Collide_FloorMask:0;
  memcpy(&item->x378_itemColl.floor.normal.x,&words[w],4);
  memcpy(&item->x378_itemColl.floor.normal.y,&words[(w+1)%12],4);
  memcpy(&item->x378_itemColl.floor.normal.z,&words[(w+2)%12],4);
  static Item before;before=*item;const char* names[]={"normal_x","normal_y","normal_z"};
  for(unsigned axis=0;axis<3;++axis){NativeScriptValue out;const Vec3* normal=&item->x378_itemColl.floor.normal;
   const float* native=axis==0?&normal->x:axis==1?&normal->y:&normal->z;
   CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_FloorNormalX+axis,NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;}
  surface_signal(caller,"ObserveFloor");
  for(unsigned axis=0;axis<3;++axis){NativeScriptValue out=field(caller,names[axis]);const Vec3* normal=&item->x378_itemColl.floor.normal;
   const float* native=axis==0?&normal->x:axis==1?&normal->y:&normal->z;
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;}
  CHECK(!memcmp(item,&before,sizeof(*item)));++cases;
 }
 NativeScriptRuntimeContext invalid=c;++invalid.scene_epoch;floor_rejected(&invalid);invalid=c;++invalid.generation;floor_rejected(&invalid);
 floor_rejected(NULL);invalid=c;invalid.source_class=NativeCallbackSource_Fighter;floor_rejected(&invalid);
 item->native_removal_in_progress=true;floor_rejected(&c);item->native_removal_in_progress=false;
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(caller,c.generation,&view));
 NativeScriptStorageParameters parameters=primary_bank.storage_parameters;
 view.storage->parameters=&parameters;floor_rejected(&c);view.storage->parameters=&primary_bank.storage_parameters;
 primary_bank.valid=false;floor_rejected(&c);primary_bank.valid=true;
 NativeScriptRuntimeContext stale=c;HSD_GObjFree(caller);
 HSD_GObj* recycled=item_actor();CHECK(recycled==caller);floor_rejected(&stale);
 // Actual unchanged native heal-amount function; pickup eligibility itself is
 // the exact flag/amount expression at ftpickupitem.c:57 and :117.
 const ItemKind kinds[]={It_Kind_Heart,It_Kind_Tomato,It_Kind_Foods,It_Kind_Lucky_Egg,It_Kind_WhispyHealApple,It_Kind_Box};
 const int amounts[]={0,1,-1};Item sample={0};HSD_GObj object={.user_data=&sample};
 for(unsigned k=0;k<sizeof(kinds)/sizeof(*kinds);++k)for(unsigned a=0;a<3;++a){
  sample=(Item){.kind=kinds[k]};
  switch(kinds[k]){
  case It_Kind_Heart:sample.xDD4_itemVar.heart.xDD4_heal=amounts[a];break;
  case It_Kind_Tomato:sample.xDD4_itemVar.tomato.heal_amount=amounts[a];break;
  case It_Kind_Foods:sample.xDD4_itemVar.foods.heal_amount=amounts[a];break;
  case It_Kind_Lucky_Egg:sample.xDD4_itemVar.egg.heal_amount=amounts[a];break;
  case It_Kind_WhispyHealApple:sample.xDD4_itemVar.whispyapple.xDD4_heal=amounts[a];break;
  default:break;
  }
  int amount=it_8026B47C(&object);CHECK(amount==(k==5?0:amounts[a]));
  for(unsigned flag=0;flag<2;++flag){((Fighter*)own->user_data)->x2222_b4=flag;
   CHECK((!((Fighter*)own->user_data)->x2222_b4||!it_8026B47C(&object))==(!flag||amount==0));++pairs;}
  ++cases;
 }
}
