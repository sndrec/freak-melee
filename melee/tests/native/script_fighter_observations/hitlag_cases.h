// Pure physical hitlag observations use the real backend and property/receiver
// routes. No hitlag callback or release mutation is simulated here.
static void hitlag_cases(HSD_GObj* owned,HSD_GObj* foreign,HSD_GObj* stock,HSD_GObj* caller)
{
 HSD_GObj* targets[]={owned,foreign,stock};
 for(unsigned t=0;t<3;++t){Fighter* f=targets[t]->user_data;NativeScriptRuntimeContext c=context(targets[t]);
  Fighter_GObj* saved_relation=f->x1A5C;uint32_t saved_stamp=f->native_hitlag_link_generation;
  for(unsigned active=0;active<2;++active)for(unsigned other=0;other<2;++other)for(unsigned relation=0;relation<2;++relation){
   f->x2219_b0=f->x2219_b1=f->x2219_b2=f->x2219_b3=f->x2219_b4=f->x2219_b6=f->x2219_b7=other;f->x2219_b5=active;
   f->x1A5C=relation?(Fighter_GObj*)(uintptr_t)1:NULL;f->native_hitlag_link_generation=relation?UINT32_MAX:0;
   static Fighter before;before=*f;NativeScriptValue r;
   CHECK(nativeScriptBindingHitlag_Read(&c,NativeScriptBinding_HitlagActive,&r,&diagnostic));CHECK(r.type==NativeScriptType_Bool&&r.as.u64==f->x2219_b5);++pairs;
   CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_HitlagActive,NULL,0,&r,&diagnostic));CHECK(r.type==NativeScriptType_Bool&&r.as.u64==active);++pairs;
   if(t==0){CHECK(helper(owned,"ObserveHitlag"));r=field(owned,"hitlag");CHECK(r.type==NativeScriptType_Bool&&r.as.u64==active);}
   CHECK(nativeScriptItem_SetAttachmentOwner(caller,targets[t]));
   CHECK(nativeScriptItem_RunSignal(caller,name_key("ObserveOwnerHitlag"),0,&r)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;
   r=field(caller,"hitlag");CHECK(r.type==NativeScriptType_Bool&&r.as.u64==active);
   CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
  }
  f->x1A5C=saved_relation;f->native_hitlag_link_generation=saved_stamp;
 }
 NativeScriptRuntimeContext c=context(caller);NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptValue token,r;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,stock));
 CHECK(nativeScriptReceiver_Push(scope.pool,&c,c.scene_epoch,&primary_bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&diagnostic));
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
 ((Fighter*)stock->user_data)->x2219_b5=1;((Fighter*)foreign->user_data)->x2219_b5=0;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,foreign));
 CHECK(nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_HitlagActive,&token,1,&r,&diagnostic));CHECK(r.type==NativeScriptType_Bool&&r.as.u64==1);++cases;++pairs;
 Fighter* f=stock->user_data;uint32_t generation=f->x8_spawnNum;FighterKind kind=f->kind;u16 classifier=stock->classifier;
 for(unsigned mode=0;mode<4;++mode){
  if(mode==0)++f->x8_spawnNum;if(mode==1)f->kind=Ft_Kind_Zelda;if(mode==2)stock->classifier=HSD_GOBJ_CLASS_ITEM;if(mode==3){substitute=*f;stock->user_data=&substitute;}
  r=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_HitlagActive,&token,1,&r,&diagnostic));CHECK(r.type==NativeScriptType_U64&&r.as.u64==99);++cases;
  f->x8_spawnNum=generation;f->kind=kind;stock->classifier=classifier;stock->user_data=f;
 }
 NativeFighterLifetime retained;CHECK(nativeFighterLifetime_Capture(f,&retained));ftData* data=f->ft_data;f->ft_data=NULL;CHECK(!nativeFighterLifetime_Current(&retained));f->ft_data=data;CHECK(nativeFighterLifetime_Current(&retained));++cases;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,NULL));
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,false,false,&token,&diagnostic));
 r=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_HitlagActive,&token,1,&r,&diagnostic));CHECK(r.type==NativeScriptType_U64&&r.as.u64==99);++cases;
 CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));
 c=context(stock);static Fighter before;before=*f;
 const uint32_t operations[]={NativeScriptBinding_HitlagLinkedId,NativeScriptBinding_HitlagLink,NativeScriptBinding_HitlagUnlink,NativeScriptBinding_HitlagRequestRelease};
 NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=0};
 for(unsigned s=0;s<4;++s){r=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  bool ok=s?nativeScriptBindingHitlag_Invoke(&c,operations[s],s==1?&arg:NULL,s==1?1:0,&r,&diagnostic):nativeScriptBindingHitlag_Read(&c,operations[s],&r,&diagnostic);
  CHECK(!ok&&r.type==NativeScriptType_U64&&r.as.u64==99&&!memcmp(f,&before,sizeof(*f)));++cases;
 }
}
