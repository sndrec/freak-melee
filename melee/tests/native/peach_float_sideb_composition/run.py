"""Normal frozen-tool import, generated DLL and actual-body Float/SideB comparison."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

def digest(path):
    return hashlib.file_digest(path.open("rb"), "sha256").hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("root", "baseline", "output"):
        p.add_argument("--"+name, type=Path, required=True)
    p.add_argument("--prepared", action="store_true", help="Continue a prepared isolated project; diagnostic use only")
    a = p.parse_args()
    root, base, out = a.root.resolve(), a.baseline.resolve(), a.output.resolve()
    sdk = base/"sdk"
    commands = []
    def run(args, log):
        args = list(map(str,args)); commands.append(args)
        r = subprocess.run(args, cwd=root, capture_output=True, text=True)
        log.write_text(r.stdout+r.stderr)
        if r.returncode: raise RuntimeError(str(log)+"\n"+(r.stdout+r.stderr)[-4500:])
        return r.stdout
    if not a.prepared:
        if out.exists(): raise ValueError("Use fresh output")
        r = subprocess.run([sys.executable,str(HERE/"prepare.py"),"--root",str(root),"--baseline",str(base),"--output",str(out),"--generate"],cwd=root,capture_output=True,text=True)
        if r.returncode: raise RuntimeError(r.stdout+r.stderr)
    case = out/"comparison"; case.mkdir(exist_ok=True)
    shutil.copyfile(out/"generated.inc",case/"generated.inc")
    run([sys.executable,HERE/"fixture.py","--root",sdk,"--output",case,"--generated",case/"generated.inc","--parameters",out/"fighter_parameters.fmp","--dat",out/"project/fighter.dat"],case/"extract.log")
    args = [base/"compiler/clang.exe","-std=gnu11","-O2","-fms-runtime-lib=dll","-DMELEE_NATIVE=1","-D_DLL","-D_MT","-D_USE_MATH_DEFINES","-D_CRT_SECURE_NO_WARNINGS","-I"+str(sdk/"src"),"-I"+str(sdk/"extern/dolphin/include"),"-I"+str(case),HERE/"test.c",sdk/"src/melee/ft/native_callback_lifetime.c",sdk/"src/melee/it/native_item_owner.c",sdk/"src/melee/ft/native_fighter_contact.c","-o",case/"test.exe"]
    batch = case/"compile.cmd"
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,args)))+'\n')
    run(["cmd","/c",batch],case/"compile.log")
    output = run([case/"test.exe"],case/"run.log")
    print(output.strip())
    m = re.search(r"(\d+) original/generated comparisons, (\d+) checks, (\d+) authored safety cases",output)
    assert m
    receipt = json.loads((out/"result.json").read_text())
    receipt.update(status="PASS unregistered Float/SideB actual-source composition",comparisons=int(m[1]),checks=int(m[2]),safety_cases=int(m[3]),fixture_commands=commands,census=json.loads((case/"census.json").read_text()),limits=["No installation, independence or gameplay acceptance.","Parasol travel/common protocol361..364/369..370 remains inherited;356 remains null.","Animation, drift, geometry, input entry results and 831CC internal collision solver are controlled; real contact generation/recipient dispatch, common selector/masks/math and outer landing body execute.","Earlier immutable composition is reused; old pluck/counter matrices are not counted as rerun coverage.","Hook expected-condition cases are distinct from original-body phase comparisons; common caller ordering is root-owned separate coverage.","Missing/changed contact identities are authored safety beyond valid stock input; NULL-contact native dereference is not claimed as retail parity."])
    inputs = {x for x in sdk.rglob("*") if x.is_file()}
    inputs.update(x for x in (base/"compiler").rglob("*") if x.is_file())
    inputs.update(x for x in HERE.iterdir() if x.is_file())
    inputs.update([base/"bin/native_content_tool.exe",base/"freeze.json",out/"project/fighter.dat",out/"fighter_parameters.fmp"])
    receipt["inputs"]={str(x):digest(x) for x in sorted(inputs)}
    archive=out/"source_snapshot";archive.mkdir(exist_ok=True)
    for x in HERE.iterdir():
        if x.is_file(): shutil.copyfile(x,archive/x.name)
    receipt["outputs"]={str(x):digest(x) for x in sorted(out.rglob("*")) if x.is_file() and x!=out/"result.json"}
    (out/"result.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print("Receipt:",out/"result.json")

if __name__=="__main__": main()
