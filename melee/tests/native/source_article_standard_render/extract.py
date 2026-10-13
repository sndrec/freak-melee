import importlib.util,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('factory',HERE.parent/'script_item_spawn/extract.py');factory=importlib.util.module_from_spec(spec);spec.loader.exec_module(factory)
def extract(root,out):
 root=Path(root);out=Path(out);hashes=factory.extract(root,out)
 boundary=out/'boundary.c';s=boundary.read_text()
 s=s.replace(factory.body(s,'HSD_JObjDispAll'),'')
 if 'void it_8026EECC(' in s:s=s.replace(factory.body(s,'it_8026EECC'),'')
 s=s.replace('#include "spawn_leaves.h"','#include "spawn_leaves.h"\n#include "render_leaves.h"')
 boundary.write_text(s)
 leaves=(HERE.parent/'script_item_spawn/spawn_leaves.h').read_text()
 leaves=leaves.replace(factory.body(leaves,'nativeItemBehavior_Select'),'')
 leaves=leaves.replace(factory.body(leaves,'Item_802680CC'),'')
 leaves=leaves.replace('spawn_stage=value;', 'spawn_stage=value;if(value==StageConstruct&&render_replace_program)render_swapped=true;')
 leaves='extern bool render_replace_program,render_swapped;\n'+leaves
 (out/'spawn_leaves.h').write_text(leaves)
 behavior=root/'src/melee/it/native_item_behavior.c';item=root/'src/melee/it/item.c';draw=root/'src/melee/it/itdraw.c'
 pieces=[factory.body(behavior.read_text(),'nativeItemBehavior_Select'),factory.body(item.read_text(),'Item_802680CC')]
 names=['it_8026EB18','it_8026EECC_inline_1','it_8026EECC_inline_2','it_8026EECC_inline_3','it_8026EECC_inline_0','it_8026EECC_inline_sw','it_8026EECC']
 pieces += [factory.body(draw.read_text(),n) for n in names]
 (out/'render_bodies.inc').write_text('\n'.join(pieces)+'\n')
 hashes.update({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [behavior,item,draw,HERE/'extract.py',HERE/'render_leaves.h']})
 return hashes
