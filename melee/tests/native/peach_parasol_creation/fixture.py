"""Extract complete producer, constructor, attachment and cleanup dependencies."""
from pathlib import Path
import argparse,hashlib,json,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from peach_turnip_composition_fixture import body

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    groups={
      'src/melee/ft/kinds/ftPeach/ftpeach.c':['ftPe_Init_OnDeath2'],
      'src/melee/ft/kinds/ftPeach/ftpeachspecialhi.c':['ftPe_8011D518','ftPe_8011D598','ensureUnkItem','ftPe_SpecialHi_8011D620','ftPe_SpecialHi_8011D650','ftPe_SpecialHi_8011D424'],
      'src/melee/ft/kinds/ftPeach/ftpeachspecialn.c':['onExitHitlag','ftPe_SpecialN_DoDeath2','ftPe_SpecialN_OnDeath2'],
      'src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c':['ftPe_SpecialLw_UnsetVeg','ftPe_SpecialLw_8011CFA0'],
      'src/melee/it/kinds/itpeachparasol.c':['itPeachParasol_Logic60_Destroyed','it_802BDB94','it_802BDA64','it_802BDD3C','it_802BDDB4','itPeachParasol_Logic60_PickedUp','it_802BDBF8','it_802BDC18'],
      'src/melee/it/kinds/itpeachtoad.c':['itpeachtoad_inline_1','itPeachToad_Logic91_Destroyed','it_802BDF40','it_802BDFC0'],
      'src/melee/it/kinds/itpeachturnip.c':['it_802BD45C','itPeachTurnip_Logic56_Destroyed'],
      'src/melee/it/it_26B1.c':['it_8026B724','it_8026B73C','it_8026BB20','it_8026BB44','it_8026B3A8'],
      'src/melee/it/it_2725.c':['it_80272A18','it_80272A3C','it_80273168','it_802742F4'],
      'src/melee/it/item.c':['Item_8026A8EC','Item_8026849C','Item_8026B074','Item_8026AB54','Item_802674AC','Item_80268B18','Item_CreateAuthored','Item_802694CC'],
      'src/melee/ft/ftcommon.c':['ftCommon_8007E6DC'],
      'src/melee/ft/kinds/ftCommon/ftpickupitem.c':['ftpickupitem_80094818'],
      'src/melee/it/native_article.c':['nativeArticle_Creator','article_logic','nativeArticle_Create'],
      'src/melee/ft/native_fighter_module.c':['article_signal_author','signal_article'],
      'src/melee/db/dbitem.c':['db_80225DD8'],
      'src/melee/lb/lb_00B0.c':['lb_8000B804'],
      'src/melee/it/kinds/inlines.h':['Item_InitSpawn','Item_AttachToParent'],
    }
    blocks=[];receipts={}
    ordered=sorted(groups.items(), key=lambda entry: 0 if entry[0].endswith('kinds/inlines.h') else 1)
    for rel,names in ordered:
        path=a.root/rel;text=path.read_text();receipts[rel]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bodies':{}}
        for name in names:
            b=body(text,name);blocks.append('// Actual '+rel+'\n'+b)
            receipts[rel]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()}
    (a.output/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    (a.output/'source.json').write_text(json.dumps({'body_count':len(blocks),'sources':receipts,'scope':'complete Parasol producer and constructor, controlled allocation and model/state internals'},indent=2)+'\n')
if __name__=='__main__':main()
