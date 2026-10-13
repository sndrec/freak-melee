"""Exact native catch/animation/common bodies, with unchanged HEAD catch sampler."""
import hashlib,importlib.util,re,subprocess,json
from pathlib import Path
def extract(root,out):
 hashes={}
 def load(rel,name):
  path=root/rel;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
 m=load('tests/native/script_form_feedback/extract.py','body');n=load('tests/native/script_item_scheduler/extract_scheduler.py','native')
 def source(rel):
  path=root/rel;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return n.native_only(path.read_text())
 props=source('src/melee/ft/native_script_binding_properties.c')
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','ledge_guard_error',m.body(props,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 bodies=[]
 for file,names in [('ftcommon.c',['ftCommon_8007D780','ftCommon_8007D5D4','ftCommon_8007EFC0','_func_8007E2FC_inline','ftCommon_8007E2FC','ftCommon_8007E2F4']),('ftanim.c',['ftAnim_8006EBA4']),('ftcliffcommon.c',['ftCo_CliffCatch_Phys','ftCliffCommon_80081370','ftCliffCommon_80081298'])]:
  text=source('src/melee/ft/'+file);bodies += [m.body(text,name) for name in names]
 for relative,names in [('src/melee/ft/ft_081B.c',['ft_CheckGroundAndLedge']),('src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c',['ftCo_800CB870'])]:
  text=source(relative);bodies += [m.body(text,name) for name in names]
 (out/'native_ledge.inc').write_text('\n'.join(bodies)+'\n')
 rel='src/melee/ft/ftcliffcommon.c'
 r=subprocess.run(['git','show','24b898f1d:'+rel],cwd=root,capture_output=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
 if r.returncode:raise RuntimeError('Pinned catch source unavailable')
 text=n.native_only(r.stdout.decode());(out/'head_ftcliffcommon.c').write_text(text)
 body=re.sub(r'\bftCliffCommon_80081370\b','reference_ftCliffCommon_80081370',m.body(text,'ftCliffCommon_80081370'))
 (out/'reference_ledge.inc').write_text(body+'\n')
 (out/'reference_proof.json').write_text(json.dumps({'revision':'24b898f1d','sources':{rel:hashlib.sha256(r.stdout).hexdigest()}},indent=2)+'\n')
 return hashes
