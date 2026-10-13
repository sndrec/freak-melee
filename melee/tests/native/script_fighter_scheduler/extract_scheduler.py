"""Exact actual Fighter static/cue dispatch and attribute/CPU bodies."""
import hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('fighter_scheduler_body',HERE.parent/'script_item_scheduler/extract_scheduler.py');reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/ft/kinds/ftCommon/ftCo_Attack1.c':['onPkPc21EC','getMotionFlags','ftCo_Attack11_EnterMotion'],
 'src/melee/ft/kinds/ftCommon/ftCo_Wait.c':['ftCo_Wait_IASA'],
 'src/melee/ft/ft_0892.c':['ft_800898C0'],
 'src/melee/ft/ftcommon.c':['ftCommon_8007E6DC','ftCommon_8007E79C','ftCommon_8007E7E4','ftCommon_8007F578','ftCommon_8007F5CC','ftCommon_8007F824','ftCommon_8007F86C','ftCommon_8007F8E8','ftCommon_AuxiliaryItemAttach','ftCommon_8007F948','ftCommon_8007FE84','ftCommon_8007FA00','ftCommon_8007FA58','ftCommon_8007FC7C'],
 'src/melee/ft/kinds/ftCommon/ftpickupitem.c':['ftpickupitem_80094818'],
 'src/melee/ft/kinds/ftCommon/ftCo_Landing.c':['ftCo_Landing_Enter'],
 'src/melee/ft/ftchangeparam.c':['ftCo_800D105C'],
 'src/melee/ft/ftcpuattack.c':['ftCo_CpuClearTargetAndFinish','ftCo_CpuSetNeutralStick','ftCo_CpuFlickLstickX','ftCo_CpuPressAWithLstickX','ftCo_800B683C','ftCo_800B77E8'],
 'src/melee/ft/fighter.c':['Fighter_8006C80C']}
def extract(root,output):
 bodies=[];hashes={}
 for path,names in FUNCTIONS.items():
  file=root/path;hashes[str(file)]=hashlib.sha256(file.read_bytes()).hexdigest();source=reference.native_only(file.read_text())
  for name in names:bodies.append(reference.reference.body(source,name))
 prototypes=[body[:body.index('{')].rstrip()+';' for body in bodies]
 cpu=(root/'src/melee/ft/ftcpuattack.c').read_text();first=cpu.index('typedef struct ftCo_CollData');end=cpu.index('} ftCo_CollData;',first)+len('} ftCo_CollData;')
 output.write_text('// Exact actual scheduler bodies.\n'+cpu[first:end]+'\n'+ '\n'.join(prototypes)+'\n'+ '\n\n'.join(bodies)+'\n')
 return hashes
