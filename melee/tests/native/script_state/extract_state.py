"""Actual common state/collision bodies, isolated fixture generation only."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import importlib.util
reference_path=Path(__file__).resolve().parents[1]/'script_kinetics/extract_primitives.py'
spec=importlib.util.spec_from_file_location('kinetic_fixture_extract',reference_path)
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
ROOT=Path(__file__).resolve().parents[3]
FUNCTIONS={
    'src/melee/ft/kinds/ftCommon/ftCo_Fall.c':['ftCo_Fall_inline','ftCo_Fall_Enter'],
    'src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c':['ftCo_FallAerial_Enter'],
    'src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c':['inline0','ftCo_80096900','ftCo_800969D8'],
    'src/melee/ft/kinds/ftCommon/ftCo_Landing.c':['ftCo_Landing_Enter','ftCo_Landing_Enter_Basic','ftCo_LandingFallSpecial_Enter'],
    'src/melee/ft/kinds/ftCommon/ftCo_LandingAir.c':['ftCo_LandingAir_EnterWithMsidLag'],
    'src/melee/ft/ftcollision_ground.c':['ft_80082708','ft_800827A0','ft_80082838','ft_80082888','ft_80082978'],
    'src/melee/ft/ft_081B.c':['ft_80081D0C','ft_800824A0'],
}
def extract(root,output):
    pieces=[];inputs={};names=[]
    for relative,functions in FUNCTIONS.items():
        path=root/relative;text=path.read_text();inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
        # Select the actual native branch generically; hook guards can appear
        # between the directive and switch, so an exact string patch is unsafe.
        text=reference.native_only(text)
        for name in functions:
            piece=reference.body(text,name)
            if relative.endswith('ftCo_FallSpecial.c'):piece=piece.replace('inline0','scriptStateFixture_FreefallCore')
            pieces.append(piece);names.append(name)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('#include <melee/ft/native_script_fighter.h>\n#include <melee/ft/native_fighter_lifetime.h>\n#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n'+
        '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
    return {'inputs':inputs,'functions':names,'extracted_sha256':hashlib.sha256(output.read_bytes()).hexdigest()}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source-root',type=Path,default=ROOT);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();print(json.dumps(extract(args.source_root.resolve(),args.output.resolve())))
if __name__=='__main__':main()
