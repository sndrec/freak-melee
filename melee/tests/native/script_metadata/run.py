"""Import actual Mario fighter attributes directly from immutable DAT bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
NO_WINDOW=getattr(subprocess,"CREATE_NO_WINDOW",0)
UNITS=["native_script_source.c","native_script_expression.c","native_script_program.c",
       "native_script_statement.c","native_script_value.c","native_script_metadata.c"]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--dat",type=Path,default=ROOT/"build/native/dat-samples/PlMr.dat")
    parser.add_argument("--root",default="ftDataMario")
    parser.add_argument("--script",type=Path,default=ROOT/"examples/mario_full.fms")
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    executable=args.output/"test.exe"
    command=[str(args.compiler),"-std=c11","-Wall","-Wextra","-Werror","-O2","-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS",
             "-I",str(ROOT/"src"),"-I",str(ROOT/"extern/dolphin/include"),str(HERE/"test.c")]
    command += [str(ROOT/"src/melee/ft"/unit) for unit in UNITS]+[str(ROOT/"src/sysdolphin/baselib/dat.c"),"-o",str(executable)]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (args.output/"build.log").write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(executable),str(args.dat),str(args.script),args.root],cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (args.output/"run.log").write_text(run.stdout+run.stderr);run.check_returncode()
    receipt=json.loads(run.stdout)
    paths=[HERE/"test.c",Path(__file__).resolve(),args.dat,args.script,ROOT/"src/sysdolphin/baselib/dat.c",ROOT/"src/sysdolphin/baselib/dat.h"]
    paths += [ROOT/"src/melee/ft"/unit for unit in UNITS]+[ROOT/"src/melee/ft/native_script_metadata.h"]
    receipt.update({"result":"pass","scope":"typed actor-root DAT Attribute imports; no entity import or body/package acceptance",
                    "compiler":command,"game_acceptance":False,"source_sha256":{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}})
    (args.output/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps({key:value for key,value in receipt.items() if key not in ("compiler","source_sha256")},indent=2))


if __name__=="__main__":main()
