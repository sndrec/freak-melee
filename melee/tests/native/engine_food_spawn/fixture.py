"""Extract the actual common food constructor and actual counter increment."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
GROUPS={"src/melee/it/kinds/itfoods.c":["it_8028FAF4"],"src/melee/it/it_2725.c":["it_80274ED8"]}
def body(text,name):
    m=re.search(r"(?m)^[A-Za-z_][\w \t*]*\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\{",text)
    if not m:raise ValueError(name)
    close=re.search(r"(?m)^}",text[m.end():])
    if not close:raise ValueError("Missing end: "+name)
    return text[m.start():m.end()+close.end()],text[:m.start()].count("\n")+1

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);blocks=[];inputs={}
    for path,names in GROUPS.items():
        raw=(a.root/path).read_bytes();text=raw.decode();record={"sha256":hashlib.sha256(raw).hexdigest(),"bodies":{}}
        for name in names:
            b,line=body(text,name);blocks.append(b);record["bodies"][name]={"sha256":hashlib.sha256(b.encode()).hexdigest(),"line":line}
        inputs[path]=record
    for path,name,dest in [("src/melee/ft/native_fighter_target.c","nativeFighterTarget_Airborne","airborne_body.inc"),("tests/native/fighter_target_test.c","airborne_lifetime_cases","airborne_cases.inc")]:
        source=(HERE.parent/"fighter_target_test.c") if path.startswith("tests/") else a.root/path
        raw=source.read_bytes();b,line=body(raw.decode(),name)
        (a.output/dest).write_bytes((b+"\n").encode())
        inputs[str(source.resolve())]={"sha256":hashlib.sha256(raw).hexdigest(),"bodies":{name:{"sha256":hashlib.sha256(b.encode()).hexdigest(),"line":line}}}
    (a.output/"food_spawn.inc").write_bytes(("\n\n".join(blocks)+"\n").encode())
    (a.output/"extraction.json").write_text(json.dumps({"inputs":inputs,"scope":"Complete actual it_8028FAF4 and actual it_80274ED8; allocation, center and availability controlled."},indent=2)+"\n")
if __name__=="__main__":main()
