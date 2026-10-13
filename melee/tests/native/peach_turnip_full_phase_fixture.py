"""Extract complete Turnip phases and real scale/stage response from a pinned SDK."""
from pathlib import Path
import argparse,hashlib,json,re
from peach_turnip_composition_fixture import body

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--generated',type=Path,required=True);p.add_argument('--script',type=Path,required=True);p.add_argument('--parameters',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    sources={
      'src/melee/it/kinds/itpeachturnip.c':['decrease_lifetimer','itPeachturnip_UnkMotion4_Anim','itPeachturnip_UnkMotion4_Phys','itPeachturnip_UnkMotion1_Anim','itPeachturnip_UnkMotion1_Phys','itPeachturnip_UnkMotion3_Anim','itPeachturnip_UnkMotion3_Phys','itPeachTurnip_Logic56_Thrown','itPeachturnip_UnkMotion3_Coll','itPeachTurnip_Logic56_Dropped','itPeachTurnip_Logic56_DmgDealt','itPeachTurnip_Logic56_Clanked','itPeachTurnip_Logic56_Reflected','itPeachTurnip_Logic56_HitShield','itPeachTurnip_Logic56_ShieldBounced','itPeachTurnip_Logic56_EvtUnk','itPeachTurnip_Logic56_Destroyed'],
      'src/melee/it/it_2725.c':['it_80272860','it_80273030','it_802751D8','it_80274484'],
      'src/melee/it/itmaplib.c':['it_80276FC4'],
      'src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c':['ftPe_SpecialLw_UnsetVeg'],
    }
    blocks=[];receipts={}
    for path,names in sources.items():
      text=(a.root/path).read_text();receipts[path]={'sha256':hashlib.sha256((a.root/path).read_bytes()).hexdigest(),'bodies':{}}
      for name in names:
        b=body(text,name);receipts[path]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()};blocks.append('// Actual '+path+'\n'+b)
    (a.output/'turnip_full_phase_actual.inc').write_text('\n\n'.join(blocks)+'\n')
    generated=a.generated.read_text()
    schema=re.search(r'schema != (0x[0-9a-fA-F]+ULL)',generated)
    values=re.findall(r'^    parameter int (\w+) = (-?\d+)\s*$',a.script.read_text(),re.M)
    if not schema or values!=[('surface_response_mask','14'),('retire_contact_mask','3')]:raise ValueError('unexpected phase parameter schema')
    imported=[]
    for line in a.parameters.read_text().splitlines():
      kind,name,equals,value=line.split()
      if kind!='int' or equals!='=':raise ValueError('unexpected imported mask parameter type')
      imported.append((name,int(value)))
    expected=[('It_Kind_Peach_Turnip.'+name,int(value)) for name,value in values]
    if imported!=expected:raise ValueError('imported mask parameters differ from owned declarations')
    (a.output/'full_phase_parameters.inc').write_text('static const uint64_t phase_parameter_schema = '+schema.group(1)+';\nstatic const uint32_t phase_parameters[] = { '+','.join(str(value) for name,value in imported)+' };\n')
    (a.output/'full_phase_source.json').write_text(json.dumps({'body_count':len(blocks),'sources':receipts,'parameters':imported,'input_sha256':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in [a.script,a.parameters,a.generated]},'scope':'complete Turnip phase comparison; controlled geometry/stage helper dependencies; constructor/pickup/full resource cleanup excluded'},indent=2)+'\n')
if __name__=='__main__':main()
