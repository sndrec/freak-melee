"""Mechanically extract complete stock constructor and scale/attachment bodies."""
from pathlib import Path
import argparse
import hashlib
import json
import re
SYMBOLS = {
 'melee/it/kinds/itsamusgrapple.c': ['itSamusGrapple_Logic53_Spawned', 'samus_grapple_fighter_compare',
    'samus_grapple_init_link', 'it_802B743C', 'samus_grapple_setup_tail',
    'it_802B75FC', 'it_802B7C18', 'itSamusGrapple_Logic53_PickedUp'],
 'melee/it/kinds/itlinkhookshot.c': ['it_802A2474', 'it_802A24A0', 'itLinkHookshot_ApplyOwnerScale'],
 'melee/it/item.c': ['Item_8026AB54'],
 'melee/it/it_26B1.c': ['it_8026BAE8'],
}

def body(source, name):
    found = re.search(r'^[A-Za-z_][A-Za-z0-9_ \t*]*[ \t*]'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source,re.M)
    if not found: raise ValueError('missing complete body: '+name)
    end=source.index('{',found.start())+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[found.start():end]

def extract(sdk, output):
    records=[];sources={};chunks=[]
    # Dependency bodies precede consumers; no function/body rewriting.
    for relative in ['melee/it/kinds/itlinkhookshot.c','melee/it/item.c','melee/it/it_26B1.c','melee/it/kinds/itsamusgrapple.c']:
        path=sdk/'src'/relative;text=path.read_text(encoding='utf-8')
        sources[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
        for symbol in SYMBOLS[relative]:
            value=body(text,symbol);chunks.append(value)
            records.append({'source':str(path),'symbol':symbol,'body_sha256':hashlib.sha256(value.encode()).hexdigest()})
    output.mkdir(parents=True,exist_ok=True)
    (output/'original.inc').write_text('\n\n'.join(chunks)+'\n',encoding='utf-8',newline='\n')
    (output/'originals.json').write_text(json.dumps({'sources':sources,'bodies':records},indent=2)+'\n',encoding='utf-8')
    return sources

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('sdk',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();extract(a.sdk.resolve(),a.output.resolve())
