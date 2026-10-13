"""Complete source expressions through actual factories/actors/scheduler/VM."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);loaded=importlib.util.module_from_spec(spec);spec.loader.exec_module(loaded);return loaded
extractor=module(HERE.parent/'script_item_spawn/extract.py','spawn_extract')
def extract(root,out):
 hashes=extractor.extract(root,out,overrides=['nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindings_SourceAlive','nativeScriptBindings_Context'])
 scheduler=out/'actual_scheduler.h';text=scheduler.read_text();constructor=extractor.body(text,'Item_CreateAuthored');at=constructor.index('{')+1
 text=text.replace(constructor,constructor[:at]+'\n    if(spawnItem) positioned_spawn=*spawnItem;'+constructor[at:]);scheduler.write_text(text)
 leaves=root/'tests/native/script_item_spawn/spawn_leaves.h';source=leaves.read_text()
 source=source.replace('(void)spawn;if(selection==NativeItemBehavior_Engine)', 'spawn_collision_flag=spawn->x44_flag.b0;if(selection==NativeItemBehavior_Engine)')
 source=source.replace(extractor.body(source,'lb_8000B1CC'),'void lb_8000B1CC(HSD_JObj* joint,Vec3* input,Vec3* output){projectile_joint=joint;projectile_offset=input?*input:(Vec3){0};*output=joint->translate;if(input){output->x+=input->x;output->y+=input->y;output->z+=input->z;}}')
 source=source.replace(extractor.body(source,'it_80273670'),'void it_80273670(HSD_GObj* object,int anim,float frame){(void)object;projectile_appearance=anim;projectile_variant=frame;stage(StageAppearance);}')
 source='extern unsigned spawn_collision_flag;\nextern SpawnItem positioned_spawn;\nextern Vec3 projectile_offset;extern HSD_JObj* projectile_joint;extern int projectile_appearance;extern float projectile_variant;\n'+source
 names=['nativeItem_CaptureCreator','nativeItem_Creator','it_8026B724','it_8026B73C','it_80274574','it_8026B894','ftParts_GetBoneIndex']
 for name in names:source=source.replace(extractor.body(source,name),'')
 actual=[]
 for relative,names in [('src/melee/it/native_item_owner.c',['nativeItem_CaptureCreator','nativeItem_Creator']),
  ('src/melee/it/it_26B1.c',['it_8026B724','it_8026B73C','it_8026B894']),
  ('src/melee/it/it_2725.c',['HSD_JObjSetScale_2','it_80274594','it_80274574']),('src/melee/ft/ftparts_table.c',['ftParts_GetBoneIndex'])]:
  path=root/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  actual += [extractor.body(text,n) for n in names]
 (out/'spawn_leaves.h').write_text(source+'\nHSD_JObjInfo hsdJObj;\nbool hsdObjIsDescendantOf(HSD_Obj* object,HSD_ClassInfo* info){(void)object;(void)info;abort();}\nvoid it_8027429C(Item_GObj* object,Vec3* velocity){(void)object;(void)velocity;abort();}\nvoid (HSD_JObjSetMtxDirty)(HSD_JObj* o){o->flags|=JOBJ_MTX_DIRTY;}\nfloat ftLib_80086A0C(HSD_GObj* o){(void)o;return 1;}\nvoid it_80275534(HSD_GObj* o, float v){(void)o;(void)v;}\nvoid it_80274DFC(HSD_GObj* o){(void)o;}\nvoid it_80274E44(HSD_GObj* o){(void)o;}\n'+'\n\n'.join(actual)+'\n')
 hashes[str(leaves)]=hashlib.sha256(leaves.read_bytes()).hexdigest();return hashes

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extract(ROOT,out);compiler=a.compiler
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog','binding_spawn','binding_effect_resolve','binding_spawn_resolve','binding_properties','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','actor_access','receiver','binding_receiver','fighter_actor']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/ft/native_script_binding_signal_resolve.c', ROOT/'tests/native/script_bindings_unavailable_signal.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['feedback','jump','forms','ledge', 'hitlag', 'stage', 'family']]
 units += [ROOT/'src/melee/ft/native_script_binding_article_reference_resolve.c']
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['cpu_control','targets','item_model','article_combat','world_feedback']]
 units += [ROOT/'src/melee/ft/native_script_binding_article_motion.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['vectors','article_reference','article_lifecycle']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_script_item_spawn']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE/'test.c',out/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['colors','fighter_items','normal_attacks']]
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['audio','model_graph','item_collision','item_terrain','capture','collision','item_animation','item_lifecycle','ground_controls','item_observations','fighter_hitboxes','guard','air_inputs','air_physics','fighter_observations','contact','absorber','held_state','held_items']]
 units += [ROOT/'src/melee/ft/native_source_motion_policy.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
 units += [ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_attachments.c']
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'probe.fms',HERE/'README.md',compiler]+list(HERE.glob('*.h'))+list(out.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(p):sha(p) for p in inputs});commands=[]
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE.parent/'script_item_spawn'),'-I'+str(HERE.parent/'script_item_scheduler')]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe'),str(HERE/'probe.fms')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-9000:])
 if any(sha(Path(p))!=v for p,v in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual complete typed source compiler/VM and native entity bindings','actual source factories/public constructors/scheduler/actor pools/creator/ownership/part mapping','manual immutable specs/assets and controlled model/graphics/audio/collision leaves'],'limits':['no game or package/render parity acceptance','recipient-qualified factories not supported','numeric handles scene-local; raw IDs cannot distinguish wrap/reuse']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
