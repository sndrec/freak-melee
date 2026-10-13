"""Extract actual generic item physics/lifetime/sound/collision bodies."""
import hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('item_reference',HERE.parent/'script_kinetics/extract_primitives.py');reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/it/it_2725.c':['it_80272860','it_80274658','it_80275158','it_802751D8','it_80273030','itColl_BounceOffShield'],
 'src/melee/it/it_26B1.c':['it_8026B724','it_8026B73C','it_8026B3A8'],
 'src/melee/it/item.c':['Item_8026AE84','Item_8026AF0C','Item_8026B034'],
 'src/melee/it/itgroundcoll.c':['it_8026D9A0'],
 'src/melee/it/itmaplib.c':['it_80276214','it_802762B0','it_802762BC','product_xy','sqrtf_accurate_local','return_sqrt_value','it_8027781C'],
 'src/melee/mp/mpcoll.c':['mpCollSetFacingDir'],
 'src/melee/lb/lbvector.c':['lbVector_NormalizeXY','lbVector_Add_xy','lbVector_Mirror'],
}
def extract(root,output):
 pieces=[];inputs={}
 for relative,names in FUNCTIONS.items():
  path=root/relative;text=path.read_text();inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in names:pieces.append(reference.body(text,name))
 output.write_text('// Actual generic engine bodies. Controlled leaves are documented in engine_fixture.c.\n#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n'+ '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
 return inputs
