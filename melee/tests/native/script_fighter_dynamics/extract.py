"""Extract exact native scheduler bodies and the pinned pre-batch reference."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'script_fighter_scheduler'
spec=importlib.util.spec_from_file_location('dynamics_scheduler',BASE/'extract_scheduler.py')
substrate=importlib.util.module_from_spec(spec);spec.loader.exec_module(substrate)
REFERENCE='45ac45dce93fd6a8540f1686fcd14e445341787b'

def extract(root,out):
 hashes=substrate.extract(root,out/'actual_scheduler.h')
 body=substrate.reference.reference.body;native=substrate.reference.native_only
 path=root/'src/melee/ft/ftdynamics.c';source=native(path.read_text())
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 names=['nativeDynamics_RootCurrent','nativeDynamics_Current','nativeDynamics_CaptureActor','nativeDynamics_Capture','ftCo_8009CB40_inline','ftCo_8009CB40','ftCo_8009E4A8','ftCo_8009E614','ftCo_8009DD94','ftCo_8009E0A8','ftCo_UnloadDynamicBones','ftCo_8009E7B4']
 struct=source[source.index('typedef struct NativeDynamicsLifetime'):source.index('} NativeDynamicsLifetime;')+len('} NativeDynamicsLifetime;')]
 headers='''#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/inlines.h>
#include <melee/gr/stage.h>
#include <melee/gr/ground.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
'''
 (out/'actual_dynamics.h').write_text(headers+struct+'\n'+'\n'.join(body(source,n) for n in names)+'\n')
 old=subprocess.run(['git','show',REFERENCE+':src/melee/ft/ftdynamics.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 old=native(old);old_names=['ftCo_8009CB40_inline','ftCo_8009CB40','ftCo_8009E4A8','ftCo_8009E614','ftCo_8009DD94','ftCo_8009E7B4']
 reference='\n'.join(body(old,n) for n in old_names)
 for n in sorted(old_names,key=len,reverse=True):reference=re.sub(r'\b'+n+r'\b','Reference_'+n,reference)
 (out/'reference_dynamics.h').write_text(reference+'\n')
 (out/'reference_strict.c').write_text(headers+reference+'\n')
 boundary=(BASE/'boundary.c').read_text();props=root/'src/melee/ft/native_script_binding_properties.c'
 boundary=boundary.replace(body(boundary,'lb_8000FD18'),'')
 hashes[str(props)]=hashlib.sha256(props.read_bytes()).hexdigest()
 for n in ['nativeScriptBindings_Context','nativeScriptBindings_SourceAlive']:boundary=boundary.replace(body(boundary,n),'')
 boundary=boundary.replace(body(boundary,'nativeScriptBindings_Write'),'''bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)s;(void)d;if(c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX||n||v->type!=NativeScriptType_F32||scheduler_fail_write)return false;((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;dynamics_observe(c->source);return true;}''')
 guards='\n'.join(re.sub(r'\berror\b','dynamics_guard_error',body(props.read_text(),n)) for n in ['error','nativeScriptBindings_Context','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 cls=root/'src/sysdolphin/baselib/class.c';hashes[str(cls)]=hashlib.sha256(cls.read_bytes()).hexdigest()
 collision=root/'src/melee/ft/ftcoll.c';hashes[str(collision)]=hashlib.sha256(collision.read_bytes()).hexdigest()
 lb=root/'src/melee/lb/lb_00F9.c';hashes[str(lb)]=hashlib.sha256(lb.read_bytes()).hexdigest()
 joint=root/'src/sysdolphin/baselib/jobj.c';hashes[str(joint)]=hashlib.sha256(joint.read_bytes()).hexdigest()
 leaves='''
HSD_JObjInfo hsdJObj;
static struct DynamicsData* cur_data;
int lb_80011ABC(void){extern unsigned dynamics_wind_queries;++dynamics_wind_queries;return -1;}
void ftAnim_8006EED4(Fighter* f,Fighter_Part i,FigaTree* t,float a,float b){(void)f;(void)i;(void)t;(void)a;(void)b;abort();}
void ftAnim_8006EBE8(HSD_GObj* o,float a,float b,float c){(void)o;(void)a;(void)b;(void)c;abort();}
void ftAnim_8006E9B4(HSD_GObj* o){(void)o;abort();}
void HSD_JObjSetupMatrixSub(HSD_JObj* j){j->flags&=~JOBJ_MTX_DIRTY;extern unsigned dynamics_matrices;++dynamics_matrices;}
void lb_8000B1CC(HSD_JObj* j,Vec3* a,Vec3* b){(void)j;*b=*a;}
void lb_8001044C(DynamicsDesc* d,void* p,int n,float y,bool f,Fighter_Part b,int i,bool g){dynamics_step(d,p,n,y,f,b,i,g);}
'''
 # Existing skeleton substrate owns unrelated HSD animation leaves only.
 includes='#include "fixture.h"\n'+headers
 current=body(cls.read_text(),'hsdObjIsDescendantOf')+'\n'+body(collision.read_text(),'ftColl_8007AF60')+'\n'+body(lb.read_text(),'lb_8000FD18')+'\n'+body(joint.read_text(),'HSD_JObjClearFlags')
 (out/'boundary.c').write_text(includes+boundary+'\n'+guards+'\n'+leaves+'\n'+current+'\n#include "actual_dynamics.h"\n#include "reference_dynamics.h"\n')
 (out/'strict.c').write_text(headers+struct+'\nvoid ftCo_8009CB40(Fighter*,ssize_t,bool,FigaTree*);\nvoid ftCo_8009E4A8(Fighter*);\n'+'\n'.join(body(source,n) for n in names if n not in ['ftCo_8009CB40_inline','ftCo_8009CB40','ftCo_8009E4A8'])+'\n')
 (out/'reference_revision.txt').write_text(REFERENCE+'\n')
 return hashes
