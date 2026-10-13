"""Selected actual catch bodies and renamed unchanged pre-cohort native oracle."""
import hashlib,importlib.util,re,subprocess
REFERENCE='50f2b476b'
SELECTION={
 'Catch':['ftCo_Catch_CheckInput','ftCo_800D8A38','ftCo_800D8AE0','ftCo_800D8C54','ftCo_Catch_Anim','ftCo_CatchDash_Anim','fn_800D8E30','fn_800D8E94'],
 'CatchPull':['fn_800D9CE8','ftCo_CatchPull_Anim'],
 'CatchWait':['fn_800DA1D8','fn_800DA2B0','ftCo_CatchWait_IASA','fn_800DA4C0'],
 'CaptureWait':['ftCo_CaptureWait_Setup','fn_800DB6C8','fn_800DB790','fn_800DBAE4','fn_800DBBF8_noinline','fn_800DBBF8','ftCo_CaptureWaitLw_Phys_inline','fn_800DBAC4','fn_800DBED4_inline'],
 'CatchAttack':['fn_800DA4FC'],
 'Throw':['ftCo_800DD1E4_inline1','ftCo_800DD1E4_inline2','ftCo_800DD1E4_inline3','ftCo_800DD1E4','ftCo_800DD398','ftCo_800DD4B0','ftCo_ThrowF_Anim','ftCo_ThrowB_Anim','ftCo_ThrowHi_Anim','ftCo_ThrowLw_Anim'],
 'Thrown':['ftCo_800DE3FC'], 'AirCatch':['ftCo_800C3B10'], 'PickupItem':['ftpickupitem_Anim']}
def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
def extract(root,out):
 path=root/'tests/native/script_target_queries/extract.py';hashes=module(path,'catch_guards').extract(root,out);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_form_feedback/extract.py';body=module(path,'catch_body').body;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_target_queries/boundary.c';text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 text=text.replace('CALL(nativeScriptBindingTransition_Invoke)','')
 text=text.replace(body(text,'nativeFighterRuntime_Definition'),'')
 text=text.replace(body(text,'nativeFighterRuntime_SourcePackage'),'const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==target_package.target?&target_package:NULL;}')
 (out/'boundary.c').write_text(text)
 names=[n for group in SELECTION.values() for n in group];rename=lambda s:re.sub(r'\b('+'|'.join(names)+r')\b',lambda m:'reference_'+m[0],s)
 headers=set();actual=[];reference=[]
 for family,functions in SELECTION.items():
  path=root/('src/melee/ft/kinds/ftCommon/'+('ftpickupitem.c' if family=='PickupItem' else 'ftCo_'+family+'.c'));source=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  old=subprocess.run(['git','show',REFERENCE+':'+str(path.relative_to(root)).replace('\\','/')],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  headers.update(re.findall(r'^#include .+$',source,re.M))
  for name in functions:
   actual.append(re.sub(r'^static (?:inline )?','',body(source,name)));reference.append(rename(re.sub(r'^static (?:inline )?','',body(old,name))))
 # The heavy-item dispatch branch is copied verbatim with explicit enclosing
 # locals and a continuation marker. The surrounding damage/reaction engine is
 # outside this fixture; no replacement decision or gameplay body is supplied.
 path=root/'src/melee/ft/kinds/ftCommon/ftCo_Damage.c';source=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 previous=subprocess.run(['git','show',REFERENCE+':src/melee/ft/kinds/ftCommon/ftCo_Damage.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 headers.update(re.findall(r'^#include .+$',source,re.M))
 for text,name,destination in [(source,'actual_heavy_damage_branch',actual),(previous,'reference_heavy_damage_branch',reference)]:
  start=text.index('#ifdef MELEE_NATIVE\n        } else if (fp->item_gobj',text.index('void ftCo_8008EC90'))
  end=text.index(' else if (!ftCo_8009F0F0',start)
  branch=text[start:end].replace('} else if','if',1)
  destination.append('void '+name+'(Fighter_GObj* gobj){\nFighter* fp=GET_FIGHTER(gobj);\n'
    'const uint32_t callback_generation=(uint32_t)fp->x8_spawnNum;\n'
    'Fighter_GObj* retained_victim=fp->victim_gobj;uint32_t victim_generation;\n'
    'bool victim_live=nativeCallbackSource_Snapshot(retained_victim,NativeCallbackSource_Fighter,&victim_generation);\n'
    'const NativeFighterDefinition* authored=nativeFighterRuntime_Definition(fp->kind);\n'+branch+
    '\nrecord(28,gobj,0,0,0);\n}\n')
 declarations=[s[:s.index('{')].strip()+';' for s in actual+reference]
 (out/'kernels.h').write_text('\n'.join(sorted(headers))+'\nvoid fn_800D8BFC(Fighter_GObj*);\nvoid record(int,HSD_GObj*,int,float,float);\n'+'\n'.join(declarations)+'\n')
 (out/'kernels.c').write_text('#include "kernels.h"\n'+'\n'.join(actual+reference)+'\n')
 (out/'selection.json').write_text(__import__('json').dumps(SELECTION,indent=2)+'\n')
 return hashes
