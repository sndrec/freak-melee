"""Compare authored Thunder articles/fighter with unchanged stock; no game.

Requires a freshly normal-imported behavior.fms/FMP and generated C. Explicit SDK
and original source roots are separate, hashed inputs; compiler tool paths never
select live include paths implicitly. The generated C is compiled directly into
the test translation unit so the unactivated private constructor helper is tested.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from ness_pkfire_stock_compare import root_offset
ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    match=re.search(r"(?m)^[^\n;{}]*\b"+re.escape(name)+r"\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{",source)
    if not match: raise RuntimeError("Missing original function: "+name)
    start=source.index("{",match.start()); depth=1;end=start+1
    while depth:
        depth+=(source[end]=="{")-(source[end]=="}");end+=1
    return source[match.start():end]+"\n"

def prepare(a):
    out=a.out_dir;out.mkdir(parents=True,exist_ok=True)
    dat=a.stock_dat.read_bytes();base=root_offset(dat,"ftDataNess")
    table=32+struct.unpack_from(">I",dat,base+72)[0]
    desc=32+struct.unpack_from(">I",dat,table+3*4)[0]
    tuning=32+struct.unpack_from(">I",dat,desc+4)[0]
    raw=struct.unpack_from(">5I",dat,tuning)
    names=("lifetime","speed","spawn_angle_degrees","stick_threshold","turn_radius_degrees")
    fmp=(a.project/"fighter_parameters.fmp").read_text();verified={}
    for name,bits in zip(names,raw):
        m=re.search(r"(?m)^float It_Kind_Ness_PKThunder\."+name+r" = (\S+)$",fmp)
        if not m or struct.unpack("<I",struct.pack("<f",float(m[1])))[0]!=bits:
            raise RuntimeError("Imported Thunder tuning differs: "+name)
        verified[name]=bits
    if a.fighter:
        fighter_tuning=32+struct.unpack_from(">I",dat,base+4)[0]
        fields=(("thunder_loop1",64,"int"),("thunder_loop2",68,"int"),
                ("thunder_gravity_delay",72,"int"),
                ("thunder_fall_acceleration",80,"float"),
                ("thunder_launch_speed",84,"float"),
                ("thunder_launch_deceleration",92,"float"),
                ("thunder_knockdown_degrees",96,"float"),
                ("thunder_wallhug_degrees",100,"float"),
                ("thunder_freefall_blend",108,"float"),
                ("thunder_landing_lag",112,"float"))
        for name,offset,kind in fields:
            bits=struct.unpack_from(">I",dat,fighter_tuning+offset)[0]
            m=re.search(r"(?m)^"+kind+" "+name+r" = (\S+)$",fmp)
            imported=(int(m[1])&0xffffffff) if m and kind=="int" else struct.unpack("<I",struct.pack("<f",float(m[1])))[0] if m else None
            if imported!=bits: raise RuntimeError("Imported fighter tuning differs: "+name)
            verified[name]=bits
    (out/"import_receipt.json").write_text(json.dumps({"verified_words":len(verified),"raw_bits":verified},indent=2)+"\n")
    (out/"ness_thunder_raw.generated.h").write_text("static const uint32_t thunder_raw[]={"+",".join("UINT32_C(0x%08x)"%x for x in raw)+"};\n")
    generated=(a.project/"behavior.generated.c").read_text()
    required=sorted(set(re.findall(r"host->(\w+) == 0",generated)))
    (out/"ness_thunder_required_host.generated.h").write_text("".join("host.%s=(__typeof__(host.%s))unexpected;\n"%(x,x) for x in required))
    ref=a.reference_root
    hi=ref/"src/melee/ft/kinds/ftNess/ftnessspecialhi.c"
    vec=a.vector_source
    chunks=[] if a.fighter else [function(hi.read_text(),n) for n in ("ftNs_SpecialHi_CheckSpecialHiHold","ftNs_SpecialHi_ItemPKThunderRemove","ftNs_SpecialHi_ItemPKThunderCheckOwn")]
    chunks.append(function(vec.read_text(),"C_VECCrossProduct"))
    vector=ref/"src/melee/lb/lbvector.c"
    chunks.extend(function(vector.read_text(),n) for n in ("lbVector_Normalize","lbVector_CrossprodNormalized","lbVector_Angle","lbVector_Mirror"))
    if a.fighter:
        chunks.append("#define lbvector_sin sinf\n#define lbvector_cos cosf\n"+function(vector.read_text(),"lbVector_RotateAboutUnitAxis"))
        chunks.append(function(vector.read_text(),"lbVector_sqrtf_accurate"))
    (out/"ness_thunder_original.generated.h").write_text("\n".join(chunks))
    sources=[ROOT/"tests/native/ness_thunder_stock_test.c",a.sdk_root/"src/melee/ft/native_fighter_parameters.c",ref/"src/melee/it/kinds/itnesspkthunderball.c",ref/"src/melee/it/kinds/itnesspkthundertrail.c"]
    if a.fighter: sources.append(hi)
    inputs=[*sources,hi,vec,vector,a.compiler,a.stock_dat,a.project/"behavior.fms",a.project/"fighter_parameters.fmp",a.project/"behavior.generated.c",a.sdk_root/"src/melee/ft/native_fighter_module.h",a.sdk_root/"src/melee/ft/native_fighter_runtime.h",ROOT/"tests/native/ness_thunder_candidate.fms",ROOT/"examples/ness_wip.fms",ROOT/"src/melee/ft/kinds/ftNess/ftness_clone.c",Path(__file__)]
    frozen_receipt=a.sdk_root.parent/"freeze.json"
    if frozen_receipt.is_file(): inputs.append(frozen_receipt)
    generator=a.sdk_root.parent/"bin/native_content_tool.exe"
    if generator.is_file(): inputs.append(generator)
    if a.fighter: inputs.append(ROOT/"tests/native/ness_thunder_fighter_cases.h")
    (out/"source_manifest.json").write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},indent=2)+"\n")
    return sources

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ("sdk-root","reference-root","project","stock-dat","out-dir","compiler"):p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--vector-source",type=Path,default=ROOT/"extern/dolphin/src/dolphin/mtx/vec.c")
    p.add_argument("--fighter",action="store_true",help="Include nine fighter phases and original full SpecialHi source")
    p.add_argument("--prepare-only",action="store_true");a=p.parse_args()
    for k,v in vars(a).items():
        if isinstance(v,Path):setattr(a,k,v.resolve())
    sources=prepare(a)
    flags=["-std=gnu11","-O2","-ffp-contract=off","-ffunction-sections","-fdata-sections","-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS","-D_USE_MATH_DEFINES","-D_DLL","-D_MT","-Xclang","--dependent-lib=msvcrt","-I"+str(a.sdk_root/"src"),"-I"+str(a.sdk_root/"extern/dolphin/include"),"-I"+str(a.out_dir),"-I"+str(a.project)]
    if a.fighter: flags.append("-DTHUNDER_FIGHTER_CASES=1")
    commands=[];objs=[]
    for i,s in enumerate(sources):
        obj=a.out_dir/(str(i)+".obj");objs.append(str(obj));commands.append([str(a.compiler),*flags,"-c",str(s),"-o",str(obj)])
    exe=a.out_dir/"ness_thunder_stock_test.exe";commands.append([str(a.compiler),"-fuse-ld=lld",*objs,"-o",str(exe),"-Wl,/OPT:REF"])
    (a.out_dir/"commands.json").write_text(json.dumps(commands,indent=2)+"\n")
    if a.prepare_only: print("Thunder original-source inputs prepared; no compilation/game.");return 0
    for i,c in enumerate(commands):
        r=subprocess.run(c,capture_output=True,text=True);(a.out_dir/(str(i)+".compile.log")).write_text(r.stdout+r.stderr)
        if r.returncode:print(r.stdout+r.stderr);return r.returncode
    r=subprocess.run([str(exe),str(a.project/"fighter_parameters.fmp")],capture_output=True,text=True)
    (a.out_dir/"comparison.log").write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);return r.returncode
if __name__=="__main__":raise SystemExit(main())
