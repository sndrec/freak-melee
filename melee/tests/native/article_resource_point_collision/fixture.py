"""Extract complete actual swept-point kernel and direct resource dispatch."""
import argparse,hashlib,json,re
from pathlib import Path

def body(text,name):
    m=re.search(r'(?m)^[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',text)
    if not m:raise ValueError(name)
    e=m.end();d=1
    while d:d+=(text[e]=='{')-(text[e]=='}');e+=1
    return text[m.start():e]

KERNEL=['mpCollPrev','clamp_above','clamp_below','mpCollCheckBounding','Vec2_Interpolate','mpCollInterpolateECB','mpColl_80043268','mpCollEnd_inline2','mpCollEnd_inline','mpCollEnd','max_inline','mpColl_80043754','mpColl_80046F78_inline','mpColl_80046F78','inline4','mpColl_80048844']
PRIVATE=['mpColl_804D64A0','mpColl_804D64A4','mpColl_IsEcbTiny']

def main():
    p=argparse.ArgumentParser()
    for k in ('root','baseline','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);hashes={}
    for prefix,base in (('original_',a.baseline/'sdk'),('',a.root)):
        source=base/'src/melee/mp/mpcoll.c';text=source.read_text(encoding='utf-8');hashes[str(source)]=hashlib.sha256(source.read_bytes()).hexdigest()
        code='\n\n'.join(body(text,n) for n in KERNEL)
        if prefix:
            code=re.sub(r'\b('+ '|'.join(KERNEL+PRIVATE) +r')\b',lambda m:prefix+m[0],code)
        (a.output/('kernel_original.inc' if prefix else 'kernel_current.inc')).write_text(code+'\n',encoding='utf-8')
    resource=a.root/'src/melee/it/native_article_resource.c';text=resource.read_text(encoding='utf-8');hashes[str(resource)]=hashlib.sha256(resource.read_bytes()).hexdigest()
    declarations=[]
    for name in ('ResourceNode','ResourceCollection'):
        declarations.append(re.search(r'typedef struct '+name+r'\s*\{.*?\} '+name+';',text,re.S)[0])
    pool=re.search(r'static struct \{.*?\} resources;',text,re.S)[0];declarations.append(pool)
    names=['article_definition','collection','live_creator','part_count','node','common_pull_state','detach_borrowers','release_collection','nativeArticleResource_Release','nativeArticleResource_CancelGeneration','nativeArticleResource_CancelCreator','nativeArticleResource_CancelDefinition','nativeArticleResource_Clear','nativeArticleResource_Begin','nativeArticleResource_SweepStage','nativeArticleResource_SweepPoint']
    (a.output/'resource.inc').write_text('\n\n'.join(declarations+[body(text,n) for n in names])+'\n',encoding='utf-8')
    vectors=a.root/'src/melee/lb/lbvector.c';text=vectors.read_text(encoding='utf-8');hashes[str(vectors)]=hashlib.sha256(vectors.read_bytes()).hexdigest()
    (a.output/'vectors.inc').write_text('\n\n'.join(body(text,n) for n in ['lbVector_Add','lbVector_Diff'])+'\n',encoding='utf-8')
    (a.output/'sources.json').write_text(json.dumps(hashes,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
