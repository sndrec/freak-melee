"""Scoped compiler + generated C/DLL tests, no gameplay process."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
HERE = Path(__file__).resolve().parent

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--compiler", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--generator", type=Path, help="Reuse root-built compiler fixture")
    a = p.parse_args(); root=a.root.resolve(); out=a.output.resolve(); clang=a.compiler.resolve()
    out.mkdir(parents=True, exist_ok=False)
    commands=[]
    def run(command, log):
        command=list(map(str, command)); commands.append(command)
        result=subprocess.run(command, cwd=root, capture_output=True, text=True)
        (out/log).write_text(result.stdout+result.stderr, encoding="utf-8")
        if result.returncode: raise RuntimeError(str(out/log)+"\n"+(result.stdout+result.stderr)[-5000:])
        return result.stdout
    def compile_case(arguments, name):
        batch=out/(name+".cmd")
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,arguments)))+'\n',encoding="utf-8")
        run(["cmd","/c",batch], name+".log")
    flags=[clang,"-std=gnu11","-O2","-fms-runtime-lib=dll","-DMELEE_NATIVE=1","-D_DLL","-D_MT","-D_CRT_SECURE_NO_WARNINGS", "-I"+str(root/"src"),"-I"+str(root/"extern/dolphin/include"),"-I"+str(out)]
    cmake=(root/"CMakeLists.txt").read_text()
    sources=re.findall(r"tools/[A-Za-z0-9_]+\.c",cmake.split("set(NATIVE_FIGHTER_SCRIPT_SOURCES",1)[1].split(")",1)[0])
    generator=a.generator.resolve() if a.generator else out/"compiler_test.exe"
    if not a.generator:
        compile_case(flags+[HERE/"compiler_test.c"]+[root/s for s in sources]+[root/"src/melee/ft/native_fighter_parameters.c","-o",generator], "compile_compiler")
    compiler_output=run([generator,HERE/"fixture.fms",out/"held_article_protocol_fixture.h"],"compiler_test.log")
    compile_case(flags+[HERE/"module_test.c","-o",out/"module_test.exe"],"compile_module")
    module_output=run([out/"module_test.exe"],"module_test.log")
    compile_case(flags+["-shared","-x","c",out/"held_article_protocol_fixture.h","-o",out/"held_protocol.dll"],"compile_dll")
    compile_case(flags+["-DHELD_PROTOCOL_DLL=1",HERE/"module_test.c","-o",out/"dll_test.exe"],"compile_dll_test")
    dll_output=run([out/"dll_test.exe",out/"held_protocol.dll"],"dll_test.log")
    def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs=set(HERE.iterdir()); inputs.update(root/s for s in sources)
    inputs.update([root/"CMakeLists.txt",root/"src/melee/ft/native_fighter_parameters.c",clang])
    inputs.update(x for folder in (root/"src",root/"extern/dolphin/include") for x in folder.rglob("*.h"))
    receipt={"status":"PASS scoped held article compiler/generated C/generated DLL", "commands":commands,
        "compiler_checks":int(re.search(r"PASS (\d+) checks",compiler_output)[1]),
        "module_checks":int(re.search(r"PASS (\d+) checks",module_output)[1]),
        "dll_checks":int(re.search(r"PASS (\d+) checks",dll_output)[1]),
        "inputs":{str(x):sha(x) for x in sorted(inputs) if x.is_file()},
        "outputs":{str(x):sha(x) for x in sorted(out.iterdir()) if x.is_file()},
        "limits":["No game, default template, source-independence or stock fighter behavior comparison.","Runtime dispatcher, live target registry, author provenance and dispatch-depth are root-owned separate tests.","These generated callbacks execute controlled Host observers and exact item generation/class guard; no always-true liveness stub.","Status is compiler-enforced read-only; request callbacks can invalidate the source and continuation must stop."]}
    (out/"result.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print(compiler_output.strip());print(module_output.strip());print(dll_output.strip());print(out/"result.json")
if __name__=="__main__": main()
