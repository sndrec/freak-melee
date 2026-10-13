"""Direct stock Yoshi ordinary/extended item callbacks and actual caller bodies.

Only the standalone fixture is compiled; no global build or game is started.
The candidate module and all production headers/callers use one frozen SDK.
"""
import argparse
from pathlib import Path
import subprocess
from yoshi_cpu_stock_compare import function

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = '1a0f7e6ac'
YOSHI = 'src/melee/ft/kinds/ftYoshi/ftyoshi.c'
COMMON = 'src/melee/ft/ftcommon.c'
PICKUP = 'src/melee/ft/kinds/ftCommon/ftpickupitem.c'
CALLERS = {COMMON: ('ftCommon_8007E6DC', 'ftCommon_8007E79C', 'ftCommon_8007E7E4'),
           PICKUP: ('ftpickupitem_80094818',)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--batch', type=Path, required=True)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--project', type=Path, required=True)
    args = parser.parse_args()
    batch, snapshot, project = args.batch.resolve(), args.snapshot.resolve(), args.project.resolve()
    batch.mkdir(parents=True, exist_ok=True)
    sdk = snapshot/'sdk'
    original = {path: subprocess.check_output(['git','show',ORIGINAL+':'+path],cwd=ROOT,text=True)
                for path in (YOSHI, COMMON, PICKUP)}
    source = '// Exact original sources '+ORIGINAL+'; original names isolated.\n'
    for name in ('ftYs_Init_OnItemPickup', 'ftYs_Init_OnItemDrop'):
        source += function(original[YOSHI], name).replace(name, 'original_'+name)
    for path, names in CALLERS.items():
        for name in names:
            source += function(original[path], name).replace(name, 'original_'+name)
            source += function((sdk/path).read_text(), name)
    (batch/'yoshi_item_original.inc').write_text(source)
    executable = batch/'yoshi_item_callback_stock_test.exe'
    command = [str(snapshot/'compiler/clang.exe'),'-fms-runtime-lib=dll','-std=gnu11','-O2',
               '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT',
               '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(batch),
               str(ROOT/'tests/native/yoshi_item_callback_stock_test.c'),
               str(sdk/'src/melee/ft/native_fighter_parameters.c'),'-o',str(executable)]
    resources = snapshot/'compiler/clang'
    if resources.exists():
        versions = list(resources.iterdir())
        assert len(versions) == 1, 'Frozen compiler must have one resource version'
        command.extend(['-resource-dir',str(versions[0])])
    setup = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    script = batch/'item-fixture-compile.cmd'
    script.write_text('@echo off\ncall '+subprocess.list2cmdline([str(setup)])+' >nul\n'+
                      subprocess.list2cmdline(command)+'\n')
    with (batch/'item-fixture-build.log').open('wb') as log:
        result = subprocess.run(['cmd','/c',str(script)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((batch/'item-fixture-build.log').read_text())
    result = subprocess.run([str(executable),str(project/'module.dll'),str(project/'fighter_parameters.fmp')],
                            cwd=ROOT,text=True,capture_output=True)
    (batch/'item-fixture-results.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr,end='')
    return result.returncode

if __name__ == '__main__':
    raise SystemExit(main())
