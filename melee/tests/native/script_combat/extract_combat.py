"""Extract actual damage/status/effect bodies and the historical RNG oracle."""
import hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('combat_reference',HERE.parent/'script_kinetics/extract_primitives.py')
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/ft/fighter.c':['Fighter_TakeDamage_8006CC7C'],
 'src/melee/ft/native_fighter_damage.c':['nativeFighterDamage_Self','nativeFighterDamage_Heal'],
 'src/melee/ft/ftcoll.c':['ftColl_8007B0C0','ftColl_8007B128','ftColl_HurtboxInit','ftColl_8007B62C','ftColl_8007B868'],
 'src/melee/ft/ftcommon.c':['ftCommon_8007DB24','ftCommon_UnlockECB'],
 'src/melee/ft/kinds/ftCommon/ftCo_Damage.c':['ftCo_ScaleBy154','ftCo_Damage_CalcKnockbackLevel','ftCo_8008E984'],
 'src/melee/it/item.c':['Item_8026AE60'],
 'src/melee/ef/eflib.c':['efLib_DestroyAll','efLib_PauseAll','efLib_ResumeAll'],
 'src/melee/ft/native_fighter_module.c':['random_int','random_int_excluding'],
}
def extract(root,output):
 pieces=[];inputs={};names=[]
 for relative,functions in FUNCTIONS.items():
  path=root/relative;source=path.read_text();inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in functions:
   body=reference.body(source,name)
   if name.startswith('random_int'):body=body.replace(name,'scriptCombatFixture_'+name,1).replace('static ','',1)
   pieces.append(body);names.append(name)
 output.write_text('// Actual engine bodies; controlled leaves in engine_fixture.c.\n#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n'+
  '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
 return {'inputs':inputs,'functions':names,'sha256':hashlib.sha256(output.read_bytes()).hexdigest()}
