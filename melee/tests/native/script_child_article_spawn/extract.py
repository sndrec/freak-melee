"""Actual positional constructor substrate plus exact full-word Fighter lookup."""
import hashlib,importlib.util
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);p=root/'tests/native/script_positional_article_spawn/extract.py'
 spec=importlib.util.spec_from_file_location('child_substrate',p);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 hashes=module.extract(root,out)
 shared=root/'tests/native/script_item_spawn/extract.py';s=importlib.util.spec_from_file_location('child_body',shared);body=importlib.util.module_from_spec(s);s.loader.exec_module(body)
 boundary=out/'boundary.c';text=boundary.read_text();old=body.body(text,'nativeScriptBindings_Read')
 text=text.replace(old,'bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* d){(void)selectors;(void)d;if(id!=NativeScriptBinding_ItemCreationValue||count)return false;float value;if(!nativeScriptItem_GetCreationValue(c->source,&value))return false;*result=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};return true;}');boundary.write_text(text)
 lookup=root/'src/melee/ft/ftlib.c';scheduler=out/'actual_scheduler.h';text=scheduler.read_text();original=body.body(text,'ftLib_8008741C');source=original;at=source.index('{')+1
 source=source[:at]+'\n    ++child_lookup_count;'+source[at:]
 scheduler.write_text('extern unsigned child_lookup_count;\n'+text.replace(original,source))
 for file in [p,lookup,Path(__file__).resolve()]:hashes[str(file)]=hashlib.sha256(file.read_bytes()).hexdigest()
 return hashes
