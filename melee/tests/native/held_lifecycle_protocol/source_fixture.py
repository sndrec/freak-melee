"""Complete lifecycle entries and exact timer branch; engine dependencies remain controlled."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
GROUPS={"src/melee/ft/ft_08A1.c":["ft_8008A348"],"src/melee/ft/ft_0D27.c":["ftCo_Mushroom_SelectAirResume"],"src/melee/ft/kinds/ftCommon/ftCo_Landing.c":["ftCo_Landing_Enter"]}
def body(text,name):
 m=re.search(r"(?m)^[A-Za-z_][\w \t*]*\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\{",text)
 if not m:raise ValueError(name)
 end=re.search(r"(?m)^}",text[m.end():]);return text[m.start():m.end()+end.end()]
def timer(text):
 start=text.index("        if (fp->x2221_b4) {",text.index("void Fighter_8006A360("));end=start;depth=0
 while end<len(text):
  depth+=(text[end]=="{")-(text[end]=="}");end+=1
  if depth==0 and text[end-1]=="}":return text[start:end]
 raise ValueError("timer end")
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument("--root",type=Path,required=True);p.add_argument("--output",type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 original=(HERE/"original54.inc").read_text();manifest=json.loads((HERE/"original54_manifest.json").read_text());assert hashlib.sha256((HERE/"original54.inc").read_bytes()).hexdigest()==manifest["original_inc_sha256"]
 for path,record in manifest["inputs"].items():
  if record.get("scope")=="original mushroom caller source":assert hashlib.sha256((HERE/"original54_callers"/Path(path).name).read_bytes()).hexdigest()==record["sha256"]
 names=["ft_8008A348","fn_800D2890","ftCo_Landing_Enter"];blocks=[body(original,n) for n in names];blocks=[re.sub(r"\b"+n+r"\b","original54_"+n,b) for b,n in zip(blocks,names)]
 blocks.append("static void original54_timer(Fighter_GObj* gobj) { Fighter* fp=gobj->user_data; uint32_t callback_generation=(uint32_t)fp->x8_spawnNum;\n"+timer(original)+"\n++timer_tail; }")
 (a.output/"original_entries.inc").write_text("\n\n".join(blocks)+"\n")
 candidate=[];inputs={}
 for path,names in GROUPS.items():
  raw=(a.root/path).read_bytes();inputs[path]=hashlib.sha256(raw).hexdigest();text=raw.decode();candidate.extend(body(text,n) for n in names)
 path="src/melee/ft/fighter.c";raw=(a.root/path).read_bytes();inputs[path]=hashlib.sha256(raw).hexdigest()
 candidate.append("static void candidate_timer(Fighter_GObj* gobj) { Fighter* fp=gobj->user_data; uint32_t callback_generation=(uint32_t)fp->x8_spawnNum;\n"+timer(raw.decode())+"\n++timer_tail; }")
 (a.output/"candidate_entries.inc").write_text("\n\n".join(candidate)+"\n")
 (a.output/"source_extraction.json").write_text(json.dumps({"original":manifest,"candidate":inputs,"scope":"Three complete entries plus exact timer if-block; only fixture framing around timer, no ticking-engine claim."},indent=2)+"\n")
if __name__=="__main__":main()
