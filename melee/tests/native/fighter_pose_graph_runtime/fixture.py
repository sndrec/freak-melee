"""Extract complete actual runtime resolution and shared animation bodies."""
import argparse,hashlib,json,re
from pathlib import Path

def body(text,name):
    m=re.search(r'(?m)^[A-Za-z_][\w\s*]*\b'+name+r'\([^;{}]*\)\s*\{',text);assert m,name
    e=m.end();d=1
    while d:d+=(text[e]=='{')-(text[e]=='}');e+=1
    return text[m.start():e]
def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    sources={}; groups={
    'runtime.inc':{'src/melee/ft/native_fighter_runtime.c':['nativeFighterRuntime_AnimationFamily','nativeFighterRuntime_PoseAssets','nativeFighterRuntime_PoseGraph','nativeFighterRuntime_PoseClip']},
    'shared.inc':{'src/melee/ft/ftanim_tree.c':['ftAnim_GetNextJointInTree'],'src/melee/ft/ftanim_tracks.c':['ftAnim_8006F4C8'],'src/melee/ft/ftanim_transition.c':['ftAnim_8006FA58','ftAnim_8006FB88'],'src/melee/ft/ftanim_skeleton.c':['ftAnim_8006FE9C','ftAnim_8006FF74'],'src/melee/ft/ftanim.c':['ftAnim_8006F994','ftAnim_80070010','ftAnim_80070108']}}
    for output,units in groups.items():
        bodies=[]
        for rel,names in units.items():
            f=a.root/rel;text=f.read_text(encoding='utf-8');sources[rel]=hashlib.sha256(f.read_bytes()).hexdigest()
            bodies.extend(body(text,name) for name in names)
        (a.output/output).write_text('\n\n'.join(bodies)+'\n',encoding='utf-8')
    (a.output/'sources.json').write_text(json.dumps(sources,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
