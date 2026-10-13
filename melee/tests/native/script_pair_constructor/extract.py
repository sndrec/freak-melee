"""Extract complete Player construction policies and exact family/CPU selection."""
import hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('family_extract',HERE.parent/'script_family_queries/extract.py')
family=importlib.util.module_from_spec(spec);spec.loader.exec_module(family)
def extract(root,out):
 hashes=family.extract(root,out)
 path=root/'src/melee/pl/player.c';text=path.read_text()
 mapping=text[text.index('typedef struct _ftMapping {'):text.index('} ftMapping;')+len('} ftMapping;')]
 actor=text[text.index('typedef struct PlayerConstructorActor {'):text.index('} PlayerConstructorActor;')+len('} PlayerConstructorActor;')]
 names=['player_constructor_selection','player_constructor_capture','player_constructor_current','player_constructor_unwind','Player_80031AD0','Player_80031FB0']
 (out/'native_constructor.inc').write_text(mapping+'\nstatic ftMapping ftMapping_list[ChKind_RuntimeCapacity];\n'+actor+'\n'+'\n'.join(family.module.body(text,n) for n in names)+'\n')
 (out/'native_constructor_check.c').write_text('#include <sysdolphin/baselib/gobjplink.h>\n#include <melee/pl/player.h>\n#include <melee/ft/fighter.h>\n#include <melee/ft/native_fighter_pool.h>\n#include <melee/gm/native_home.h>\nstatic StaticPlayer player_slots[Gm_Player_NumMax];\nstatic void Player_CheckSlot(int slot){(void)slot;}\n#include \"native_constructor.inc\"\n')
 boundary=(out/'boundary.c').read_text()
 for name in ['nativeFighterPool_CanBeginScene','nativeFighterPool_BeginScene','nativeFighterPool_RetireScene']:
  boundary=boundary.replace(family.module.body(boundary,name),'')
 (out/'boundary.c').write_text(boundary)
 hashes[str(HERE.parent/'script_family_queries/extract.py')]=hashlib.sha256((HERE.parent/'script_family_queries/extract.py').read_bytes()).hexdigest()
 return hashes
