"""Extract unchanged production bodies for the ordinary owner console fixture."""
from pathlib import Path
import argparse, hashlib, json, re

def body(text,name):
    match=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
    if not match: raise ValueError(name)
    cursor,depth=match.end(),1
    while depth:
        depth+=(text[cursor]=='{')-(text[cursor]=='}'); cursor+=1
    return text[match.start():cursor]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=True)
    sources={
        'src/melee/ft/ftlib.c':['ftLib_8008741C','ftLib_80086A0C'],
        'src/melee/it/native_article.c':['nativeArticle_Creator'],
        'src/melee/it/it_2725.c':['HSD_JObjSetScale_2','it_80274594'],
        'src/melee/it/item.c':['itemCallbackCurrent','Item_8026AB54'],
    }
    blocks,receipts=[],{}
    for path,names in sources.items():
        text=(args.root/path).read_text()
        receipts[path]={'sha256':hashlib.sha256(text.encode()).hexdigest(),'bodies':{}}
        for name in names:
            extracted=body(text,name)
            receipts[path]['bodies'][name]={'sha256':hashlib.sha256(extracted.encode()).hexdigest(),'line':text.count('\n',0,text.index(extracted))+1}
            blocks.append('// Actual '+path+'\n'+extracted)
    for path in ['src/melee/it/native_item_owner.c','src/melee/it/native_item_owner.h','src/melee/it/types.h','src/melee/ft/native_callback_lifetime.c']:
        receipts[path]={'sha256':hashlib.sha256((args.root/path).read_bytes()).hexdigest()}
    (args.output/'item_owner_actual.inc').write_text('\n\n'.join(blocks)+'\n')
    (args.output/'source.json').write_text(json.dumps({'body_count':len(blocks),'sources':receipts,'scope':'actual owner backend and allocator body with explicit engine setup boundaries; no game'},indent=2)+'\n')

if __name__=='__main__': main()
