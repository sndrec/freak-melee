import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True)
 units=[ROOT/'src/melee/ft/native_fighter_pose_editor.c',ROOT/'src/melee/ft/native_fighter_animation_editor.c',ROOT/'src/melee/lb/lbanim_dat.c',ROOT/'src/sysdolphin/baselib/dat.c',ROOT/'src/sysdolphin/baselib/fobj.c',ROOT/'src/sysdolphin/baselib/spline.c',HERE/'test.c']
 inputs={str(x):sha(x) for x in units+[HERE/'run.py',ROOT/'src/melee/ft/native_fighter_pose_editor.h',ROOT/'src/melee/ft/native_fighter_animation_editor.h',ROOT/'src/melee/ft/native_fighter_project.h']}
 cmd=['C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe','-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-Wl,/OPT:REF','-o',str(out/'test.exe')]
 batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(cmd)+'\n',encoding='utf-8')
 for stage,command in [('compile',['cmd','/d','/c',str(batch)]),('test',[str(out/'test.exe')])]:
  r=subprocess.run(command,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=180);(out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
  if r.returncode:raise RuntimeError(stage+'\n'+r.stdout+r.stderr)
 for name,expected in inputs.items():assert sha(Path(name))==expected,name
 (out/'receipt.json').write_text(json.dumps({'status':'PASS','result':json.loads(r.stdout),'inputs':inputs,'boundary':'Actual pose/animation editor, DAT decoder and FObj interpreter; paired-buffer undo inputs directly restored, UI history dispatcher not exercised; no runtime motion switch.'},indent=2)+'\n',encoding='utf-8')
 print(r.stdout)
if __name__=='__main__':main()
