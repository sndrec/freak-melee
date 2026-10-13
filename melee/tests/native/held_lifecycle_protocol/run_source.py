"""Console comparison of original54 and complete real modified common entries."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,required=True)
    p.add_argument("--compiler",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();out=a.output.resolve();clang=a.compiler.resolve()
    if out.exists():raise ValueError("Use fresh output directory")
    out.mkdir(parents=True);commands=[]
    def run(command,log):
        command=list(map(str,command));commands.append(command)
        r=subprocess.run(command,cwd=root,capture_output=True,text=True)
        (out/log).write_text(r.stdout+r.stderr,encoding="utf-8")
        if r.returncode:raise RuntimeError(str(out/log)+"\n"+(r.stdout+r.stderr)[-5000:])
        return r.stdout
    run([sys.executable,HERE/"source_fixture.py","--root",root,"--output",out],"extract.log")
    args=[clang,"-std=gnu11","-O2","-fms-runtime-lib=dll","-DMELEE_NATIVE=1","-D_DLL","-D_MT","-D_CRT_SECURE_NO_WARNINGS",
        "-I"+str(root/"src"),"-I"+str(root/"extern/dolphin/include"),"-I"+str(out),"-I"+str(HERE),
        HERE/"source_test.c",root/"src/melee/ft/native_callback_lifetime.c",
        HERE/"effect_absent.c","-o",out/"test.exe"]
    batch=out/"compile.cmd"
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,args)))+'\n',encoding="utf-8")
    run(["cmd","/c",batch],"compile.log")
    result=run([out/"test.exe"],"run.log")
    m=re.search(r"PASS (\d+) pairs (\d+) authored (\d+) safety (\d+) checks",result);assert m
    def sha(x):return hashlib.sha256(x.read_bytes()).hexdigest()
    extracted=json.loads((out/"source_extraction.json").read_text())
    inputs=set(x for x in HERE.rglob("*") if x.is_file())
    inputs.update(root/path for path in extracted["candidate"])
    inputs.update(x for folder in (root/"src",root/"extern/dolphin/include") for x in folder.rglob("*.h"))
    inputs.add(clang)
    inputs.add(root/"src/melee/ft/native_callback_lifetime.c")
    receipt={"status":"PASS original54/real candidate source entry comparison",
        "original_pairs":int(m[1]),"authored_cases":int(m[2]),"safety_cases":int(m[3]),"checks":int(m[4]),
        "commands":commands,"extraction":extracted,"inputs":{str(x):sha(x) for x in sorted(inputs)},
        "outputs":{str(x):sha(x) for x in sorted(out.iterdir()) if x.is_file()},
        "limits":["No game/default/Peach activation or generated authored callback claim.",
        "Complete real neutral/resume/landing entries and exact timer branch execute; generation/depth registry is real. Definition lookup is controlled.",
        "Ground setup, motion, camera, item neutral notification and stock cleanup are controlled call boundaries. Timer is its exact source block inside fixture framing, not full Fighter update.",
        "Authored selection cases are explicit callback contract checks, separate from healthy original54 pairs.",
        "Retirement/recycling/kind/Definition changes are authored safety; unsafe original continuations are not executed or normalized.",
        "Only declared typed semantic fields/events are compared, with exact float bits; no struct-padding or digest masking."]}
    (out/"result.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print(result.strip());print(out/"result.json")
if __name__=="__main__":main()
