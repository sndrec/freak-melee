"""Complete actual audio dispatch/stop bodies; immutable reference and live policy."""
import argparse, hashlib, json, re
from pathlib import Path

def body(text,name):
    match=re.search(r'(?m)^[\w][\w\s*]*\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',text)
    if not match: raise ValueError(name)
    i=match.end();depth=1
    while depth: depth+=(text[i]=='{')-(text[i]=='}');i+=1
    return text[match.start():i]

ENTRIES=['ft_PlaySFX','ft_800881D8','ft_80088328','ft_80088478','ft_80088510','ft_800885A8','ft_80088640']
STOPS=['ft_80088770','ft_800887CC','ft_80088828','ft_80088884','ft_800888E0','ft_8008893C','ft_80088998']
COMMANDS=['ftAction_80071B50','ftAction_80071FC8','ftAction_80072320']
def main():
    parser=argparse.ArgumentParser()
    for key in ('root','baseline','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);sources={}
    for prefix,root in [('original_',args.baseline/'sdk'),('',args.root)]:
        files={}
        for name in ['ft_0877.c','ft_0881.c','ftaction.c']:
            path=root/'src/melee/ft'/name;files[name]=path.read_text(encoding='utf-8');sources[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
        code='\n\n'.join(body(files['ft_0881.c'],name) for name in STOPS)
        code+='\n\n'+body(files['ft_0877.c'],'ft_PlaySFX')
        code+='\n\n'+'\n\n'.join(body(files['ft_0881.c'],name) for name in ENTRIES[1:])
        code+='\n\n'+'\n\n'.join(body(files['ftaction.c'],name) for name in COMMANDS)
        if prefix:code=re.sub(r'\b('+'|'.join(ENTRIES+STOPS+COMMANDS)+r')\b',lambda m:prefix+m[0],code)
        (args.output/('original.inc' if prefix else 'candidate.inc')).write_text(code+'\n',encoding='utf-8')
    path=args.root/'src/melee/ft/native_fighter_runtime.c';text=path.read_text(encoding='utf-8');sources[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    capacity=re.search(r'enum\s*\{\s*NATIVE_FIGHTER_FAMILY_CAPACITY\s*=.*?\};',text,re.S)[0]
    (args.output/'capacity.inc').write_text(capacity+'\n',encoding='utf-8')
    (args.output/'resolver.inc').write_text(body(text,'nativeFighterRuntime_Definition')+'\n'+body(text,'nativeFighterRuntime_SoundMuteChannels')+'\n',encoding='utf-8')
    (args.output/'sources.json').write_text(json.dumps(sources,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
