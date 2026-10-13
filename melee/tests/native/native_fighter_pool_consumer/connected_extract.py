"""Unchanged actual constructor/reset/teardown bodies for the connected proof."""
import hashlib, re
from run import body

def extract(root,out):
 hashes={}
 def read(name):
  p=root/name;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 s=read('src/melee/ft/fighter.c')
 names=['Fighter_NewSpawn_80068E40','Fighter_UnkInitReset_80067C98','Fighter_UnkProcessDeath_80068354','Fighter_Create_Inline2','Fighter_Create','Fighter_Unload_8006DABC']
 out.joinpath('connected_bodies.inc').write_text('\n\n'.join(body(s,n) for n in names))
 s=read('src/melee/ef/eflib.c')
 out.joinpath('effects.inc').write_text(body(s,'efLib_DestroyAll'))
 parts=read('src/melee/ft/ftparts.c');lists=read('src/melee/ft/ft_0C88.c')
 out.joinpath('parts.inc').write_text(body(parts,'ftParts_80074E58')+'\n'+body(lists,'ftCo_800C884C'))
 s=read('src/melee/ft/native_script_binding_properties.c')
 out.joinpath('source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','constructor_guard_error',body(s,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']))
 return hashes
