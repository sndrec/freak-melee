"""Frozen source census for a proposed authored follower policy; no compiler/game."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
FUNCTIONS={
 "src/melee/ft/kinds/ftCommon/ftCo_0A01.c":["ftCo_800A589C","inlineM0","ftCo_800B0918","inlineJ0","ftCo_800B0AF4","isInCaptureWait","ftCo_800B0CA8","inlineK0","ftCo_800B0E98","isPopoUpB","ftCo_800B101C_inline0","ftCo_CpuUpdateNanaFollow","ftCo_800B101C","ftCo_800A7AAC","inlineI3","ftCo_800B0760","ftCo_800AEFB8","ftCo_800B2AFC","ftCo_800B2790","ftCo_800B3900","ftCo_800A4BEC","ftCo_800A1F3C","ftCo_800A1CC4","ftCo_800A2718","ftCo_800A75DC_CheckFloor","ftCo_CpuDataShouldAct","ftCo_CpuShouldAct","ftCo_CpuInitNoTarget","ftCo_CpuActOnNoTarget","ftCo_CpuTargetAct","ftCo_CpuSetTargetFlags","ftCo_800A3200","isInTeeter","ftCo_800A1B38","ftCo_800A8DE4","ftCo_800B33B0"],
 "src/melee/ft/ftcpuattack.c":["ftCo_800B683C","ftCo_800B885C","ftCo_800B89CC","ftCo_800B8A9C"],
 "src/melee/ft/ftcmdscript.c":["ftCo_800B3E04"],
}
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def body(text,name):
 match=re.search(r"^[^\n;{}]*\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\{",text,re.M)
 if not match:raise ValueError("Missing actual definition: "+name)
 depth,end=1,match.end()
 while depth:depth+=(text[end]=="{")-(text[end]=="}");end+=1
 return text[match.start():end],text[:match.start()].count("\n")+1

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--snapshot",type=Path,required=True);parser.add_argument("--output",type=Path,required=True);args=parser.parse_args();snapshot=args.snapshot.resolve();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
 manifest=snapshot/"freeze.json";frozen={k.replace("\\","/"):v for k,v in json.loads(manifest.read_text(encoding="utf-8")).items()};inventory={"scope":"read-only source/API proposal, no compile/runtime acceptance","freeze_sha256":sha(manifest),"inputs":{},"functions":{},"raw_kind_sites":{},"modes":{}}
 for path,names in FUNCTIONS.items():
  source=snapshot/"sdk"/path;assert sha(source)==frozen["sdk/"+path];text=source.read_text(encoding="utf-8");inventory["inputs"][path]=sha(source);extracted=[]
  for name in names:
   function,line=body(text,name);extracted.append(function)
   inventory["functions"][name]={"path":path,"line":line,"lines":len(function.splitlines()),"body_sha256":hashlib.sha256(function.encode()).hexdigest(),"calls_lexical":sorted(set(re.findall(r"\b([A-Za-z_]\w*)\s*\(",function))-{name,"if","switch","for","while","sizeof"}),"field_names_lexical":sorted(set(re.findall(r"->([A-Za-z_]\w*)",function)))}
   if name=="ftCo_800B2AFC":
    chunks=re.split(r"(?m)^    (case \d+|default):",function)
    inventory["modes"]={chunks[i]:sorted(set(re.findall(r"\b(ftCo_\w+)\s*\(",chunks[i+1]))) for i in range(1,len(chunks),2)}
  inventory["raw_kind_sites"][path]=[{"line":i,"text":line.strip()} for i,line in enumerate(text.splitlines(),1) if "Ft_Kind_Nana" in line or "Ft_Kind_Popo" in line or "ftPp_" in line]
  (output/(Path(path).stem+"_bodies.txt")).write_text("\n\n".join(extracted)+"\n",encoding="utf-8",newline="\n")
 for path in ["src/melee/ft/types.h","src/melee/ft/fighter.c","src/melee/ft/native_fighter_runtime.h","src/melee/ft/native_fighter_runtime.c","src/melee/ft/native_fighter_target.h","src/melee/ft/kinds/ftCommon/forward.h","src/melee/ft/kinds/ftPopo/forward.h","tools/native_fighter_script_cpu.c","tools/native_fighter_script_terrain.c","tools/native_fighter_script_primitives.c"]:
  source=snapshot/"sdk"/path;assert sha(source)==frozen["sdk/"+path];inventory["inputs"][path]=sha(source)
 values={}
 for path,enum in [("src/melee/ft/kinds/ftCommon/forward.h","ftCommon_MotionState"),("src/melee/ft/kinds/ftPopo/forward.h","ftPopo_MotionState")]:
  text=(snapshot/"sdk"/path).read_text(encoding="utf-8")
  entries=re.search(r"typedef enum "+enum+r"\s*\{(.*?)\}",text,re.S).group(1)
  value=-1
  for entry in entries.split(','):
   entry=re.sub(r"/\*.*?\*/|//[^\n]*","",entry,flags=re.S).strip()
   if not entry:continue
   name,*expr=entry.split('=',1)
   name=name.strip()
   if expr:
    expression=expr[0].strip()
    if re.fullmatch(r"-?\d+",expression):value=int(expression)
    elif expression in values:value=values[expression]
    elif re.fullmatch(r"\w+\s*-\s*\w+",expression):
     a,b=map(str.strip,expression.split('-'));value=values[a]-values[b]
    else:raise ValueError("Unreviewed enum expression: "+entry)
   else:value+=1
   values[name]=value
 inventory["motion_values"]={name:value for name,value in values.items() if name in ["ftCo_MS_Count","ftCo_MS_CliffCatch","ftCo_MS_CliffWait"] or name.startswith("ftPp_MS_")}
 (output/"source_inventory.json").write_text(json.dumps(inventory,indent=2)+"\n",encoding="utf-8");print(output/"source_inventory.json")
if __name__=="__main__":main()
