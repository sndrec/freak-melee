"""Actual catalog/guards/dispatch argument proof, reusable by other fixtures."""
import hashlib,re,importlib.util
from pathlib import Path

def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError('missing function '+name)
 at=text.index('{',m.start())+1;depth=1
 while depth:depth+=(text[at]=='{')-(text[at]=='}');at+=1
 return text[m.start():at]

def extract(root,out):
 root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True);hashes={};inputs={}
 for relative in ['src/melee/ft/native_script_bindings.c','src/melee/ft/native_script_binding_properties.c','src/melee/ft/ftparts_table.c','src/melee/ef/efsync.c','src/melee/ef/efalt.c','src/melee/ef/efasync.c']:
  path=root/relative;inputs[relative]=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 catalog=inputs['src/melee/ft/native_script_bindings.c'];start=catalog.index('#define F ');end=catalog.index('\n};',catalog.index('static const NativeScriptBindingDescriptor catalog'))+3
 properties=inputs['src/melee/ft/native_script_binding_properties.c'];parts=inputs['src/melee/ft/ftparts_table.c']
 guards=[body(properties,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 guards=[re.sub(r'\berror\b','form_guard_error',part) for part in guards]
 (out/'source_contract.inc').write_text(catalog[start:end]+'\n'+'#include <melee/ft/native_script_binding_item_resources.h>\n#include <melee/ft/native_script_binding_spawn.h>\n'+body(catalog,'nativeScriptBindings_Descriptor')+'\n'+'\n'.join(guards)+'\n'+body(parts,'ftParts_GetBoneIndex')+'\n')
 # Reuse the complete literal ABI proof; unknown forwarded payloads stay rejected.
 path=root/'tests/native/script_position_effect_abi/extract.py'
 spec=importlib.util.spec_from_file_location('effect_payload_proof',path)
 module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 extra,_=module.extract(root,out/'abi');hashes.update(extra);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 expected=(out/'abi/expected.inc').read_text().replace('expected[65536]','expected_arguments[65536]')
 (out/'dispatch_expected.inc').write_text(expected)
 (out/'dispatch_source_proof.json').write_bytes((out/'abi/proof.json').read_bytes())
 return hashes
