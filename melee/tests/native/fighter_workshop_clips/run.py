from pathlib import Path
import argparse,hashlib,json,re,subprocess,shutil
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];UI=ROOT/'src/melee/ui'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
 start=text.index(name);brace=text.index('{',start);depth=1;i=brace+1
 while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True)
 core=(UI/'native_shell_workshop_editor.cpp').read_text(encoding='utf-8');actions=(UI/'native_shell_workshop_editor_actions.cpp').read_text(encoding='utf-8');sessions=(UI/'native_shell_workshop_editor_session.cpp').read_text(encoding='utf-8');clips=(UI/'native_shell_workshop_editor_clips.cpp').read_text(encoding='utf-8')
 extracts='namespace WorkshopEditor {\n'+'\n\n'.join(body(core,n) for n in ['Rml::ElementFormControlInput* field(', 'void set_text(', 'void update_history_buttons(', 'void select_mode('])+'\n\n'+'\n\n'.join(body(actions,n) for n in ['void select_action(', 'void apply_history('])+'\n}\n'
 branches=[body(core,n) for n in ['if (std::strncmp(id, "workshop-graph-key-", 19) == 0)', 'if (std::strncmp(id, "workshop-dope-key-", 18) == 0)', 'if (std::strcmp(id, "workshop-graph-select-all") == 0)']]
 extracts += 'namespace WorkshopEditor {\nbool fixture_clip_click(const char* id,bool shift) {\n'+'\n'.join(branches)+'\nreturn false;\n}\n}\n'
 (out/'actual.h').write_text(extracts,encoding='utf-8')
 ids=sorted(set(re.findall(r'"(workshop-[a-z-]+)"',extracts+sessions+clips)))
 input_ids={'workshop-animation-search','workshop-action-search','workshop-current-frame'};areas={'workshop-script-source','workshop-parameters-source'}
 markup='<rml><head><style>body {font-family:arial;font-size:14px;width:800px;height:600px;} input {width:150px;} textarea {width:200px;height:50px;} scrollbarvertical {width:10px;} scrollbarhorizontal {height:10px;} </style></head><body>'
 for name in ids:
  tag='input' if name in input_ids else 'textarea' if name in areas else 'div';markup+=f'<input id="{name}"/>' if tag=='input' else f'<{tag} id="{name}"></{tag}>'
 markup+='</body></rml>';(out/'document.rml').write_text(markup,encoding='utf-8')
 units=[HERE/'test.cpp',UI/'native_shell_workshop_editor_clips.cpp',UI/'native_shell_workshop_editor_session.cpp',UI/'native_shell_workshop_labels.cpp',ROOT/'src/melee/ft/native_fighter_sources.cpp']
 sources=units+[HERE/'run.py',UI/'native_shell_workshop_editor.cpp',UI/'native_shell_workshop_editor_actions.cpp',UI/'native_shell_workshop_editor_internal.h',UI/'native_shell_workshop_editor.h',ROOT/'src/melee/ft/native_fighter_animation_editor.h',ROOT/'src/melee/ft/native_fighter_project.h']
 inputs={str(x):sha(x) for x in sources}
 db=json.loads((ROOT/'build/native/compile_commands.json').read_text(encoding='utf-8'));entry=next(x for x in db if Path(x['file']).name=='native_shell_workshop_editor.cpp');args=re.split(r'\s+',entry['command']);args=args[:args.index('-o')]
 if '-include' in args:pos=args.index('-include');del args[pos:pos+2]
 libs=[ROOT/'build/native/rmlui.lib',ROOT/'build/native/_deps/freetype-build/freetype.lib',ROOT/'build/native/_deps/zlib-build/zlib.lib']
 args += ['-I'+str(UI),'-I'+str(out),*map(str,units),*map(str,libs),'-luser32','-Wl,/OPT:REF','-o',str(out/'test.exe')]
 batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'+' '.join('"'+x+'"' if ' ' in x else x for x in args)+'\n',encoding='utf-8')
 shutil.copyfile(ROOT/'build/native/zlib1.dll',out/'zlib1.dll')
 for stage,command in [('compile',['cmd','/d','/c',str(batch)]),('test',[str(out/'test.exe'),str(out/'document.rml')])]:
  r=subprocess.run(command,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=180);(out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
  if r.returncode:raise RuntimeError(stage+'\n'+r.stdout+r.stderr)
 for name,expected in inputs.items():assert sha(Path(name))==expected,name
 receipt={'status':'PASS','result':json.loads(r.stdout),'inputs':inputs,'libraries':{str(n):sha(n) for n in libs},'compiler':{str(Path(args[0])):sha(Path(args[0]))},'generated':{n:sha(out/n) for n in ['actual.h','document.rml','compile.cmd']},'boundaries':['actual clips/session/labels/ScriptFiles; exact select_mode/select_action/apply_history bodies','real Rml document controls/layout/classes; controlled clip decode and graph/model/preview rendering','no import/build/save/game/runtime demo claim']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(r.stdout)
if __name__=='__main__':main()
