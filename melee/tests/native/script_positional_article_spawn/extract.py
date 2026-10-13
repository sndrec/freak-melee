"""Reuse actual constructor/scheduler extraction; use real creator functions."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);p=root/'tests/native/script_item_spawn/extract.py'
 spec=importlib.util.spec_from_file_location('shared_spawn',p);shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
 hashes=shared.extract(root,out)
 boundary=out/'boundary.c';text=boundary.read_text()
 for stage in ['StageSpawned','StageCreated','StageLaunched','StagePickup']:
  text=text.replace('stage('+stage+');','positional_stage('+stage+',c->source);')
 boundary.write_text(text)
 scheduler=out/'actual_scheduler.h';text=scheduler.read_text();constructor=shared.body(text,'Item_CreateAuthored');at=constructor.index('{')+1
 text=text.replace(constructor,constructor[:at]+'\n    positional_spawn=*spawnItem;'+constructor[at:]);scheduler.write_text(text)
 owners=root/'src/melee/it/native_item_owner.c'
 (out/'actual_owner.inc').write_text('\n'.join(shared.body(owners.read_text(),n) for n in ['nativeItem_CaptureCreator','nativeItem_Creator'])+'\n')
 leaves=(root/'tests/native/script_item_spawn/spawn_leaves.h').read_text()
 for n in ['stage','nativeItem_CaptureCreator','nativeItem_Creator','nativeItemBehavior_Select']:
  leaves=leaves.replace(shared.body(leaves,n),'')
 leaves=leaves.replace('static ItemLogicTable spawn_food_logic;','static ItemLogicTable spawn_food_logic;\nextern bool positional_oom;\nextern SpawnItem positional_spawn,positional_initialized;\nvoid positional_stage(unsigned,HSD_GObj*);\nstatic void stage(unsigned value){spawn_stage=value;positional_stage(value,HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);}')
 leaves=leaves.replace('{(void)data;for(unsigned i=0;i<32;++i)if(!spawn_live[i])','{(void)data;if(positional_oom)return NULL;for(unsigned i=0;i<32;++i)if(!spawn_live[i])',1)
 # The selector/model leaf is controlled; actual constructor/Attach/VM and
 # actual creator identity functions execute. No native behavior table selects
 # source callbacks in this boundary.
 leaves += '''\n#include "actual_owner.inc"
bool nativeItemBehavior_Select(Item_GObj* object,const SpawnItem* spawn,NativeItemBehaviorSelection selection,FighterKind author,uint32_t owner)
{
    positional_initialized=*spawn;const NativeScriptItemProgram* p=nativeScriptItem_Program(author,owner);
    if(selection!=NativeItemBehavior_Author||!p||p->kind!=spawn->kind||!p->article)return false;
    scheduler_spec=p;Item* item=object->user_data;item->xC4_article_data=(Article*)p->article;
    item->native_article_author_kind=author;item->native_script_owner=owner;
    item->xB8_itemLogicTable=NULL;item->xBC_itemStateContainer=NULL;
    memset(&item->xDD4_itemVar.native_article,0,sizeof(item->xDD4_itemVar.native_article));
    return nativeScriptItem_Attach(object,p)&&!scheduler_fail_selector;
}
'''
 (out/'spawn_leaves.h').write_text(leaves)
 for path in [p,owners,root/'tests/native/script_item_spawn/spawn_leaves.h',root/'tests/native/script_item_spawn/spawn_stage.h',Path(__file__).resolve()]:hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 return hashes
