"""Exact Rook Item declarations through actual metadata/VM/native protocol."""
import argparse,hashlib,json,subprocess,importlib.util,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
hold=module(HERE.parent/'script_holdable_protocol/extract_holdable.py','hold')
scheduler=module(HERE.parent/'script_item_scheduler/extract_scheduler.py','scheduler')
body=scheduler.reference.body

def extract(root,out):
 out.mkdir(parents=True,exist_ok=True);hashes=hold.extract(root,out/'actual_scheduler.h')
 original=(out/'actual_scheduler.h').read_text();scheduler.extract(root,out/'full_scheduler.h');full=(out/'full_scheduler.h').read_text()
 existing={'HSD_JObjSetScaleItem','HSD_JObjSetFacingDirItem','itemCallbackCurrent','itemStateSetupCurrent','Item_80268E5C','Item_8026A8EC','Item_OnUserDataRemove','Item_IsGrabbable'}
 # Constructor and full native scheduler are actual bodies; unrelated native
 # engine leaves stay at the shared fixture's explicit controlled boundary.
 names=scheduler.FUNCTIONS['src/melee/it/item.c'];extra=[body(full,n) for n in names if n not in existing]
 (out/'actual_scheduler.h').write_text(original+'\n'+'\n'.join(x[:x.index('{')].rstrip()+';' for x in extra)+'\n'+'\n\n'.join(extra)+'\n')
 path=root/'src/melee/it/item.c';native=scheduler.native_only(path.read_text());extra=[body(native,n) for n in ['Item_802674AC','Item_CreateEngine','Item_CreateAuthored']]
 with (out/'actual_scheduler.h').open('a') as file:file.write('\n'+'\n'.join(x[:x.index('{')].rstrip()+';' for x in extra)+'\n'+'\n\n'.join(extra)+'\n')
 with (out/'actual_scheduler.h').open('a') as file:file.write('\n'+body(scheduler.native_only((root/'src/melee/it/it_26B1.c').read_text()),'it_8026C368')+'\n')
 boundary=root/'tests/native/script_holdable_protocol/boundary.c';text=boundary.read_text()
 for name in ['nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindings_Context','nativeScriptBindings_SourceAlive','nativeFighterRuntime_Definition','Item_80267978','nativeItemBehavior_Select','nativeItem_CaptureCreator','nativeItem_Creator']:
  text=text.replace(body(text,name),'')
 for name in ['nativeScriptBindingItem_Read','nativeScriptBindingItem_Write','nativeScriptBindingItem_Invoke','nativeScriptBindingSpawn_Read','nativeScriptBindingSpawn_Invoke']:
  text=re.sub(r'\b(?:READ|WRITE|CALL)\('+name+r'\)','',text)
 # Empty native food policy is a controlled common-asset boundary, never an
 # authored surrogate. Actual Food factory +public constructor still execute.
 text+='\nstatic ItemLogicTable food_logic;\nconst NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){(void)k;return NULL;}\n'
 text+='\nvoid Item_80267978(HSD_GObj* o){Item* i=o->user_data;i->xC4_article_data=(Article*)scheduler_spec->article;i->xB8_itemLogicTable=&food_logic;i->xBC_itemStateContainer=NULL;}\n'
 text+='\nbool nativeItemBehavior_Select(Item_GObj* o,const SpawnItem* spawn,NativeItemBehaviorSelection s,FighterKind author,uint32_t owner){(void)spawn;if(s==NativeItemBehavior_Engine)return true;const NativeScriptItemProgram* p=nativeScriptItem_Program(author,owner);if(!p)return false;Item* i=o->user_data;i->xC4_article_data=(Article*)p->article;i->native_article_author_kind=author;i->native_script_owner=owner;i->xB8_itemLogicTable=&food_logic;i->xBC_itemStateContainer=NULL;i->xDD4_itemVar.native_holdable.owner_kind=author;i->xDD4_itemVar.native_holdable.definition_index=(u16)p->recipe_index;return nativeScriptItem_Attach(o,p);}\n'
 creator=root/'src/melee/it/native_item_owner.c';creator_text=creator.read_text();text+='\n'+body(creator_text,'nativeItem_CaptureCreator')+'\n'+body(creator_text,'nativeItem_Creator')+'\n';hashes[str(creator)]=hashlib.sha256(creator.read_bytes()).hexdigest()
 text+='\nvoid rook_animation(HSD_GObj* o){Item_80269528(o);}\nvoid rook_hit(HSD_GObj* o){processSourceCallback(o,NativeScriptItemHook_DamageDealt);}\nvoid rook_collision(HSD_GObj* o){Item_80269978(o);}\n'
 (out/'boundary.c').write_text(text);hashes[str(boundary)]=hashlib.sha256(boundary.read_bytes()).hexdigest();return hashes

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extract(ROOT,out);compiler=a.compiler.resolve();source=ROOT/'examples/rook_workshop/moves/005_NeutralSpecial.fms'
 text=source.read_text();(out/'items.fms').write_text('fighter:\n    behavior independent\n'+text[text.index('item ChuftyCookie'):])
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_spawn','binding_spawn_resolve','binding_effect_resolve','binding_properties','binding_item','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_holdable_protocol','native_script_item_spawn']]+[ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_jump.c',HERE.parent/'script_bindings_unavailable_ledge.c',HERE.parent/'script_bindings_unavailable_forms.c',HERE.parent/'script_bindings_unavailable_feedback.c',HERE/'test.c',HERE/'unavailable.c',out/'boundary.c']
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',source,compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list((HERE.parent/'script_holdable_protocol').glob('*.h'))+list((ROOT/'examples/rook_workshop/items').glob('*.png'))
 hashes.update({str(p):sha(p) for p in inputs});commands=[]
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE.parent/'script_holdable_protocol')]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 for stage,cmd in [('compile-runtime',[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe'),str(out/'items.fms'),str(ROOT/'examples/rook_workshop/items')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-10000:])
 if any(sha(Path(p))!=v for p,v in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['exact actual9 Rook Item declaration suffix, real metadata/compiler/VM/catalog properties/Item owner/shared protocol/native scheduler/GObj lifecycle','actual source Food factory/public native constructor; common food assets/empty policy and imported model/graphics/geometry/platform leaves controlled'],'limits':['Fighter pull/native inventory hookup covered separately by factory/live tests','no real blueprint import/decoded renderer/scene geometry or gameplay acceptance','synthetic common attrs; actual Rook icons borrowed solely for immutable metadata preparation']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
