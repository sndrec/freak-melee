"""Static form binding inventory: production lexer + actual descriptor dump.

Selector/receiver paths remain distinct and unresolved; this is not whole-source
type validation. Archived contract hints are excluded from live admission.
"""
import argparse, hashlib, importlib.util, json, subprocess, sys
from collections import defaultdict
from pathlib import Path
from types import SimpleNamespace
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent; ROOT=HERE.parents[2]

def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
    out.mkdir(parents=True)
    # Compile the exact source descriptor table and Descriptor function only.
    # No runtime behavior is called or substituted by this read-only dump.
    original=ROOT/'src/melee/ft/native_script_bindings.c'
    catalog_text=original.read_text()
    end=catalog_text.index('static bool error(')
    table=out/'catalog_table.c';table.write_text(catalog_text[:end])
    flags=[a.compiler,'-std=c11','-O2','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/melee/ft'),'-I'+str(ROOT/'extern/dolphin/include')]
    flags+=['-I'+str(ROOT/'src/melee/ft')]
    units={'lexer':[ROOT/'tests/native/script_binding_inventory/lexer_census.c',ROOT/'src/melee/ft/native_script_expression.c'],'catalog':[HERE/'catalog.c',table,ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c']}
    for name,files in units.items():
        cmd=flags+list(map(str,files))+['-o',str(out/(name+'.exe'))]
        r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(name+'-compile.log')).write_text(r.stdout+r.stderr)
        if r.returncode:raise RuntimeError(r.stderr)
    rows=json.loads(subprocess.run([str(out/'catalog.exe')],capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout)
    by_id={row['id']:row for row in rows}
    def full(row):
        if not row['parent']:return row['name']
        if row['parent'] not in by_id:return {0x3f000001:'state',0x3f000003:'item'}.get(row['parent'],'semantic:'+str(row['parent']))+'.'+row['name']
        return full(by_id[row['parent']])+'.'+row['name']
    catalog=defaultdict(list)
    for row in rows:catalog[full(row)].append(row)
    spec=importlib.util.spec_from_file_location('audit',ROOT/'tools/audit_native_script_bindings.py');audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
    audit.subprocess=SimpleNamespace(run=lambda *args,**kwargs:subprocess.run(*args,creationflags=subprocess.CREATE_NO_WINDOW,**kwargs))
    result={'scope':'shared lexer structural census + actual current native descriptors; no whole-program admission','catalog':dict(catalog),'files':[]}
    for name in ['zelda_full.fms','sheik_full.fms']:
        report=audit.audit_source(ROOT/'examples'/name,out/'lexer.exe',{},audit.source_evidence())
        grouped=defaultdict(lambda:{'uses':0,'lines':[],'owners':set(),'live_catalog':[]})
        for use in report['occurrences']:
            if use['classification']!='native':continue
            op=use['operation'];row=grouped[op];row['uses']+=1;row['lines'].append(use['line']);row['owners'].add(use['owner']);row['live_catalog']=catalog.get(op,[])
        result['files'].append({'path':report['path'],'sha256':report['sha256'],'unparsed_statements':report['unparsed_statements'],'operations':{k:dict(v,owners=sorted(v['owners'])) for k,v in sorted(grouped.items())},'occurrences':report['occurrences']})
    files=set(f for group in units.values() for f in group)|{original,HERE/'run.py',ROOT/'tools/audit_native_script_bindings.py',ROOT/'src/melee/ft/native_script_ir.h',ROOT/'src/melee/ft/native_script_bindings.h'}|{ROOT/'examples'/n for n in ['zelda_full.fms','sheik_full.fms']}
    result['input_sha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(files)}
    (out/'inventory.json').write_text(json.dumps(result,indent=2)+'\n')
    for f in result['files']:
        print(f['path'],len(f['operations']),'native paths',len(f['unparsed_statements']),'unparsed lines')
        for k,v in f['operations'].items():
            if not v['live_catalog']:print('NO EXACT CATALOG',k,','.join(map(str,v['lines'][:4])))
if __name__=='__main__':main()
