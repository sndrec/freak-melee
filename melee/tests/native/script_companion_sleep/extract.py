"""Mechanical source-exact Sleep/death bodies; construction has its own fixture."""
import hashlib,importlib.util,re

def extract(root,out):
 helper=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('sleep_body',helper);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes={str(helper):hashlib.sha256(helper.read_bytes()).hexdigest()}
 def read(name):
  path=root/name;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=read('src/melee/ft/native_script_binding_properties.c')
 out.joinpath('source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','sleep_guard_error',m.body(text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']))
 functions={'ftcolanim.c':['ftCo_800BFD04','ftCo_800BFD9C'],'ft_0D4D.c':['ftCo_800D4F24_inline','ftCo_800D4F24'],'ft_0D31.c':['ftCo_800D331C'],'ftlib.c':['ftLib_80087140']}
 pieces=[]
 for path,names in functions.items():
  source=read('src/melee/ft/'+path)
  pieces += [m.body(source,n) for n in names]
 out.joinpath('sleep_bodies.inc').write_text('\n\n'.join(pieces))
 return hashes
