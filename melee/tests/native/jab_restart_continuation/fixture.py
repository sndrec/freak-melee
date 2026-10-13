"""Freeze the two real restart-policy decision bodies, SDK58 vs candidate."""
import argparse, hashlib, json, re, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'normal_attack_contracts'))
from source_fixture import extract

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('original-sdk','candidate-sdk','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);manifest={}
    for version,sdk in [('old',args.original_sdk),('new',args.candidate_sdk)]:
        path=sdk/'src/melee/ft/kinds/ftCommon/ftCo_Attack1.c';raw=path.read_bytes();text=raw.decode();bodies=[];details={}
        for name in ('doAttack12','doAttack13'):
            body=extract(text,name);details[name]={'sha256':hashlib.sha256(body.encode()).hexdigest(),'line':text[:text.index(body)].count('\n')+1}
            bodies.append(re.sub(r'\b'+name+r'\b',version+'_'+name,body))
        (args.output/(version+'.inc')).write_text('\n\n'.join(bodies)+'\n')
        manifest[version]={'path':str(path.resolve()),'sha256':hashlib.sha256(raw).hexdigest(),'bodies':details}
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if __name__=='__main__':main()
