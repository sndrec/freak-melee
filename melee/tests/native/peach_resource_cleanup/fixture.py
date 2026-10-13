"""Extract cleanup bodies and the precise pre-construction held-item slice."""
from pathlib import Path
import argparse,hashlib,json,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from peach_turnip_composition_fixture import body

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    groups={
      'src/melee/ft/kinds/ftPeach/ftpeach.c':['ftPe_Init_OnDeath2'],
      'src/melee/ft/kinds/ftPeach/ftpeachspecialhi.c':['ftPe_8011D518','ftPe_8011D598'],
      'src/melee/ft/kinds/ftPeach/ftpeachspecialn.c':['onExitHitlag','ftPe_SpecialN_DoDeath2','ftPe_SpecialN_OnDeath2'],
      'src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c':['ftPe_SpecialLw_UnsetVeg','ftPe_SpecialLw_8011CFA0'],
      'src/melee/it/kinds/itpeachparasol.c':['itPeachParasol_Logic60_Destroyed','it_802BDB94'],
      'src/melee/it/kinds/itpeachtoad.c':['itpeachtoad_inline_1','itPeachToad_Logic91_Destroyed','it_802BDF40','it_802BDFC0'],
      'src/melee/it/kinds/itpeachturnip.c':['it_802BD45C','itPeachTurnip_Logic56_Destroyed'],
      'src/melee/it/it_26B1.c':['it_8026B724','it_8026B73C','it_8026BB20','it_8026BB44'],
      'src/melee/it/it_2725.c':['it_80272A18','it_80272A3C'],
      'src/melee/it/item.c':['Item_8026A8EC'],
      'src/melee/ft/ftcommon.c':['ftCommon_8007E6DC'],
      'src/melee/ft/kinds/ftCommon/ftpickupitem.c':['ftpickupitem_80094818'],
      'src/melee/it/native_article.c':['nativeArticle_Creator'],
      'src/melee/ft/native_fighter_module.c':['article_signal_author','signal_article'],
    }
    blocks=[];receipts={}
    for rel,names in groups.items():
        path=a.root/rel;text=path.read_text();receipts[rel]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bodies':{}}
        for name in names:
            b=body(text,name);blocks.append('// Actual '+rel+'\n'+b)
            receipts[rel]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()}
    rel='src/melee/ft/kinds/ftPeach/ftpeachspecialhi.c';text=(a.root/rel).read_text();parent=body(text,'ftPe_SpecialHi_8011D424')
    marker=parent.index('HSD_GObj* igobj = fp->item_gobj;');start=parent.rfind('{',0,marker);end=start+1;depth=1
    while depth:
        depth+=(parent[end]=='{')-(parent[end]=='}');end+=1
    section=parent[start:end]
    blocks.append('/* Exact held-item statement slice; outer request gates, world-position\n * read and constructor/tail are not part of this comparison. */\nstatic void actual_save_held_slice(HSD_GObj* gobj) {\n    Fighter* fp=GET_FIGHTER(gobj);\n'+section+'\n}')
    receipts[rel]['held_statement_slice']={'parent_body_sha256':hashlib.sha256(parent.encode()).hexdigest(),'slice_sha256':hashlib.sha256(section.encode()).hexdigest(),'line':text[:text.index(section)].count('\n')+1}
    (a.output/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    (a.output/'source.json').write_text(json.dumps({'body_count':len(blocks)-1,'statement_slice_count':1,'sources':receipts,'scope':'current-holder Parasol restoration, ordered resources and live saved ordinary-item owners; successful producer constructor excluded'},indent=2)+'\n')
if __name__=='__main__':main()
