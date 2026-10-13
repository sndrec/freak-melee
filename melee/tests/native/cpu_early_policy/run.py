"""Actual early CPU bodies versus frozen70; console-only controlled engine boundaries."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SOURCE="src/melee/ft/kinds/ftCommon/ftCo_0A01.c"
FUNCTIONS=["ftCo_800A101C","ftCo_800A08F0","ftCo_800AA320","ftCo_800AA42C","ftCo_800AA844","ftCo_800AABC8","ftCo_800AACD0","ftCo_800AD54C"]
HELPERS=["ftCo_CpuSetNeutralStick","ftCo_CpuFinishWithNeutralY","isInTeeter","ftCo_CpuTurnAwayFromLedge","ftCo_CpuTurnAround","ftCo_800AA42C_inline0"]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body(text,name):
 m=re.search(r"^[^\n;{}]*\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\{",text,re.M)
 if not m:raise ValueError("No actual definition: "+name)
 depth,end=1,m.end()
 while depth:depth+=(text[end]=="{")-(text[end]=="}");end+=1
 return text[m.start():end]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("--compiler-test",type=Path);ap.add_argument("--current-snapshot",type=Path);ap.add_argument("--snapshot",type=Path,required=True);ap.add_argument("--output",type=Path,required=True);args=ap.parse_args()
 snapshot=args.snapshot.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
 current_snapshot=args.current_snapshot.resolve() if args.current_snapshot else None
 compile_root=current_snapshot/"sdk" if current_snapshot else ROOT
 frozen=snapshot/"sdk"/SOURCE;live=compile_root/SOURCE
 old=frozen.read_text(encoding="utf-8");new=live.read_text(encoding="utf-8")
 header=out/"bodies.h";parts=[body(new,name) for name in HELPERS]
 for prefix,text in [("stock_",old),("current_",new)]:
  for name in FUNCTIONS:
   function=body(text,name)
   for target in FUNCTIONS:function=re.sub(r"\b"+target+r"\b",prefix+target,function)
   parts.append(function)
 header.write_text("\n\n".join(parts)+"\n",encoding="utf-8",newline="\n")
 compiler=(current_snapshot or snapshot)/"compiler/clang.exe";manifest=snapshot/"freeze.json";hashes=json.loads(manifest.read_text(encoding="utf-8"));hashes={k.replace("\\","/"):v for k,v in hashes.items()}
 assert sha(frozen)==hashes["sdk/"+SOURCE]
 current_hashes=None
 if current_snapshot:
  current_manifest=current_snapshot/"freeze.json";current_hashes={k.replace("\\","/"):v for k,v in json.loads(current_manifest.read_text(encoding="utf-8")).items()}
  assert sha(compiler)==current_hashes["compiler/clang.exe"] and sha(live)==current_hashes["sdk/"+SOURCE]
 else:assert sha(compiler)==hashes["compiler/clang.exe"]
 cmd=[str(compiler),"-std=c11","-O2","-fms-extensions","-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS","-I"+str(compile_root/"src"),"-I"+str(compile_root/"extern/dolphin/include"),"-I"+str(out),"-MD","-MF",str(out/"dependencies.d"),str(HERE/"source_test.c"),"-o",str(out/"test.exe")]
 inputs=[live,frozen,HERE/"source_test.c",Path(__file__).resolve(),header,compile_root/"src/melee/ft/native_fighter_runtime.h",compile_root/"src/melee/ft/types.h"]
 receipt={"status":"WIP","scope":"full extracted production policy and five full continuing callers; controlled allocator/lifetime, Definition lookup, floor/RNG/planner boundaries; no game or mode6 proof","functions":FUNCTIONS,"helpers":HELPERS,"freeze_sha256":sha(manifest),"inputs":{str(p):sha(p) for p in inputs},"commands":[],"results":[]}
 generator=(args.compiler_test or ((current_snapshot/"bin/native_script_cpu_early_compiler_test.exe") if current_snapshot else ROOT/"build/native/native_script_cpu_early_compiler_test.exe")).resolve()
 receipt["inputs"][str(generator)]=sha(generator)
 if current_snapshot:
  receipt["current_freeze_sha256"]=sha(current_manifest)
  if not args.compiler_test:assert sha(generator)==current_hashes["bin/native_script_cpu_early_compiler_test.exe"]
 operations=[("generate",[str(generator),str(out/"early_cpu_action.inc"),str(out/"early_cpu_callback.inc")]),("build",cmd),("run",[str(out/"test.exe")])]
 for route in ["action","callback"]:
  executable=out/(route+".exe")
  operations.extend([("build_"+route,cmd[:-2]+['-DEARLY_CPU_MODULE="early_cpu_'+route+'.inc"',"-o",str(executable)]),("run_"+route,[str(executable)])])
 for label,command in operations:
  result=subprocess.run(command,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0));(out/(label+".log")).write_text(result.stdout,encoding="utf-8",newline="\n")
  receipt["commands"].append(command);receipt["results"].append({"phase":label,"exit_code":result.returncode,"output":result.stdout});receipt["status"]="FAIL" if result.returncode else "WIP";
  if label.startswith("build") and not result.returncode:
   dependency_text=(out/"dependencies.d").read_text(encoding="utf-8")
   for line in dependency_text.splitlines()[1:]:
    name=line.strip().removesuffix("\\").strip().replace("\\ "," ")
    path=Path(name)
    if path.is_file():
     receipt["inputs"][str(path.resolve())]=sha(path)
     if current_snapshot:
      try:key="sdk/"+path.resolve().relative_to(compile_root.resolve()).as_posix()
      except ValueError:continue
      assert sha(path)==current_hashes[key],key
  (out/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
  if result.returncode:raise RuntimeError(out/(label+".log"))
 receipt["inputs"].update({str(out/name):sha(out/name) for name in ["early_cpu_action.inc","early_cpu_callback.inc"]})
 receipt["status"]="PASS";(out/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8");print(out/"receipt.json")
if __name__=="__main__":main()
