// Static payloads/imported metadata are explicit fixture inputs. Actual GObj
// pools, actors, storage, ItemView retention and native scalar kernels execute.
float ftCo_80092ED8(int damage,float light);
static void stock_observation_remove(void* payload){(void)payload;}
static void write_field(HSD_GObj* object,const char* name,NativeScriptValue value)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptReceiverView view;
 CHECK(nativeScriptActors_Lookup(NULL,object,c.source_class,c.generation,&view));
 for(uint32_t n=0;n<view.program->value_count;++n)
  if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
   CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));return;
  }
 CHECK(false);
}
static void shield_rejected(NativeScriptRuntimeContext* c)
{
 const uint32_t ids[]={NativeScriptBinding_ShieldPushbackDamageScale,NativeScriptBinding_ShieldPushbackBase,
  NativeScriptBinding_ShieldPushbackVelocityScale,NativeScriptBinding_ShieldPushbackLightMin,NativeScriptBinding_ShieldPushbackLightMax};
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=0xfedcba9876543210};
 for(unsigned n=0;n<5;++n){NativeScriptValue output=sentinel;
  CHECK(!nativeScriptBindings_Read(c,ids[n],NULL,0,&output,&diagnostic));CHECK(!memcmp(&sentinel,&output,sizeof(output)));++observation_rejections;
 }
}
static void heavy_rejected(NativeScriptRuntimeContext* c)
{
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=0xfedcba9876543210},output=sentinel;
 CHECK(!nativeScriptBindings_Read(c,NativeScriptBinding_ItemsHeavy,NULL,0,&output,&diagnostic));
 CHECK(!memcmp(&output,&sentinel,sizeof(output)));++observation_rejections;
}
static void shield_weight_cases(HSD_GObj* owned,HSD_GObj* foreign,HSD_GObj* raw,HSD_GObj* article_object)
{
 const uint32_t ids[]={NativeScriptBinding_ShieldPushbackDamageScale,NativeScriptBinding_ShieldPushbackBase,
  NativeScriptBinding_ShieldPushbackVelocityScale,NativeScriptBinding_ShieldPushbackLightMin,NativeScriptBinding_ShieldPushbackLightMax};
 const char* names[]={"push_damage","push_base","push_velocity","push_min","push_max"};
 float* coefficients[]={&shield_common.x28C,&shield_common.x290,&shield_common.x294,&shield_common.x2E4,&shield_common.x2E8};
 const uint32_t words[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,1,0x80000001,0x7f7fffff};
 NativeScriptRuntimeContext c=context(owned),foreign_context=context(foreign),raw_context=context(raw);
 Fighter* f=owned->user_data;
 for(unsigned n=0;n<5;++n)for(unsigned w=0;w<sizeof(words)/sizeof(words[0]);++w){
  memcpy(coefficients[n],&words[w],4);NativeScriptValue value;
  CHECK(nativeScriptBindings_Read(&c,ids[n],NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,coefficients[n],4));
  CHECK(nativeScriptBindings_Read(&foreign_context,ids[n],NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,coefficients[n],4));
  CHECK(helper(owned,"ObservePushback"));value=field(owned,names[n]);CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,coefficients[n],4));
  shield_pairs+=2;++shield_vm_cases;++cases;
 }
 for(unsigned n=0;n<256;++n){
  shield_common.x28C=.25f*(float)(1+n%7);shield_common.x290=.125f*(float)(n%5);
  shield_common.x294=.5f*(float)(1+n%3);shield_common.x2E4=.125f*(float)(n%4);shield_common.x2E8=.5f+(float)(n%3)*.125f;
  int damage=(int)(n%31)-7;float light=.125f*(float)(n%9);
  float expected=ftCo_80092ED8(damage,light)*shield_common.x294;
  write_field(owned,"hit_damage",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=damage});
  write_field(owned,"light",(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=light});
  CHECK(helper(owned,"Pushback"));NativeScriptValue value=field(owned,"push_result");
  CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,4));++shield_pairs;++shield_vm_cases;++cases;
 }
 p_ftCommonData=NULL;shield_rejected(&c);p_ftCommonData=&shield_common;shield_rejected(&raw_context);
 NativeScriptRuntimeContext bad=c;++bad.generation;shield_rejected(&bad);heavy_rejected(&bad);
 bad=c;++bad.scene_epoch;shield_rejected(&bad);heavy_rejected(&bad);
 NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=1};
 for(unsigned n=0;n<5;++n)CHECK(!nativeScriptBindings_Write(&c,ids[n],NULL,0,&value,&diagnostic));
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_ItemsHeavy,NULL,0,&value,&diagnostic));
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owned,c.generation,&view));
 NativeScriptStorageLayout layout=*view.storage->layout;const NativeScriptStorageLayout* original=view.storage->layout;
 view.storage->layout=&layout;shield_rejected(&c);heavy_rejected(&c);view.storage->layout=original;
 static Item stock;static ItemAttr attributes;
 HSD_GObj* stock_object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(stock_object);
 stock=(Item){.entity=stock_object,.kind=It_Kind_Box,.x1C=(int32_t)0x81234567,.xCC_item_attr=&attributes};
 GObj_InitUserData(stock_object,6,stock_observation_remove,&stock);
 ((Item*)article_object->user_data)->xCC_item_attr=&article_attributes;
 for(unsigned target=0;target<3;++target)for(unsigned bits=0;bits<2;++bits)for(unsigned heavy=0;heavy<2;++heavy){
  HSD_GObj* object=target==2?article_object:stock_object;Item* item=object->user_data;ItemAttr* attr=item->xCC_item_attr;
  if(target==1){stock.native_creator=foreign;stock.native_creator_generation=((Fighter*)foreign->user_data)->x8_spawnNum;stock.native_creator_kind=((Fighter*)foreign->user_data)->kind;}
  else stock.native_creator=NULL;
  ((unsigned char*)attr)[0]=bits?255:0;attr->x0_is_heavy=heavy;f->item_gobj=object;
  static Item before;before=*item;static Fighter before_fighter;before_fighter=*f;
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemsHeavy,NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_Bool&&value.as.u64==itIsHeavy(object));
  CHECK(helper(owned,"ObserveWeight"));CHECK(field(owned,"heavy").as.u64==heavy);
  CHECK(!memcmp(item,&before,sizeof(before))&&!memcmp(f,&before_fighter,sizeof(*f)));++heavy_pairs;++cases;
 }
 f->item_gobj=NULL;CHECK(helper(owned,"ObserveWeight"));CHECK(!field(owned,"heavy").as.u64);++heavy_pairs;
 f->item_gobj=stock_object;stock.xCC_item_attr=NULL;heavy_rejected(&c);stock.xCC_item_attr=&attributes;
 stock_object->user_data=NULL;heavy_rejected(&c);stock_object->user_data=&stock;
 stock_object->classifier=HSD_GOBJ_CLASS_FIGHTER;heavy_rejected(&c);stock_object->classifier=HSD_GOBJ_CLASS_ITEM;
 stock.entity=NULL;heavy_rejected(&c);stock.entity=stock_object;
 f->item_gobj=article_object;Item* authored=article_object->user_data;uint32_t generation=(uint32_t)authored->x1C;
 NativeScriptReceiverView item_view;CHECK(nativeScriptItem_View(article_object,generation,&item_view));
 NativeScriptStorageParameters parameters=*item_view.storage->parameters;const NativeScriptStorageParameters* original_parameters=item_view.storage->parameters;
 item_view.storage->parameters=&parameters;heavy_rejected(&c);item_view.storage->parameters=original_parameters;
 ++authored->x1C;heavy_rejected(&c);authored->x1C=(int32_t)generation;
 f->item_gobj=stock_object;HSD_GObjFree(stock_object);
 CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemsHeavy,NULL,0,&value,&diagnostic));CHECK(value.type==NativeScriptType_Bool&&!value.as.u64);++heavy_pairs;
 f->item_gobj=NULL;attributes=(ItemAttr){0};article_attributes=(ItemAttr){0};
}
static void compile_observation_negatives(void)
{
 const char* sources[]={"func Invalid():\n    items.heavy = true\n","func Invalid():\n    let bool heavy = items.heavy()\n",
  "func Invalid():\n    shield.pushback_base = 1.0\n","func Invalid():\n    let float value = shield.pushback_light_max()\n",
  "article Probe states 1:\n    on created:\n        let bool value = items.heavy\n    state 0 anim -1:\n",
  "article Probe states 1:\n    on created:\n        let float value = shield.pushback_damage_scale\n    state 0 anim -1:\n"};
 NativeScriptCompileBank scratch;static NativeScriptCompileBank sentinel;const NativeScriptCompileBank* accepted=&sentinel;
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<sizeof(sources)/sizeof(sources[0]);++n){configure(0);scratch=primary_bank;
  NativeScriptCompileRequest request={.source=sources[n],.source_bytes=(uint32_t)strlen(sources[n]),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
  CHECK(!nativeScriptCompile_Build(&scratch,&request,&accepted,&diagnostic));CHECK(accepted==&sentinel);++observation_rejections;++cases;
 }
}
