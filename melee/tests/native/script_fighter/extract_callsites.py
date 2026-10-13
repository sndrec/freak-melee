from pathlib import Path
import argparse
p=argparse.ArgumentParser();p.add_argument('--root');p.add_argument('--output');a=p.parse_args()
root=Path(a.root);out=Path(a.output);out.parent.mkdir(parents=True,exist_ok=True)
def function(file,name):
 s=(root/file).read_text(); import re
 m=re.search(r'^.*\b'+re.escape(name)+r'\([^;]*?\)\n\{',s,re.M)
 if not m:raise RuntimeError(name)
 start=m.start();opened=s.index('{',m.start());depth=1;end=opened+1
 while depth:
  if s[end]=='{':depth+=1
  elif s[end]=='}':depth-=1
  end+=1
 return s[start:end]
parts=[function('src/melee/ft/ftcommon.c','ftCommon_8007DB58')]
for n in ['setBit','Fighter_UnkRecursiveFunc_8006D044','Fighter_8006D10C_Inline2','Fighter_8006D10C_Inline1','Fighter_8006D10C']:
 parts.append(function('src/melee/ft/fighter.c',n))
for n in ['ftCo_800D6824','ftCo_800D68C0','ftCo_Attack100_CheckInput']:
 parts.append(function('src/melee/ft/kinds/ftCommon/ftCo_Attack100.c',n))
parts.append(function('src/melee/ft/kinds/ftCommon/ftCo_SpecialAir.c','ftCo_SpecialAir_CheckInput'))
parts.append(function('src/melee/ft/kinds/ftCommon/ftCo_SpecialS.c','doEnter'))
parts.append(function('src/melee/ft/kinds/ftCommon/ftCo_SpecialS.c','ftCo_SpecialS_CheckInput'))
out.write_text('// Mechanically extracted actual engine dispatch bodies.\n'+'\n\n'.join(parts)+'\n')
