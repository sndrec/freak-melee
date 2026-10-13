"""Actual source-alive admission, extracted without gameplay substitutes."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);path=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 p=root/'src/melee/ft/native_script_binding_properties.c';text=p.read_text()
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','observation_error',m.body(text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 source=root/'src/melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c'
 current=source.read_text();previous=subprocess.run(['git','show','45ac45dce:src/melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c'],cwd=root,capture_output=True,text=True,check=True).stdout
 (out/'prebatch_entry.c').write_text(previous,newline='\n')
 inline=m.body(current,'ft_800CEF08_inline1');entry=m.body(current,'ftCo_ItemParasolOpen_Enter')
 reference=m.body(previous,'ft_800CEF08_inline1')+'\n'+m.body(previous,'ftCo_ItemParasolOpen_Enter')
 reference=re.sub(r'\bft_800CEF08_inline1\b','reference_flags',reference)
 reference=re.sub(r'\bftCo_ItemParasolOpen_Enter\b','reference_enter',reference)
 (out/'entry.inc').write_text(inline+'\n'+entry+'\n'+reference+'\n',newline='\n')
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest(),str(path):hashlib.sha256(path.read_bytes()).hexdigest(),str(source):hashlib.sha256(source.read_bytes()).hexdigest(),str(out/'prebatch_entry.c'):hashlib.sha256((out/'prebatch_entry.c').read_bytes()).hexdigest()}
