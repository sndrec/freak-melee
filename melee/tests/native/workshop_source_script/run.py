"""Exact Workshop process orchestration against the real source content tool."""
import argparse,hashlib,json,subprocess,shutil,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--tool',type=Path,required=True);p.add_argument('--project',type=Path,required=True);p.add_argument('--script',type=Path,default=ROOT/'examples/mario_full.fms');p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);project=out/'project';shutil.copytree(a.project,project)
 source_path=ROOT/'src/melee/ui/native_shell_workshop_editor_script.cpp';source=source_path.read_text()
 spec=importlib.util.spec_from_file_location('source_body',ROOT/'tests/native/script_item_spawn/extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
 (out/'actual_source_tool.h').write_text(extractor.body(source,'run_script_tool')+'\n')
 shutil.copyfile(a.tool,out/'native_content_tool.exe')
 for dll in a.tool.parent.glob('*.dll'):shutil.copyfile(dll,out/dll.name)
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=[source_path,HERE/'test.cpp',HERE/'run.py',a.tool,a.script]+[x for x in a.project.rglob('*') if x.is_file()];before={str(x):sha(x) for x in inputs}
 project_before={str(x.relative_to(project)):sha(x) for x in project.rglob('*') if x.is_file()}
 command=[str(a.compiler),'-std=c++17','-O2','-D_CRT_SECURE_NO_WARNINGS','-I'+str(out),str(HERE/'test.cpp'),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe'),str(project),str(a.script)])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-6000:])
 for path,digest in before.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed '+path)
 project_after={str(x.relative_to(project)):sha(x) for x in project.rglob('*') if x.is_file()}
 changed=[name for name in project_before.keys()|project_after.keys() if project_before.get(name)!=project_after.get(name)]
 if changed!=['behavior.fms']:raise RuntimeError('Unexpected project writes: '+repr(changed))
 receipt={'status':'PASS','summary':json.loads(r.stdout),'input_sha256':before,'private_project_changed_files':changed,'scope':'exact production Workshop source staging/hidden child/tool/error mapping and cleanup with real CLI source-check/script-build plus actual candidate project assets; no game/GUI Apply acceptance','limitations':['Rml layout/session integration tested separately','project is an independent archive copy; accepted live preview belongs to root game acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
