// Compiler-prepared child paths, actual Item actor/VM delivery, and native
// synchronous ABI dispatch. Effect rendering remains a recorded fixture leaf.
static void joint_effect_cases(void)
{
 CHECK(specs[0].model_selector_count==1);
 NativeScriptItemModelSelector* selector=&model_selectors[0][0];
 CHECK((selector->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT);
 CHECK(selector->depth==2&&selector->path[0]==0&&selector->path[1]==0&&selector->argument_count==2);
 for(unsigned mutation=0;mutation<=7;++mutation){
  HSD_GObj* object=create(0);HSD_JObj* root=object->hsd_obj;
  root->child=HSD_JObjAlloc();root->child->child=HSD_JObjAlloc();
  HSD_JObj* grandchild=root->child->child;
  value(object,"effect",1151,true);unsigned before=spawns;
  NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
  uint64_t identity=bank.storage_parameters.identity;mode=mutation;
  NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptVMStatus status=nativeScriptItem_RunHook(object,NativeScriptItemHook_ShieldBounced,NULL,0,&result);
  CHECK(spawns==before+1&&spawned_id==1151&&spawned_root==grandchild);
  if(mutation)CHECK(status!=NativeScriptVM_Ok&&result.type==NativeScriptType_U64&&result.as.u64==99);
  else {CHECK(status==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&!result.as.u64);value(object,"completed",1,false);}
  bank.storage_parameters.identity=identity;mode=0;
  if(nativeMatchPool_Resolve(reference)){if(mutation==1)item.x1C=17;cleanup(object);}
 }
 for(unsigned failure=0;failure<5;++failure){
  HSD_GObj* object=create(0);HSD_JObj* root=object->hsd_obj;
  root->child=HSD_JObjAlloc();root->child->child=HSD_JObjAlloc();
  int id=1151;NativeScriptItemModelSelector saved=*selector;
  if(failure==0)root->child=NULL; // Native-style absent selected path: no spawn.
  if(failure==1)id=1040; // Position ABI must not enter a Joint constructor.
  if(failure==2)id=-1;
  if(failure==3)++selector->path_length;
  if(failure==4)HSD_CLASS_METHOD(root->child->child)=&base_class;
  value(object,"effect",id,true);unsigned before=spawns;
  NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptVMStatus status=nativeScriptItem_RunHook(object,NativeScriptItemHook_ShieldBounced,NULL,0,&result);
  CHECK(spawns==before);
  if(failure)CHECK(status!=NativeScriptVM_Ok&&result.type==NativeScriptType_U64&&result.as.u64==99);
  else {CHECK(status==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&!result.as.u64);value(object,"completed",1,false);}
  *selector=saved;cleanup(object);
 }
}
