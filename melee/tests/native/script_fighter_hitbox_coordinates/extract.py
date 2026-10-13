"""Exact hitbox native references, property routing and selected-source guards."""
import hashlib,importlib.util,re,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('body',HERE.parent/'script_form_feedback/extract.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def extract(root,out):
 hashes={}
 def read(path):
  p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=read('src/melee/ft/native_script_binding_properties.c')
 guards='\n'.join(module.body(props,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 guards=re.sub(r'\berror\b','hitbox_guard_error',guards)
 boundary=read('tests/native/script_fighter_scheduler/boundary.c')
 boundary=boundary.replace(module.body(boundary,'nativeScriptBindings_SourceAlive'),'')
 for name in ['nativeScriptBindings_Read','nativeScriptBindings_Write']:
  actual=module.body(props,name)
  route=re.search(r'    if\(\(semantic>=NativeScriptBinding_HitboxCurrentX.*?;\n',actual,re.S)
  if not route:raise RuntimeError('missing actual coordinate property route')
  old=module.body(boundary,name)
  # Only the real coordinate branch is admitted here. Other scheduler property
  # leaves retain their explicitly controlled/rejecting boundary contracts.
  signature=actual[:actual.index('{')+1]
  fallback='return false;\n}'
  boundary=boundary.replace(old,signature+'\n'+route.group(0)+fallback)
 (out/'boundary.c').write_text('#include <melee/ft/native_script_binding_fighter_hitboxes.h>\n#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
 bodies=[]
 for path,names in [('src/melee/ft/ftchangeparam.c',['ftCo_CalcYScaledKnockback']),
                    ('src/melee/ft/ft_0DF0.c',['ftCo_800DEEB8']),
                    ('src/melee/ft/ft_0881.c',['ft_80089118','ft_80089228']),
                    ('src/melee/ft/ftcoll.c',['ftColl_8007ABD0','ftColl_8007B8A8','ftColl_8007AFF8']),
                    ('src/melee/lb/lbcollision.c',['lbColl_80008428','lbColl_80008434','lbColl_80008440']),
                    ('src/melee/ft/kinds/ftSeak/ftseakspecials.c',['ftSk_SpecialS_UpdateHitboxes','ftSk_SpecialS_ZeroHitboxPositions'])]:
  text=read(path);bodies.extend(module.body(text,n) for n in names)
 (out/'native_hitbox.inc').write_text('#include <melee/lb/lbcollision.h>\n'+'\n\n'.join(bodies)+'\n')
 source=read('examples/sheik_full.fms')
 def helper(name):
  match=re.search(r'^func '+name+r'\(.*?(?=^func |^move |^action |^article |\Z)',source,re.M|re.S)
  if not match:raise RuntimeError('missing source helper '+name)
  return match.group(0).rstrip()+'\n'
 script='fighter:\n    behavior independent\nstate int calls\nstate float chain_hit_x[4]\nstate float chain_hit_y[4]\nstate float chain_hit_z[4]\n'
 script+=helper('chain_zero_capsules')+helper('chain_endpoint')
 script+='func Index() -> int:\n    state.calls = state.calls + 1\n    return 1\nfunc WriteOnce(float value):\n    hitbox.current(Index()).x = value\nfunc ReadOnce() -> float:\n    return hitbox.previous(Index()).y\nfunc Invalid(int index):\n    hitbox.current(index).x = 4\n'
 (out/'coordinate_source.inc').write_text('static const char* script_source = '+json.dumps(script)+';\n')
 return hashes
