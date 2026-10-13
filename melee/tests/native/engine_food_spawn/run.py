"""Scoped actual food/airborne console fixtures; no gameplay or full build."""
import argparse,hashlib,json,subprocess,sys,re
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,required=True);p.add_argument("--compiler",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();out=a.output.resolve();clang=a.compiler.resolve();out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(args,log):
        args=list(map(str,args));commands.append(args);r=subprocess.run(args,cwd=root,capture_output=True,text=True)
        (out/log).write_text(r.stdout+r.stderr)
        if r.returncode:raise RuntimeError(str(out/log)+"\n"+(r.stdout+r.stderr)[-5000:])
        return r.stdout
    run([sys.executable,HERE/"fixture.py","--root",root,"--output",out],"extract.log")
    flags=[clang,"-std=gnu11","-O2","-fms-runtime-lib=dll","-DMELEE_NATIVE=1","-D_DLL","-D_MT","-D_CRT_SECURE_NO_WARNINGS","-I"+str(root/"src"),"-I"+str(root/"extern/dolphin/include"),"-I"+str(out)]
    outputs=[]
    for name,source in [("food",HERE/"test.c"),("airborne",HERE/"airborne_test.c")]:
        batch=out/(name+".cmd");args=flags+[source,"-o",out/(name+".exe")]
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,args)))+'\n')
        run(["cmd","/c",batch],name+"-compile.log");outputs.append(run([out/(name+".exe")],name+".log"))
    extraction=json.loads((out/"extraction.json").read_text());inputs=set(p for p in HERE.rglob("*") if p.is_file())
    inputs.update(root/path for path in extraction["inputs"]);inputs.add(clang)
    inputs.update(p for folder in [root/"src",root/"extern/dolphin/include"] for p in folder.rglob("*.h"))
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    counts=re.search(r"PASS (\d+) cases (\d+) checks",outputs[0]);assert counts
    receipt={"status":"PASS complete actual food constructor/counter and actual airborne target query","food_cases":int(counts[1]),"food_checks":int(counts[2]),"airborne":"PASS actual source + exact new fighter_target_test cases","commands":commands,"extraction":extraction,"inputs":{str(p):sha(p) for p in sorted(inputs)},"outputs":{str(p):sha(p) for p in sorted(out.iterdir()) if p.is_file()},"limits":["Only availability, caller-center and allocation are controlled in the food fixture; actual counter body executes.","No allocation/model/callback internals, food spawned variant RNG, drawing or gameplay claim.","Only initialized SpawnItem fields are read; unspecified fields/padding and signed counter overflow are outside this defined source contract.","Airborne executes the actual source registry query plus the exact new cases from the existing target test; other target methods remain covered by the root linked target suite."]}
    (out/"result.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print("".join(outputs).strip());print(out/"result.json")
if __name__=="__main__":main()
