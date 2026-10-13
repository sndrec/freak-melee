"""Conservative native stage storage/reference audit, including header/local statics.

This finds write and pointer-escape candidates. It does not prove immutability,
interprocedural alias closure, or full snapshot coverage. Ambiguous eligible source locations are reported; external declarations are
outside this inventory.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
UNITS="grbattle grlast groldpupupu grgreens grgarden gryorster grstory grflatzone groldyoshi groldkongo grizumi grrcruise grgreatbay grshrine gronett grpstadium grzebes grkraid grfourside grinishie1 grinishie2 grpura grbigblue gricemt grcastle grkongo grcorneria grmutecity grvenom grzakogenerator grfzerocar grlib".split()

def location(loc):
    return loc.get('expansionLoc',loc.get('spellingLoc',loc))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--unit',choices=UNITS,action='append')
    args=parser.parse_args();out=args.output.resolve()
    if out.exists():parser.error('Use a fresh output directory')
    out.mkdir(parents=True)
    directory=ROOT/'src/melee/gr'
    headers={p:p.read_bytes() for p in directory.glob('*.h')}
    rows=[];unresolved=[];commands=[]
    audited=args.unit or UNITS
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    input_paths=[Path(__file__).resolve(),args.compiler]+list(headers)+[directory/(u+'.c') for u in audited]
    input_hashes={str(p):sha(p) for p in input_paths}
    flags=['-std=c11','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-Isrc','-Iextern/dolphin/include','-fsyntax-only','-Xclang','-ast-dump=json']
    for unit in args.unit or UNITS:
        source=directory/(unit+'.c');files={source:source.read_bytes(),**headers}
        cmd=[str(args.compiler),*flags,str(source)]
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(unit+'.log')).write_bytes(result.stderr);commands.append(dict(unit=unit,command=cmd,exit=result.returncode))
        if result.returncode:raise RuntimeError(unit+': '+result.stderr.decode(errors='replace')[-2000:])
        ast=json.loads(result.stdout);decls={};aliases={};functions=[]
        def resolve(n):
            loc=location(n.get('loc',{}));off=loc.get('offset');name=n.get('name','').encode()
            if not name or not isinstance(off,int):return None
            explicit=loc.get('file')
            candidates=[p for p,b in files.items() if b[off:off+len(name)]==name]
            if explicit:
                path=Path(explicit);path=path if path.is_absolute() else ROOT/path
                path=path.resolve()
                if path in candidates:return path
                if path in files:unresolved.append(dict(unit=unit,name=n.get("name"),file=str(path),reason="declared token does not match source offset"))
                return None
            if not loc.get('includedFrom') and source in candidates:return source
            if len(candidates)==1:return candidates[0]
            if len(candidates)>1:unresolved.append(dict(unit=unit,name=n.get("name"),candidates=[str(p) for p in candidates],reason="ambiguous byte-offset location"))
            return None
        def register(n,fn):
            if n.get('kind')=='FunctionDecl' and any(c.get('kind')=='CompoundStmt' for c in n.get('inner',[])):
                if resolve(n):functions.append(n)
                fn=n.get('name')
            if n.get('kind')=='VarDecl' and n.get('storageClass')!='extern' and (not fn or n.get('storageClass')=='static'):
                path=resolve(n)
                if path:
                    loc=location(n['loc']);off=loc['offset'];b=files[path]
                    begin=location(n['range']['begin']).get('offset',off);last=location(n['range']['end']);end=last.get('offset',off)+last.get('tokLen',len(n['name']))
                    decls[n['id']]=dict(name=n['name'],file=str(path.relative_to(ROOT)),line=b[:off].count(b'\n')+1,type=n['type']['qualType'],scope=fn or 'file',storage=n.get('storageClass'),declaration=b[begin:end].decode('utf-8',errors='replace'),uses=[])
            for child in n.get('inner',[]):register(child,fn)
        register(ast,None)
        def references(n):
            ids=set()
            if n.get('kind')=='DeclRefExpr':ids.add(n.get('referencedDecl',{}).get('id'))
            for child in n.get('inner',[]):ids.update(references(child))
            return ids
        # Flow-insensitive pointer aliases: conservative union, never an
        # immutability claim. Scalar copies do not alias their source storage.
        bindings=[]
        def bindings_walk(n):
            if n.get('kind')=='VarDecl' and '*' in n.get('type',{}).get('qualType','') and n.get('inner'):
                bindings.append((n['id'],references(n)))
            if n.get('kind')=='BinaryOperator' and n.get('opcode')=='=':
                children=n.get('inner',[])
                if len(children)==2 and children[0].get('kind')=='DeclRefExpr' and '*' in children[0].get('type',{}).get('qualType',''):
                    bindings.append((children[0]['referencedDecl']['id'],references(children[1])))
            for child in n.get('inner',[]):bindings_walk(child)
        for fn in functions:bindings_walk(fn)
        for _ in range(len(bindings)+1):
            changed=False
            for target,inputs in bindings:
                roots=set(i for i in inputs if i in decls)
                for i in inputs:roots.update(aliases.get(i,set()))
                old=aliases.setdefault(target,set());before=len(old);old.update(roots);changed|=len(old)!=before
            if not changed:break
        def uses(n,parents,fn):
            if n.get('kind')=='DeclRefExpr':
                ref=n.get('referencedDecl',{}).get('id');roots=aliases.get(ref,set())|({ref} if ref in decls else set())
                if roots:
                    evidence=set()
                    for parent,index in parents:
                        kind=parent.get('kind');op=parent.get('opcode')
                        if kind in ['BinaryOperator','CompoundAssignOperator'] and op in ['=','+=','-=','*=','/=','|=','&=','^=','<<=','>>='] and index==0:evidence.add('write-or-pointee-write')
                        if kind=='UnaryOperator' and op in ['++','--']:evidence.add('write-or-pointee-write')
                        if kind=='UnaryOperator' and op=='&':evidence.add('address-escape')
                        if kind=='CallExpr' and index>0:evidence.add('call-argument')
                        if kind=='ReturnStmt':evidence.add('return-escape')
                    loc=location(n.get('range',{}).get('begin',{}));off=loc.get('offset',0)
                    item=dict(function=fn.get('name'),line=files[resolve(fn)][:off].count(b'\n')+1,evidence=sorted(evidence) or ['read'],through_alias=ref not in decls)
                    for root in roots:decls[root]['uses'].append(item)
            for index,child in enumerate(n.get('inner',[])):uses(child,parents+[(n,index)],fn)
        for fn in functions:uses(fn,[],fn)
        records=list(decls.values())
        for row in records:
            row['candidate']=any(u['evidence']!=['read'] for u in row['uses'])
        rows.append(dict(unit=unit,source=str(source),sha256=hashlib.sha256(files[source]).hexdigest(),variables=records))
        print(json.dumps(dict(unit=unit,variables=len(records),candidates=sum(r['candidate'] for r in records))),flush=True)
    assert all(sha(Path(p))==digest for p,digest in input_hashes.items()),'audit inputs changed'
    receipt=dict(input_sha256=input_hashes,status='AUDIT_WITH_LOCATION_GAPS' if unresolved else 'AUDIT',units=rows,commands=commands,unresolved=unresolved,limits=['Pointer aliases are flow-insensitive candidates; no interprocedural or field-sensitive ownership proof.','Only eligible stage-unit/header storage is inventoried; external module allocations and pointees require their own audit.','No game launch or snapshot acceptance.'])
    (out/'inventory.json').write_text(json.dumps(receipt,indent=2)+'\n')
if __name__=='__main__':main()
