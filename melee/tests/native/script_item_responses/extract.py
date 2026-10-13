"""Complete native response kernels; controlled stage/presentation leaves."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
def module(path,name):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def extract(root,out):
 root=Path(root);out=Path(out)
 base=root/'tests/native/script_target_queries/extract.py';hashes=module(base,'response_guards').extract(root,out);hashes[str(base)]=hashlib.sha256(base.read_bytes()).hexdigest()
 path=root/'tests/native/script_form_feedback/extract.py';m=module(path,'body');hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def body(text,name):
  return m.body(re.sub(r"//[^\n]*|/\*.*?\*/","",text,flags=re.S),name)
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 common=['product_xy','sqrtf_accurate_local','return_sqrt_value','it_8027781C','it_80276D9C','it_80277C40','it_80276FC4']
 kernels=[body(source('src/melee/it/itmaplib.c'),name) for name in common]
 kernels += [body(source('src/melee/it/it_2725.c'),name) for name in ['itColl_BounceOffVictim','it_8027321C']]
 kernels += [body(source('src/melee/it/ithitbox.c'),'it_80275640')]
 kernels += [body(source('src/melee/it/native_item_owner.c'),name) for name in ['nativeItem_CaptureCreator','nativeItem_Creator']]
 kernels += [body(source('src/sysdolphin/baselib/class.c'),'hsdObjIsDescendantOf')]
 kernels += [body(source('src/melee/ft/ftchangeparam.c'),'ftCo_CalcYScaledKnockback')]
 kernels += [body(source('src/melee/ft/ftidentity.c'),'ftLib_80086960')]
 kernels += [body(source('src/melee/it/itcoll.c'),'it_80272460')]
 kernels += [body(source('src/melee/lb/lbvector.c'),name) for name in ['lbVector_Add_xy','lbVector_Mirror','lbVector_NormalizeXY']]
 # Existing native math approximation is supplied by the actual native headers.
 preamble="""
#include <melee/it/inlines.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_article.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/it/it_2725.h>
#include <melee/it/itmaplib.h>
#include <melee/it/ithitbox.h>
#include <melee/it/iteffect.h>
#include <melee/it/it_26B1.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mpcoll.h>
#include <melee/it/itcoll.h>
#include <math.h>
const Vec3 it_803B8570={0},it_803B857C={0};
ItemCommonData response_common;ItemCommonData* it_804D6D28=&response_common;
unsigned response_sound,response_effect,response_damage,response_geometry;uint32_t response_order;
static void observe(unsigned code){response_order=response_order*10+code;}
void Item_8026AE84(Item* i,enum_t s,u8 volume,u8 pan){(void)i;(void)s;(void)volume;(void)pan;++response_sound;observe(1);}
int lbAudioAx_80023870(int a,int b,int c,int d){(void)a;(void)b;(void)c;(void)d;abort();}
void mpColl_SetECBSource_Fixed(CollData* c,HSD_GObj* o,float t,float b,float r,float l){(void)c;(void)o;(void)t;(void)b;(void)r;(void)l;++response_geometry;observe(4);}
void mpCollSetFacingDir(CollData* c,int f){c->facing_dir=f;}
#define SPEED(name) bool name(CollData* c,Vec3* v){(void)c;*v=(Vec3){0.25f,-0.5f,0};return true;}
SPEED(mpCollGetSpeedLeftWall) SPEED(mpCollGetSpeedRightWall) SPEED(mpCollGetSpeedCeiling) SPEED(mpCollGetSpeedFloor)
void it_80278800(Item_GObj* o,s32 effect,s32 bone,Vec3* p,Vec3* r,s32 flags,f32 scale){(void)o;(void)bone;(void)r;(void)flags;(void)scale;if(effect!=0x405||!p)abort();++response_effect;observe(2);}
static struct Fighter_804D6524_t scale_policy={.x4=0.75f};struct Fighter_804D6524_t* Fighter_804D6524=&scale_policy;
float ft_80089228(Fighter* f,int id,int instance,float damage){(void)f;(void)id;(void)instance;return damage;}
Fighter_GObj* nativeArticle_Creator(Item_GObj* o){(void)o;return NULL;}
Fighter_GObj* nativeModelEffect_Creator(EF_Effect* o){(void)o;return NULL;}
"""
 # Current native routines include every body, with one explicit post-kernel
 # test observer. It does not replace native work or imply native reentry.
 for i,kernel in enumerate(kernels):
  if re.search(r'\b(?:itColl_BounceOffVictim|it_80276FC4)\s*\(',kernel[:kernel.index('{')]):
   at=kernel.rfind('}');arg='gobj' if 'itColl_BounceOffVictim' in kernel[:kernel.index('{')] else 'item_gobj'
   kernels[i]=kernel[:at]+'\nresponse_kernel_probe('+arg+');\n'+kernel[at:]
 # Record complete actual hitbox setter calls without replacing its work.
 for i,kernel in enumerate(kernels):
  if re.search(r'\bit_80272460\s*\(',kernel[:kernel.index('{')]):
   at=kernel.index('{')+1;kernels[i]=kernel[:at]+'\n++response_damage;observe(3);\n'+kernel[at:]
 actor=source('src/melee/it/native_script_item_actor.c')
 old='return alive(actor)?resolve_relation(&actor->attachment_owner):NULL;'
 if actor.count(old)!=1:raise RuntimeError('Attachment getter extraction changed')
 actor=actor.replace(old,'Fighter_GObj* result=alive(actor)?resolve_relation(&actor->attachment_owner):NULL;response_relation_probe(object);return result;')
 (out/'actor.c').write_text('#include \"fixture.h\"\n'+actor)
 # Pin the pre-batch native oracle to its actual Git source, preserving exactly
 # the original composition. Hash the bytes and receipt the commit explicitly.
 commit=subprocess.run(['git','rev-parse','HEAD'],cwd=root,capture_output=True,text=True,check=True).stdout.strip()
 reference=[]
 for relative,name in [('src/melee/it/it_2725.c','itColl_BounceOffVictim'),('src/melee/it/itmaplib.c','it_80276FC4')]:
  text=subprocess.run(['git','show',commit+':'+relative],cwd=root,capture_output=True,text=True,check=True).stdout
  body_text=body(text,name).replace(name,'response_reference_bounce' if name=='itColl_BounceOffVictim' else 'response_reference_stage')
  reference.append(body_text)
 (out/'reference_source.json').write_text(__import__('json').dumps({'commit':commit,'bodies_sha256':hashlib.sha256('\n'.join(reference).encode()).hexdigest()},indent=2))
 (out/'native_responses.inc').write_text(preamble+'\n'+'\n'.join(kernels)+'\n'+'\n'.join(reference)+'\n')
 own=Path(__file__).parent/'unselected.inc';hashes[str(own)]=hashlib.sha256(own.read_bytes()).hexdigest();(out/'unselected.inc').write_bytes(own.read_bytes())
 return hashes
