static void target_cases(HSD_GObj* caller,HSD_GObj* foreign,HSD_GObj* stock)
{
 NativeScriptRuntimeContext c=context(caller);Fighter* owner=caller->user_data;
 NativeScriptValue out,sentinel={.type=NativeScriptType_U64,.as.u64=99};
 Fighter* previous=owner->cpu.x44;owner->cpu.x44=NULL;
 CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CPUTargetID,NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U32&&!out.as.u64);
 CHECK(helper(caller,"Target")&&!(uint32_t)field(caller,"target_id").as.i64);++cases;
 HSD_GObj* choices[]={caller,foreign,stock};
 for(unsigned n=0;n<3;++n){
  Fighter* selected=choices[n]->user_data;owner->cpu.x44=selected;
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CPUTargetID,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U32&&out.as.u64==(uint32_t)selected->x8_spawnNum);
  CHECK(helper(caller,"Target")&&(uint32_t)field(caller,"target_id").as.i64==(uint32_t)selected->x8_spawnNum);
  CHECK(owner->cpu.x44==selected);++cases;++pairs;
 }
 Fighter* selected=stock->user_data;uint32_t saved_generation=selected->x8_spawnNum;
 const uint32_t identities[]={1,INT32_MAX,UINT32_C(0x80000000),UINT32_MAX};
 for(unsigned n=0;n<4;++n){
  selected->x8_spawnNum=identities[n];owner->cpu.x44=selected;
  CHECK(helper(caller,"Target")&&(uint32_t)field(caller,"target_id").as.i64==identities[n]);++cases;
 }
 selected->x8_spawnNum=saved_generation;
 // No dereference of an unregistered Fighter payload, wrong-class GObj or
 // mismatched payload backlink. Failed reads preserve the caller's output.
 for(unsigned bad=0;bad<3;++bad){
  uint16_t classifier=stock->classifier;HSD_GObj* backlink=selected->gobj;
  owner->cpu.x44=bad==0?&substitute:selected;
  if(bad==1)stock->classifier=HSD_GOBJ_CLASS_ITEM;
  if(bad==2)selected->gobj=caller;
  out=sentinel;CHECK(!nativeScriptBindingCPUControl_Read(&c,NativeScriptBinding_CPUTargetID,&out,&diagnostic));
  CHECK(!memcmp(&out,&sentinel,sizeof(out)));
  stock->classifier=classifier;selected->gobj=backlink;++cases;
 }
 HSD_GObj* old=actor(-1);owner->cpu.x44=old->user_data;HSD_GObjFree(old);
 HSD_GObj* replacement=actor(-1);CHECK(replacement==old);
 out=sentinel;CHECK(!nativeScriptBindingCPUControl_Read(&c,NativeScriptBinding_CPUTargetID,&out,&diagnostic));
 CHECK(!memcmp(&out,&sentinel,sizeof(out)));owner->cpu.x44=previous;
 HSD_GObjFree(replacement);++cases;
}
