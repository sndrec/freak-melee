/* Controlled registry/graphics leaves; preparation and WIC pixels are real. */
#include "boundary.h"
#include <melee/it/types.h>
#include <melee/it/itdraw.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <dolphin/gx.h>
#include <stdlib.h>
#include <string.h>

bool fixture_idle=true,fixture_camera=true;
uint32_t fixture_references;
const NativeScriptCompileBank* fixture_selected,*fixture_retained;
const NativeScriptItemProgram* fixture_spec;
VisualTrace visual_trace;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
HSD_GObj* HSD_GObj_804D7818;
__typeof__(HSD_GObj_DelayedProcInfo) HSD_GObj_DelayedProcInfo;
static HSD_CObj camera;

static void require(bool value){if(!value)abort();}
void visualFixture_Reset(void){visual_trace=(VisualTrace){0};fixture_camera=true;}
bool nativeScriptRuntime_Idle(void){return fixture_idle;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{
    static NativeScriptFighterProgram selected;
    selected.bank=fixture_selected;
    return kind==Ft_Kind_CustomFirst && fixture_selected?&selected:NULL;
}
bool nativeScriptFighter_ReferencesProgram(const NativeScriptCompileBank* bank)
{return bank==fixture_retained && (fixture_references&1)!=0;}
bool nativeScriptItem_ReferencesProgram(const NativeScriptCompileBank* bank)
{return bank==fixture_retained && (fixture_references&2)!=0;}
bool nativeScriptEffect_ReferencesProgram(const NativeScriptCompileBank* bank)
{return bank==fixture_retained && (fixture_references&4)!=0;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object)
{return object && object->user_data?fixture_spec:NULL;}
void it_8026EECC(HSD_GObj* object,int pass)
{
    (void)pass;require(fixture_spec && fixture_spec->article &&
        ((Item*)object->user_data)->xC4_article_data==fixture_spec->article);
    ++visual_trace.models;
}
u32 it_8026ECE0(Item_GObj* object,u32 pass)
{(void)object;(void)pass;++visual_trace.pass_tail;return 1;}
HSD_CObj* HSD_CObjGetCurrent(void){return fixture_camera?&camera:NULL;}
void HSD_CObjGetViewingMtx(HSD_CObj* object,Mtx matrix)
{
    require(object==&camera);memset(matrix,0,sizeof(Mtx));
    matrix[0][1]=matrix[1][2]=matrix[2][0]=1;
}
void HSD_StateInvalidate(int mask){require(mask==-1);++visual_trace.invalidations;}
void HSD_StateInitTev(void){}
void HSD_ClearVtxDesc(void){}
void GXLoadPosMtxImm(f32 matrix[3][4],u32 id){require(matrix[0][1]==1 && id==0);}
void GXSetCurrentMtx(u32 id){require(id==0);}
void GXInitTexObj(GXTexObj* texture,void* pixels,u16 width,u16 height,GXTexFmt format,
    GXTexWrapMode horizontal,GXTexWrapMode vertical,u8 mipmap)
{
    (void)texture;require(format==GX_TF_RGBA8 && horizontal==GX_CLAMP && vertical==GX_CLAMP && mipmap==GX_FALSE);
    visual_trace.pixels=pixels;visual_trace.width=width;visual_trace.height=height;
}
void GXLoadTexObj(GXTexObj* texture,GXTexMapID id){(void)texture;require(id==GX_TEXMAP0);}
void GXClearVtxDesc(void){}
void GXSetVtxAttrFmt(GXVtxFmt format,GXAttr attr,GXCompCnt count,GXCompType type,u8 fraction)
{require(format==GX_VTXFMT0 && type==GX_F32 && !fraction && ((attr==GX_VA_POS && count==GX_POS_XYZ)||(attr==GX_VA_TEX0 && count==GX_TEX_ST)));}
void GXSetVtxDesc(GXAttr attr,GXAttrType type){require((attr==GX_VA_POS || attr==GX_VA_TEX0) && type==GX_DIRECT);}
void GXSetNumChans(u8 count){require(count==0);}
void GXSetNumTexGens(u8 count){require(count==1);}
void GXSetTexCoordGen2(GXTexCoordID coordinate,GXTexGenType type,GXTexGenSrc source,u32 matrix,GXBool normalize,u32 post)
{require(coordinate==GX_TEXCOORD0 && type==GX_TG_MTX2x4 && source==GX_TG_TEX0 && matrix==GX_IDENTITY && !normalize && post==GX_PTIDENTITY);}
void GXSetNumTevStages(u8 count){require(count==1);}
void GXSetTevOrder(GXTevStageID stage,GXTexCoordID coordinate,GXTexMapID map,GXChannelID channel)
{require(stage==GX_TEVSTAGE0 && coordinate==GX_TEXCOORD0 && map==GX_TEXMAP0 && channel==GX_COLOR_NULL);}
void GXSetTevOp(GXTevStageID stage,GXTevMode mode){require(stage==GX_TEVSTAGE0 && mode==GX_REPLACE);}
void GXSetCullMode(GXCullMode mode){require(mode==GX_CULL_NONE);}
void GXSetZMode(GXBool compare,GXCompare function,GXBool update){require(compare && function==GX_LEQUAL && !update);}
void GXSetBlendMode(GXBlendMode mode,GXBlendFactor source,GXBlendFactor destination,GXLogicOp operation)
{require(mode==GX_BM_BLEND && source==GX_BL_SRCALPHA && destination==GX_BL_INVSRCALPHA && operation==GX_LO_NOOP);}
void GXSetAlphaCompare(GXCompare first,u8 first_ref,GXAlphaOp operation,GXCompare second,u8 second_ref)
{require(first==GX_GREATER && !first_ref && operation==GX_AOP_AND && second==GX_GREATER && !second_ref);}
void GXBegin(GXPrimitive primitive,GXVtxFmt format,u16 count)
{require(primitive==GX_QUADS && format==GX_VTXFMT0 && count==4);++visual_trace.quads;}
void GXPosition3f32(f32 x,f32 y,f32 z)
{require(visual_trace.positions<4);visual_trace.points[visual_trace.positions++]=(Vec3){x,y,z};}
void GXTexCoord2f32(f32 x,f32 y)
{require(visual_trace.uvs<4);visual_trace.uv[visual_trace.uvs][0]=x;visual_trace.uv[visual_trace.uvs++][1]=y;}
