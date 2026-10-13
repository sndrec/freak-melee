"""Actual owned helper callback binding/VM and shared native root setter."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];PRE=HERE.parent/'script_fighter_presentation_scheduler'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: required')
 out.mkdir(parents=True);ex=module('scheduler_extract',PRE/'extract.py');hashes=ex.extract(ROOT,out/'actual_presentation.h');body=ex.reference.reference.body
 reference_text=subprocess.run(['git','show','1b8671834:src/melee/ft/ftparts_rotation.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 reference=out/'root_y_reference.inc';reference.write_text(body(reference_text,'ftPartGetRotZ').replace('ftPartGetRotZ','FrozenRootRotationY'))
 original=PRE/'boundary.c';boundary=out/'boundary.c';text=original.read_text();text=text.replace(body(text,'nativeFighterRuntime_SourcePackage'),'').replace(body(text,'ftParts_GetBoneIndex'),'');text=text.replace(r'#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>','#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>');text=text.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;', 'if(!n&&(s==NativeScriptBinding_BonesRootRotationX||s==NativeScriptBinding_BonesRootRotationY))return nativeScriptBindingPresentation_Read(c,s,out,d);if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;');text='#include <melee/ft/native_script_binding_presentation.h>\n'+text;boundary.write_text(text)
 unavailable=out/'unavailable.c';unavailable.write_text((PRE/'unavailable.c').read_text().replace('CALL(nativeScriptBindingSetup_Invoke)',''))
 jobj=ROOT/'src/sysdolphin/baselib/jobj.c';extra=out/'joint_leaves.c';extra.write_text('#include <melee/ft/ftcharacter_assets.h>\n#include <sysdolphin/baselib/jobj.h>\n#include <melee/ft/ft_0877.h>\n#include <melee/ft/ft_0881.h>\n#include <melee/ft/ftanim.h>\n#include <melee/it/it_26B1.h>\n#include <melee/lb/lb_00B0.h>\n#include <stdlib.h>\n'+'\n'.join(body(jobj.read_text(),fn) for fn in ['HSD_JObjGetFlags','HSD_JObjClearFlags','HSD_JObjSetMtxDirtySub'])+'\n'+'''void ft_PlaySFX(Fighter*f,enum_t s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_800881D8(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088478(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088510(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_800885A8(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088640(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088328(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
bool itIsHeavy(Item_GObj*g){(void)g;abort();}
Item_HoldKinds itGetHoldKind(Item_GObj*g){(void)g;abort();}
void ftAnim_80070C48(Fighter_GObj*g,s32 v){(void)g;(void)v;abort();}
void ftAnim_800704F0(Fighter_GObj*g,int n,float v){(void)g;(void)n;(void)v;abort();}
void ftAnim_80070FB4(Fighter_GObj*g,s32 a,s32 b){(void)g;(void)a;(void)b;abort();}
void ftAnim_80070CC4(Fighter_GObj*g,int v){(void)g;(void)v;abort();}
''')
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_setup','binding_presentation']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft'/n for n in ['native_callback_lifetime.c','native_fighter_pose_path.c','native_fighter_pool.c','ftparts_table.c','ftparts_rotation.c','native_source_declarations.c','native_source_asset_validation.c','native_fighter_sound_policy.c','ftcharacter_asset_layout.c']]+[ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name!='script_bindings_unavailable_presentation.c')+[ROOT/'tests/native/script_receiver_unavailable_identity.c',HERE/'test.c',boundary,unavailable,extra]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c',ROOT/'src/melee/ft/native_source_motion_policy.c',HERE/'unselected.c']
 inputs=units+[reference,HERE/'root_y_cases.h',HERE/'run.py',HERE/'README.md',original,PRE/'unavailable.c',PRE/'boundary.h',PRE/'extract.py',jobj,ROOT/'src/melee/ft/ftcharacter_assets.c',ROOT/'examples/drmario_full.fms',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();hashes.update({str(f):sha(f) for f in inputs});runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(PRE),'-I'+str(out)];commands=[]
 for stage,cmd in [('runtime',[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 if any(sha(Path(f))!=h for f,h in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','result':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'root_y_reference':{'commit':'1b8671834','alteration':'function name only'},'scope':['real compiler/Setup/actor/VM/helper callback registration, clearing and snapshots','actual Presentation_Invoke and native ftPartSetRotX plus HSD dirty/flags bodies','actual Fighter/GObj pools and controlled owned source rig metadata'],'limits':['source rig/data views controlled; no original DAT/model/render acceptance','audio and unrelated primitive leaves reject explicitly','no full DrMario or gameplay claim']};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
