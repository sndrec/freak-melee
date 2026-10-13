"""Extract exact shared jump/IASA entry bodies with current source barriers."""
import hashlib,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('jump_extract',HERE.parent/'script_item_scheduler/extract_scheduler.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
def extract(root,out):
    paths={
        'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c':['ft_SetVec','ft_800CB6EC','ft_did_jump','ftCo_800CB870','ftCo_JumpAerial_CheckInput','ftCo_JumpAerial_Begin','ftCo_JumpAerial_Enter_Basic','ftCo_JumpAerial_IASA'],
        'src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c':['ftCo_800D74A4_inline','ftCo_800D730C','ftCo_800D74A4'],
        'src/melee/ft/ftchangeparam.c':['ftCo_800D0EC8']}
    bodies=[];inputs={}
    for path,names in paths.items():
        file=root/path;inputs[str(file)]=hashlib.sha256(file.read_bytes()).hexdigest()
        source=helper.native_only(file.read_text())
        for name in names:bodies.append(helper.reference.body(source,name))
    prototypes=[body[:body.index('{')].strip()+';' for body in bodies]
    (out/'jump_actual.h').write_text('\n'.join(prototypes)+'\n'+'\n\n'.join(bodies)+'\n')
    return inputs
