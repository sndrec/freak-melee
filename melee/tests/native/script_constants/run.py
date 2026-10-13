"""Verify typed constants against actual native declarations and direct IR/VM."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
NO_WINDOW=getattr(subprocess,"CREATE_NO_WINDOW",0)
UNITS=["native_script_constants.c","native_script_source.c","native_script_expression.c",
       "native_script_program.c","native_script_statement.c","native_script_bytecode.c",
       "native_script_vm.c","native_script_value.c"]
HEADERS=["src/melee/ft/forward.h","src/melee/ft/kinds/ftCommon/forward.h",
         "src/melee/it/forward.h","src/melee/lb/forward.h","src/melee/mp/forward.h","src/melee/db/db.h",
         "src/melee/ft/native_script_fighter.h","src/melee/ft/native_fighter_runtime.h",
         "src/melee/ft/ftcmdscript.h"]


def compile_run(compiler,output,stem,units):
    command=[str(compiler),"-std=c11","-Wall","-Wextra","-Werror","-Wno-array-parameter","-O2","-ffp-contract=off",
             "-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS","-I",str(ROOT/"src"),
             "-I",str(ROOT/"extern/dolphin/include"),*map(str,units),"-o",str(output/(stem+".exe"))]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (output/(stem+"-build.log")).write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(output/(stem+".exe"))],cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (output/(stem+"-run.log")).write_text(run.stdout+run.stderr);run.check_returncode()
    return json.loads(run.stdout),command


def references(output):
    # C declaration extraction only: no FMS lexical/semantic acceptance is inferred.
    production=(ROOT/"src/melee/ft/native_script_constants.c").read_text()
    rows=re.findall(r'\{\{"([^"]+)", "([^"]+)", ([^\n]+), NativeScriptType_(I32|U32), NativeScriptConstant_\w+\}, (\d+)\}',production)
    switches={int(i):symbol for i,symbol in re.findall(r'case (\d+): return (\w+);',production)}
    aliases={}
    primitive=(ROOT/"tools/native_fighter_script_primitives.c").read_text()
    for name,symbol in re.findall(r'\{ "([^"]+)", "(\w+)" \}',primitive):
        if name.split('.')[0] in ("Motion","Transition","Hurt","Hitbox","DebugLevel","AnimationFamily","Collision","Axis"):
            aliases[name]=symbol
    bone=(ROOT/"tools/native_fighter_script_code.c").read_text()
    bone=re.search(r'const char\* nativeScriptCode_BoneSymbol(.*?)\n}',bone,re.S).group(1)
    aliases.update(re.findall(r'memcmp\(name, "(\w+)".*?return "(\w+)"',bone,re.S))
    body=[]
    for name,evidence,bits,type_name,flag in rows:
        if int(flag):
            symbol=switches[int(flag)]
        else:
            match=re.fullmatch(r'(?:\(uint64_t\)(?:\(int64_t\)\((\w+)\)|(\w+))|(\w+))',bits)
            assert match, (name,bits)
            symbol=next(group for group in match.groups() if group is not None)
        if name in aliases:assert symbol==aliases[name],(name,symbol,aliases[name])
        body.append(f'    EXPECT("{name}",NativeScriptType_{type_name},{symbol});')
    assert rows and len(rows)==len(set(row[0] for row in rows))
    source='\n'.join('#include <'+path.removeprefix('src/')+'>' for path in HEADERS)
    source+='''
#include <melee/ft/native_script_constants.h>
#include <stdio.h>
#include <string.h>
#define EXPECT(name,t,native) do {NativeScriptConstant v; ++checks; if (!nativeScriptConstants_Lookup(name,(uint32_t)strlen(name),&v) || v.type!=(t) || v.bits!=((t)==NativeScriptType_U32 ? (uint64_t)(uint32_t)(native) : (uint64_t)(int64_t)(native))) {fprintf(stderr,"reference mismatch: %s\\n",name);return 1;}}while(0)
int main(void) {unsigned checks=0;
'''
    source+='\n'.join(body)
    source+='\n    printf("{\\"result\\":\\"pass\\",\\"native_reference_checks\\":%u}\\n",checks);return 0;\n}\n'
    target=output/"native_references.c";target.write_text(source)
    # Ensure every public declaration in the chosen families appears in the catalog.
    families={HEADERS[0]:["Fighter_Part","FighterKind"],HEADERS[1]:["ftCommon_MotionState"],
              HEADERS[2]:["ItemKind","Item_StateChangeFlags","Item_HoldKinds","Item_UnkKinds"],
              HEADERS[3]:["HurtCapsuleState","HitCapsuleState","HurtHeight"],HEADERS[5]:["DbLKind"]}
    names=set(row[0] for row in rows)
    for path,enum_names in families.items():
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',(ROOT/path).read_text(),flags=re.S)
        for enum in enum_names:
            match=re.search(r'typedef enum '+enum+r'\s*\{(.*?)\}',text,re.S);assert match,enum
            for entry in match[1].split(','):
                entry=re.sub(r'^\s*#.*$','',entry,flags=re.M).strip()
                if entry:assert entry.split('=')[0].strip() in names,(path,entry)
    return target


def census(compiler,output):
    executable=output/"census.exe"
    command=[str(compiler),"-std=c11","-Wall","-Wextra","-Werror","-Wno-array-parameter","-O2",
        "-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS","-I",str(ROOT/"src"),"-I",str(ROOT/"extern/dolphin/include"),
        str(HERE/"census.c"),str(ROOT/"src/melee/ft/native_script_constants.c"),
        str(ROOT/"src/melee/ft/native_script_expression.c"),"-o",str(executable)]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (output/"census-build.log").write_text(build.stdout+build.stderr);build.check_returncode()
    paths=sorted((ROOT/"examples").glob("*_full.fms"))+sorted((ROOT/"examples/rook_workshop").rglob("*.fms"))
    report={"scope":"shared-lexer constant spellings only; no binding, metadata, grammar or whole-program acceptance","files":{}}
    total=0;names=set();unresolved=[]
    for path in paths:
        run=subprocess.run([str(executable),str(path)],cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW);run.check_returncode()
        item=json.loads(run.stdout);item["sha256"]=hashlib.sha256(path.read_bytes()).hexdigest()
        report["files"][str(path.relative_to(ROOT))]=item;total+=item["lexical_uses"];names.update(item["uses"])
        unresolved.extend(item["unresolved"])
    report.update({"file_count":len(paths),"lexical_uses":total,"distinct_constants":len(names),"unresolved":sorted(set(unresolved))})
    (output/"census.json").write_text(json.dumps(report,indent=2)+"\n")
    return {key:report[key] for key in ("file_count","lexical_uses","distinct_constants","unresolved")}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    paths=[Path(__file__).resolve(),HERE/"test.c",HERE/"census.c",args.compiler.resolve(),
           ROOT/"tools/native_fighter_script_code.c",ROOT/"tools/native_fighter_script_primitives.c",
           *[ROOT/"src/melee/ft"/unit for unit in UNITS],
           *sorted((ROOT/"src").rglob("*.h")),*sorted((ROOT/"extern/dolphin/include").rglob("*.h")),
           *sorted((ROOT/"examples").glob("*_full.fms")),
           *sorted((ROOT/"examples/rook_workshop").rglob("*.fms"))]
    pins={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    reference=references(args.output)
    result,command=compile_run(args.compiler,args.output,"test",[HERE/"test.c",*[ROOT/"src/melee/ft"/unit for unit in UNITS]])
    ref,ref_command=compile_run(args.compiler,args.output,"references",[reference,ROOT/"src/melee/ft/native_script_constants.c"])
    result.update(ref)
    result["template_census"]=census(args.compiler,args.output)
    assert all(hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest for path,digest in pins.items()), "Inputs changed during constants fixture"
    result.update({"game_acceptance":False,"scope":"exact typed constants; direct Source/IR/Bytecode/VM; no complete package acceptance",
                   "compiler":command,"reference_compiler":ref_command,
                   "source_sha256":pins})
    (args.output/"receipt.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({key:value for key,value in result.items() if key not in ("compiler","reference_compiler","source_sha256")},indent=2))


if __name__=="__main__":main()
