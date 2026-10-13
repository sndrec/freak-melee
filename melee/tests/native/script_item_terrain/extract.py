"""Committed native kernels with explicit silent controlled engine leaves."""
import hashlib,importlib.util,json,re,subprocess
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 extractor=root/'tests/native/script_position_effect_abi/extract.py'
 spec=importlib.util.spec_from_file_location('terrain_body',extractor);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(extractor)]=hashlib.sha256(extractor.read_bytes()).hexdigest()
 def body(text,name):
  if name=='it_8026DD5C':
   start=text.index('bool it_8026DD5C(');return text[start:text.index('\n}',start)+2]
  return m.body(text,name)
 def read(rel):
  p=root/rel;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 def native(rel,names):
  text=read(rel);ref=subprocess.run(['git','show','50f2b476b:'+rel],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  bodies=[]
  for n in names:
   actual=body(text,n)
   if actual!=body(ref,n):raise RuntimeError('Native kernel changed '+n)
   bodies.append(actual)
  return bodies
 property_text=read('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','terrain_guard_error',m.body(property_text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 boundary=read('tests/native/script_target_queries/boundary.c')
 boundary=boundary.replace('#include "native_target.inc"','')
 boundary=boundary.replace(m.body(boundary,'nativeScriptBindings_Write'),'')
 boundary=boundary.replace('k==target_foreign_package.target?&target_foreign_package:NULL','NULL')
 (out/'boundary.c').write_text(boundary)
 bone=read('tests/native/script_target_bone_unavailable.c')
 bone=bone.replace('HSD_JObjInfo hsdJObj;','').replace(m.body(bone,'hsdObjIsDescendantOf'),'')
 (out/'bone_unavailable.c').write_text(bone)
 (out/'fixture.h').write_text('#include <melee/ft/native_source_package.h>\nextern NativeSourcePackage target_package;\n')
 setup=read('tests/native/script_target_queries/test.c');setup=setup[:setup.index('static void args(')]
 setup=setup.replace('#include "foreign_source.h"','').replace('static Item items[128]','static Item items[512]').replace('item_count<128','item_count<512')
 start=setup.index('    const char* source=');end=setup.index('    article=',start)
 source='fighter:\n    behavior independent\nfunc Effect(int id, float x, float y, float z, float scalar):\n    effects.spawn_with_scalar_at_position(id, x, y, z, scalar)\narticle Probe states 1:\n    state int left\n    state int landed\n    state int contact\n    state int air\n    state int completed\n    state int mode\n    state bool reset\n    state bool nested\n    on spawned:\n        item.follow_ground()\n        item.state.completed = item.state.completed + 1\n    on created:\n        item.bounce_to_ground(item.state.reset)\n        item.state.completed = item.state.completed + 1\n    on dropped:\n        item.slide_on_ground()\n        item.state.completed = item.state.completed + 1\n    on picked_up:\n        item.set_collision_facing(item.state.mode)\n        item.reset_default_ecb()\n        item.clear_animation_commands()\n        item.state.completed = item.state.completed + 1\n    on accessory:\n        item.bounce_to_ground()\n        item.state.completed = item.state.completed + 1\n    on thrown:\n        item.respond_to_stage(item.state.reset)\n        item.state.completed = item.state.completed + 1\n    on stage_contact:\n        item.state.contact = item.state.contact + 1\n        if item.state.mode == 1: velocity.x = 7\n        if item.state.mode == 2:\n            let int zero = 0\n            item.state.contact = 1 / zero\n    on stage_left:\n        item.state.left = item.state.left + 1\n        if item.state.nested:\n            item.state.nested = false\n            item.bounce_to_ground(true)\n        if item.state.mode == 1:\n            velocity.x = 7\n        if item.state.mode == 2:\n            let int zero = 0\n            item.state.left = 1 / zero\n    on stage_landed:\n        item.state.landed = item.state.landed + 1\n    on entered_air:\n        item.state.air = item.state.air + 1\n        velocity.x = 7\n    state 0 anim -1:\n'
 setup=setup[:start]+'    const char* source='+json.dumps(source)+';\n'+setup[end:]
 (out/'controlled_setup.inc').write_text(setup)
 bodies=native('src/melee/it/itgroundcoll.c',['it_8026E5A0','it_8026E71C','it_8026D62C','it_8026DC24','it_8026DD5C','it_8026DDFC','it_8026DE98','it_8026E_inline','it_8026E15C_inline1','it_8026E15C_inline2','it_8026E15C','it_8026E248_inline','it_8026DE98_outline','it_8026DE98_outline2','it_8026DE98_outline3','it_8026DE98_outline4','it_8026E248','it_8026E414','it_8026E8C4'])
 bodies+=native('src/melee/it/itmaplib.c',['it_80276214','it_802762B0','it_802762BC','it_80276308','it_802763E0','it_80275DFC','it_80275D5C','it_80276CEC','it_80277544'])
 bodies+=native('src/melee/mp/mpcoll.c',['mpColl_SetECBSource_Fixed','mpCollSetFacingDir'])
 bodies+=native('src/melee/it/item.c',['Item_80267454'])
 bodies+=native('src/melee/it/it_2725.c',['it_802725D4','it_80274658','it_80274740'])
 bodies+=native('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf'])
 bodies+=native('extern/dolphin/src/dolphin/mtx/vec.c',['C_VECAdd'])
 # This is the one intentionally corrected real cached-pointer continuation.
 item=read('src/melee/it/item.c');entered=m.body(item,'Item_8026ADC0')
 ref=subprocess.run(['git','show','50f2b476b:src/melee/it/item.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 previous=m.body(ref,'Item_8026ADC0')
 fixed=previous.replace('        bool bound=', '        NativeItemLifetime lifetime;\n        if(!nativeItemLifetime_Capture(gobj,&lifetime))return;\n        bool bound=')
 fixed=fixed.replace('NativeScriptVM_Ok)return;', 'NativeScriptVM_Ok||\n           !nativeItemLifetime_Current(&lifetime))return;')
 if entered!=fixed:raise RuntimeError('EnteredAir correction differs from its narrow checkpoint change')
 bodies.append(entered)
 (out/'native_ground.inc').write_text('\n'.join(bodies)+'\n')
 efasync=read('src/melee/ef/efasync.c');efsync=read('src/melee/ef/efsync.c')
 cases=m.cases(efasync,'efAsync_Dispatch',0x250,0x478)
 selected='case 0x3F1:\ncase 0x3F2:\n'+cases[0x3F1][1]
 selected+='case 0x4CF:\n'+m.cases(efsync,'efSync_Spawn',0x4BA,65536)[0x4CF][1]
 (out/'native_effect_cases.inc').write_text(selected)
 (out/'native_facing.inc').write_text('\n'.join(native('src/melee/ef/eflib.c',['efLib_CreateGenerator_Translate_FacingDir']))+'\n')
 for rel,fn in [('src/melee/ef/efasync.c','efAsync_Dispatch'),('src/melee/ef/efsync.c','efSync_Spawn')]:native(rel,[fn])
 return hashes
