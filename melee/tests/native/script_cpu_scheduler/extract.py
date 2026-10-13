"""Mechanical native CPU body/prefix extraction; explicit controlled large tails."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('cpu_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_item_scheduler/extract_scheduler.py';spec=importlib.util.spec_from_file_location('cpu_native',path);n=importlib.util.module_from_spec(spec);spec.loader.exec_module(n)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 common=n.native_only(source('src/melee/ft/kinds/ftCommon/ftCo_0A01.c'))
 attack=n.native_only(source('src/melee/ft/ftcpuattack.c'))
 properties=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','cpu_guard_error',m.body(properties,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 bodies=[m.body(common,name) for name in ['ftCo_CpuRecoverDiagonally','ftCo_800A0CB0','ftCo_IsGrabbing','ftCo_800AA320','ftCo_800B3900']]
 bodies += [m.body(attack,'ftCo_800B630C')]
 for name,tail,marker in [('ftCo_800B0AF4','output_tails','    struct CpuFighter* data = &fp->cpu;'),('ftCo_800B2AFC','mode_tails','    UNUSED u8 pad_high[8];')]:
  body=m.body(common,name);at=body.index(marker);bodies.append(body[:at]+'    ++'+tail+';\n}\n')
 # Exact initializer final hook/default segment, after its native reset/widths.
 body=m.body(common,'ftCo_800A101C');at=body.index('    if(nativeScriptFighter_IsAttached(arg0->gobj))')
 bodies.append('void cpu_initialize_tail(Fighter* arg0)\n{\n    struct CpuFighter* temp_r30=&arg0->cpu;\n'+body[at:])
 recovery=m.body(common,'ftCo_800A96B8');start=recovery.index('    if(nativeScriptFighter_IsAttached(fp->gobj))');end=recovery.index('    if (authored != NULL',start)
 bodies.append('void cpu_recovery_dispatch(Fighter* fp)\n{\n'+recovery[start:end]+'    ftCo_CpuRecoverDiagonally(fp);\n}\n')
 prototypes='\n'.join(b[:b.index('{')].rstrip()+';' for b in bodies)
 (out/'native_cpu.inc').write_text(prototypes+'\n'+'\n'.join(bodies)+'\n')
 return hashes
