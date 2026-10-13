"""Extract two complete capsule engine bodies; controlled joint conversion only."""
import argparse, hashlib, json, re
from pathlib import Path

def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    sourcepath=a.source.resolve();source=sourcepath.read_text(encoding='utf-8');a.output.mkdir(parents=True,exist_ok=True);bodies=[]
    for name in ('ftColl_8007AD18','ftColl_8007B8A8'):
        m=re.search(r'(?m)^void '+name+r'\([^;{}]*\)\s*\{',source);assert m
        end=source.index('{',m.start())+1;depth=1
        while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
        bodies.append(source[m.start():end])
    (a.output/'actual.inc').write_text('\n\n'.join(bodies)+'\n',encoding='utf-8')
    (a.output/'source.json').write_text(json.dumps({'source':str(sourcepath),'sha256':hashlib.sha256(sourcepath.read_bytes()).hexdigest(),'functions':['ftColl_8007AD18','ftColl_8007B8A8'],'controlled':['lb_8000B1CC joint-to-world conversion'],'excluded':['full collision traversal','Sheik Chain policy','gameplay']},indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
