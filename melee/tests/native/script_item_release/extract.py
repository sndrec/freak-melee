"""Exact current release call chain and retained native comparison bodies."""
import hashlib,importlib.util,re,subprocess
REFERENCE='948cf3ed5'
SELECTION={'src/melee/ft/ftidentity.c':['ftLib_80086960'],'src/melee/it/it_2725.c':['it_80273B50','it_80273F34','it_8027429C'],'src/melee/it/item.c':['Item_8026A848'],'src/melee/ft/ftlib.c':['ftLib_80086630','ftLib_80086724','ftLib_800867A0','ftLib_GetItem','ftLib_800867CC','ftLib_80086764'],'src/melee/ft/ftcommon.c':['ftCommon_8007E6DC','ftCommon_8007E82C']}
def module(path,name):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def extract(root,out):
 hashes={};path=root/'tests/native/script_form_feedback/extract.py';body=module(path,'release_body').body;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def read(relative):
  p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 text=read('src/melee/ft/native_script_binding_properties.c');guards=[re.sub(r'\berror\b','release_guard_error',body(text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']];(out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 text=read('tests/native/script_target_queries/boundary.c');text=text.replace('#include "native_target.inc"','').replace('k==target_foreign_package.target?&target_foreign_package:NULL','NULL').replace(body(text,'nativeScriptBindings_Write'),'');(out/'boundary.c').write_text(text)
 text=read('tests/native/script_target_bone_unavailable.c');text=text.replace('HSD_JObjInfo hsdJObj;','').replace(body(text,'hsdObjIsDescendantOf'),'');(out/'bone_unavailable.c').write_text(text)
 names=[n for group in SELECTION.values() for n in group];rename=lambda s:re.sub(r'\b('+'|'.join(names)+r')\b',lambda m:'reference_'+m[0],s)
 actual=[];reference=[]
 for relative,functions in SELECTION.items():
  text=read(relative);old=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for n in functions:actual.append(body(text,n));reference.append(rename(body(old,n)))
 text=read('src/sysdolphin/baselib/class.c');actual.append(body(text,'hsdObjIsDescendantOf'))
 text=read('src/melee/it/native_item_owner.c');actual += [body(text,n) for n in ['nativeItem_CaptureCreator','nativeItem_Creator']]
 text=read('tests/native/script_receiver_unavailable_identity.c');text=text.replace(body(text,'nativeItem_Creator'),'');(out/'identity_unavailable.c').write_text(text)
 text=read('src/melee/it/it_2725.c');macros=text[text.index('#define it_2725_JObjSetTranslate'):text.index('static inline void it_2725_JObjSetTranslateInline')]
 headers=['melee/it/ithitbox.h','melee/it/it_3F14.h','melee/ft/fighter.h','melee/it/it_2725.h','melee/it/item.h','melee/it/itmaplib.h','melee/it/it_279C.h','melee/it/it_26B1.h','melee/it/inlines.h','melee/it/native_item_lifetime.h','melee/it/native_item_owner.h','melee/pl/plbonuslib.h','melee/ft/native_fighter_lifetime.h','melee/ft/ftcommon.h','melee/ft/ftlib.h','melee/ft/ftdata.h','melee/ft/kinds/ftCommon/ftCo_HammerWait.h','melee/lb/lb_00B0.h','melee/mp/mpcoll.h','sysdolphin/baselib/jobj.h','sysdolphin/baselib/class.h','math.h']
 (out/'kernels.h').write_text('\n'.join('#include <'+h+'>' for h in headers)+'\n'+'\n'.join(s[:s.index('{')].strip()+';' for s in actual+reference)+'\n')
 (out/'kernels.c').write_text('#include "kernels.h"\n'+macros+'\n'+'\n'.join(actual+reference)+'\n')
 return hashes
