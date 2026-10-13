"""Original native insertion bodies and an explicit completed-installer observer."""
import hashlib,importlib.util,re
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 helper=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('graph_body',helper);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(helper)]=hashlib.sha256(helper.read_bytes()).hexdigest()
 def source(relative):
  p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 parts=source('src/melee/ft/ftparts.c');tree=source('src/melee/ft/ftanim_tree.c')
 bodies=[m.body(tree,'ftAnim_GetNextJointInTree'),m.body(parts,'ftParts_80075304'),m.body(parts,'ftParts_800753D4')]
 (out/'native_parts.inc').write_text('\n'.join(b[:b.index('{')].rstrip()+';' for b in bodies)+'\n'+'\n'.join(bodies)+'\n')
 installer=source('src/melee/ft/native_fighter_model_graph.c')
 if installer.count('    return true;')!=1:raise RuntimeError('Installer completion anchor changed')
 installer=installer.replace('    return true;','    graph_completed(gobj);\n    return true;')
 installer=re.sub(r'#include "([^"]+)"',r'#include <melee/ft/\1>',installer)
 (out/'native_installer.c').write_text('#include "fixture.h"\n'+installer)
 return hashes
