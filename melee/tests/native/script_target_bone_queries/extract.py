"""Exact current native query/matrix bodies, with explicit external observers."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
def extract(root,out):
 hashes={};p=root/'tests/native/script_form_feedback/extract.py'
 s=importlib.util.spec_from_file_location('bone_body',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 def read(name):
  p=root/name;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=read('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','target_guard_error',m.body(props,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 marker=guards[1].index('{')+1;guards[1]=guards[1][:marker]+'\n    target_guard_probe(context);'+guards[1][marker:]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 target=read('src/melee/ft/native_fighter_target.c')
 bodies=[m.body(read('src/melee/ft/ftlib.c'),'ftLib_8008741C'),m.body(target,'nativeFighterTarget_BonePosition')]
 for file,functions in [
  ('src/melee/ft/ftlib.c',['ftLib_80086630','ftLib_800866DC','ftLib_800869D4','ftLib_80086A18','ftLib_80086BEC']),
  ('src/melee/ft/ftcommon_data.c',['ftCommon_GetModelScale']),
  ('src/melee/ft/kinds/ftCommon/ftCo_Guard.c',['ftCo_ShieldScale','ftCo_80094098']),
  ('src/melee/ft/native_fighter_target.c',['nativeFighterTarget_ModelScale','nativeFighterTarget_GuardActive','nativeFighterTarget_Shield','nativeFighterTarget_CameraPosition'])]:
  text=read(file)
  revision='ce94f0a0b' if file.endswith('ftCo_Guard.c') else '948cf3ed5'
  reference=subprocess.run(['git','show',revision+':'+file],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in functions:
   actual=m.body(text,name)
   if actual!=m.body(reference,name):raise RuntimeError('Native target geometry body changed: '+name)
   bodies.append(actual)
 (out/'native_target.inc').write_text('\n'.join(bodies)+'\n')
 ahead=[]
 for file,functions in [('src/melee/ft/ftlib.c',['ftLib_80086FD4','ftLib_80086368']),
                        ('src/melee/it/it_26B1.c',['it_8026B634','it_8026C258','it_8026BB88'])]:
  text=read(file)
  reference=subprocess.run(['git','show','ce94f0a0b:'+file],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in functions:
   actual=m.body(text,name)
   if actual!=m.body(reference,name):raise RuntimeError('Native ahead-search reference changed: '+name)
   ahead.append(actual)
 (out/'native_ahead.inc').write_text('\n'.join(ahead)+'\n')
 nearest=[]
 for file,functions in [
  ('src/melee/ft/ftlib.c',['ftLib_GetMotionId','ftLib_8008732C']),
  ('src/melee/ft/ft_0877.c',['ft_80087A18','ft_80087A80']),
  ('src/melee/ft/kinds/ftCommon/ftCo_0A01.c',['ftCo_800A0F00','ftCo_800A1AB4','inlineD0','inlineD1','ftCo_800A4A40'])]:
  text=read(file)
  reference=subprocess.run(['git','show','ce94f0a0b:'+file],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in functions:
   actual=m.body(text,name)
   if actual!=m.body(reference,name):raise RuntimeError('Native nearest-available reference changed: '+name)
   nearest.append(actual)
 (out/'native_nearest.inc').write_text('\n'.join(nearest)+'\n')
 native=[]
 for name,functions in [
  ('src/sysdolphin/baselib/jobj.c',['has_scl','HSD_JObjGetFlags','HSD_JObjClearFlags','HSD_JObjSetMtxDirtySub','HSD_JObjMakeMatrix','HSD_JObjSetupMatrixSub']),
  ('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf']),
  ('src/melee/lb/lbjoint_position.c',['lb_8000B1CC']),
  ('src/sysdolphin/baselib/mtx.c',['HSD_CalcDeterminantMatrix3x4','HSD_MtxInverse','HSD_MtxSRT','HSD_MtxSRTQuat']),
  ('src/melee/lb/lbcollision.c',['lbColl_80005EBC','lbColl_80006E58','lbColl_80008248']),
  ('src/melee/ft/ftcommon.c',['ftCommon_8007F804']),
  ('src/melee/ft/kinds/ftSamus/ftsamusspeciallw0.c',['ftSs_Init_80128A1C']),
  ('extern/dolphin/src/dolphin/mtx/mtx.c',['C_MTXIdentity','C_MTXCopy','C_MTXConcat','C_MTXQuat','C_MTXScale']),
  ('extern/dolphin/src/dolphin/mtx/mtxvec.c',['C_MTXMultVec']),
  ('extern/dolphin/src/dolphin/mtx/vec.c',['C_VECAdd','C_VECScale','C_VECDotProduct']),
  ('src/sysdolphin/baselib/robj.c',['dummy_func','loadRvalue','expLoadDesc','HSD_RObjExpressionUsesNativeDummy'])]:
  text=read(name)
  for function in functions:native.append(m.body(text,function))
 aurora=root.parent/'aurora/lib/dolphin/mtx/mtx.c';hashes[str(aurora)]=hashlib.sha256(aurora.read_bytes()).hexdigest();native.append(m.body(aurora.read_text(),'C_MTXTrans'))
 extra=out/'native_bodies.c';extra.write_text('#include "fixture.h"\n#include "native_leaves.h"\n#define EPSILON 0.0000000001f\nstatic void resolveIKJoint1(HSD_JObj*j){(void)j;abort();}\nstatic void resolveIKJoint2(HSD_JObj*j){(void)j;abort();}\n'+'\n'.join(native)+'\nvoid bone_install_dummy(HSD_RObj*r){HSD_ExpDesc d={0};r->flags=0x80000000;expLoadDesc(&r->u.exp,&d);}\n')
 resource=read('src/melee/it/native_article_resource.c')
 resource_names=['registered','same_reference','owner_current','nativeArticleResource_Collection','nativeArticleResource_AttachmentPhysical','nativeArticleResource_AttachmentCurrent','nativeArticleResource_CapturePhysical','nativeArticleResource_Capture','nativeArticleResource_Current','common_pull_state','nativeArticleResource_DetachBorrowers','nativeArticleResource_BindExternalPart','nativeArticleResource_ResolveBoundPart']
 (out/'native_resource_lookup.inc').write_text('ResourceCollection nativeArticleResourceCollections[NATIVE_ARTICLE_RESOURCE_COLLECTIONS];\n'+'\n'.join(m.body(resource,n) for n in resource_names)+'\n')
 h=read('src/melee/it/kinds/itsamusgrapple.c');c=read('src/melee/ft/ftcoll.c');l=read('src/melee/lb/lbcollision.c')
 descriptor=re.search(r'const itSamusGrapple_Hitbox it_803B8660 = \{.*?^\};',h,re.M|re.S).group()
 names=['lbColl_80008428','lbColl_80008434','lbColl_80008440','lbColl_CopyHitCapsule']
 (out/'native_hitbox_create.inc').write_text(descriptor+'\n'+'\n'.join(m.body(l,n) for n in names)+'\n'+m.body(c,'ftColl_800768A0')+'\n'+m.body(c,'ftColl_8007B8A8')+'\n'+m.body(c,'ftColl_8007AFF8')+'\n'+m.body(h,'it_802B7160').replace('it_802B7160','hitbox_native_reference',1)+'\n')
 lifetime=read('src/melee/ft/native_hitbox_lifetime.c')
 with (out/'native_hitbox_create.inc').open('a') as f:
  f.write('\n'+m.body(lifetime,'nativeHitboxLifetime_Clear')+'\n')
 return hashes
