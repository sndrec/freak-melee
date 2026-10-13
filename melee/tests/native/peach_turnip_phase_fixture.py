"""Extract real Turnip phases and shared scalar math from a pinned SDK."""
from pathlib import Path
import argparse,hashlib,json
from peach_turnip_composition_fixture import body

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    sources={
      'src/melee/it/kinds/itpeachturnip.c':['decrease_lifetimer','itPeachturnip_UnkMotion4_Anim','itPeachturnip_UnkMotion4_Phys','itPeachturnip_UnkMotion1_Anim','itPeachturnip_UnkMotion1_Phys','itPeachturnip_UnkMotion3_Anim','itPeachturnip_UnkMotion3_Phys','itPeachTurnip_Logic56_Dropped','itPeachTurnip_Logic56_DmgDealt','itPeachTurnip_Logic56_Clanked','itPeachTurnip_Logic56_Reflected','itPeachTurnip_Logic56_HitShield','itPeachTurnip_Logic56_ShieldBounced','itPeachTurnip_Logic56_EvtUnk','itPeachTurnip_Logic56_Destroyed'],
      'src/melee/it/it_2725.c':['it_80272860','it_80273030','it_802751D8'],
      'src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c':['ftPe_SpecialLw_UnsetVeg'],
    }
    blocks=[];receipts={}
    for path,names in sources.items():
      text=(a.root/path).read_text();receipts[path]={'sha256':hashlib.sha256((a.root/path).read_bytes()).hexdigest(),'bodies':{}}
      for name in names:
        b=body(text,name);receipts[path]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()};blocks.append('// Actual '+path+'\n'+b)
    (a.output/'turnip_phase_actual.inc').write_text('\n\n'.join(blocks)+'\n')
    (a.output/'phase_source.json').write_text(json.dumps({'body_count':len(blocks),'sources':receipts,'missing_phase_contracts':['throw-scale read/absolute instance restoration','masked stage response']},indent=2)+'\n')
if __name__=='__main__':main()
