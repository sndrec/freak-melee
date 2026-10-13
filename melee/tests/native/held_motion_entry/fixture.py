"""Extract complete candidate entry bodies; original53 remains pinned and immutable."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent

def body(text,name):
    m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
    if not m:raise ValueError(name)
    end=m.end();depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    original=json.loads((HERE/"original53_manifest.json").read_text())
    assert hashlib.sha256((HERE/"original53.inc").read_bytes()).hexdigest()==original["original_inc_sha256"]
    for path,record in original["inputs"].items():
        assert hashlib.sha256((HERE/"original53"/Path(path).name).read_bytes()).hexdigest()==record["sha256"]
    groups={"src/melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c":["ft_800CEF08_inline1","ftCo_ItemParasolOpen_Enter"],"src/melee/ft/kinds/ftCommon/ftCo_ItemParasolFallSpecial.c":["ftCo_800CF3C8"]}
    blocks=[];sources={}
    for path,names in groups.items():
        raw=(a.root/path).read_bytes();text=raw.decode();sources[path]={"sha256":hashlib.sha256(raw).hexdigest(),"bodies":{}}
        for name in names:
            b=body(text,name);blocks.append(b);sources[path]["bodies"][name]={"sha256":hashlib.sha256(b.encode()).hexdigest(),"line":text[:text.index(b)].count("\n")+1}
    (a.output/"candidate.inc").write_text("\n\n".join(blocks)+"\n",encoding="utf-8")
    for path in ["src/melee/ft/native_fighter_held_motion.c","src/melee/ft/native_fighter_held_motion.h","src/melee/ft/native_callback_lifetime.c"]:
        sources[path]={"sha256":hashlib.sha256((a.root/path).read_bytes()).hexdigest(),"scope":"actual separately linked production unit"}
    (a.output/"extraction.json").write_text(json.dumps({"original":original,"candidate":sources,"scope":"complete original/candidate entries and real selector/registry; motion/animation/catalog boundaries controlled"},indent=2)+"\n",encoding="utf-8")
if __name__=="__main__":main()
