"""Exact Item teardown and ordinary unload, with factory provenance prefix."""
import hashlib, importlib.util, json

def extract(root,out):
 helper=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('constructor_items_body',helper)
 module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 hashes={str(helper):hashlib.sha256(helper.read_bytes()).hexdigest()}
 def read(path):
  file=root/path;hashes[str(file)]=hashlib.sha256(file.read_bytes()).hexdigest();return file.read_text()
 source=read('src/melee/it/item.c')
 names=['DestroyItemInline','ItemSwitch','func_8026A8EC_inline1','func_8026A8EC_inline3','Item_8026A8EC','Item_OnUserDataRemove']
 pieces=[module.body(source,name) for name in names]
 native_bodies={name:hashlib.sha256(module.body(source,name).encode()).hexdigest() for name in names}
 factory=module.body(source,'Item_80267AA8')
 factory=factory[:factory.index('    if (!nativeItemBehavior_Select')]
 native_bodies['Item_80267AA8_ownership_admission_prefix']=hashlib.sha256(factory.encode()).hexdigest()
 # The native ownership/admission prefix is copied literally. Later model,
 # attribute/ECB/behavior construction is outside this focused boundary.
 factory=factory.replace('Item_80267AA8','fixture_factory_admission',1)+'#endif\n    return true;\n}'
 pieces.append(factory)
 source=read('src/melee/it/it_26B1.c')
 pieces.append(module.body(source,'RunCallbackUnk'))
 first=source.index('typedef struct ItemInteractionReference')
 last=source.index('/// Remove item from player on death?',first)
 pieces.append(source[first:last].replace('#endif\n#endif\n','#endif\n',1))
 pieces.append(module.body(source,'it_8026B7F8'))
 native_bodies['it_8026B7F8']=hashlib.sha256(module.body(source,'it_8026B7F8').encode()).hexdigest()
 source=read('src/melee/ft/fighter.c')
 pieces.append(module.body(source,'Fighter_Unload_8006DABC'))
 native_bodies['Fighter_Unload_8006DABC']=hashlib.sha256(module.body(source,'Fighter_Unload_8006DABC').encode()).hexdigest()
 out.joinpath('teardown.inc').write_text('\n\n'.join(pieces))
 out.joinpath('native_bodies.json').write_text(json.dumps(native_bodies,indent=2)+'\n')
 source=read('src/melee/ft/native_script_binding_properties.c')
 import re
 out.joinpath('source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','sleep_guard_error',module.body(source,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']))
 return hashes
