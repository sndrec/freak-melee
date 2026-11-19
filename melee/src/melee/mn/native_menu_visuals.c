#include "native_menu_visuals.h"
#include "native_menu_paint.h"
#include "native_menu_tile.h"
#ifdef MELEE_NATIVE
#include <melee/ft/native_gx_image.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <stdlib.h>
#include <string.h>

enum { LABELS = 16, SPRITES = 256 };
typedef struct MenuLabel { char text[128]; HSD_JObj* anchor; Mtx inverse;
    float x,y,z,width,height; NativeMenuFontFace face; } MenuLabel;
typedef struct MenuSprite { u8* pixels; u16 width,height; float x,y,w,h;
    char title[128]; NativeMenuFontFace face; } MenuSprite;
struct NativeMenuVisuals {
    NativeMenuVisuals** owner;
    HSD_GObj* gobj;
    HSD_JObj* root;
    Mtx inverse;
    unsigned count;
    MenuLabel labels[LABELS];
    MenuSprite sprites[SPRITES];
};
static void matrix(HSD_JObj* anchor, Mtx inverse, Mtx result)
{
    Mtx view, model;
    HSD_JObjSetupMatrix(anchor);
    HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), view);
    C_MTXConcat(anchor->mtx,inverse,model);
    C_MTXConcat(view,model,result);
    GXLoadPosMtxImm(result,0);GXSetCurrentMtx(0);
}
static void draw(HSD_GObj* object,int pass)
{
    NativeMenuVisuals* scene=object->user_data;
    // Menu glyphs and PNG portraits use alpha blending. Draw in the XLU pass
    // after opaque and texture-edge menu geometry, using this scene's camera.
    if(pass!=2 || !HSD_CObjGetCurrent())return;
    Mtx view;
    matrix(scene->root,scene->inverse,view);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
    GXSetNumChans(0);GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetNumTevStages(1);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0,GX_REPLACE);GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE);
    GXSetColorUpdate(GX_TRUE);GXSetAlphaUpdate(GX_FALSE);
    GXSetFog(GX_FOG_NONE,0,0,0,0,(GXColor){0,0,0,0});
    GXSetTevDirect(GX_TEVSTAGE0);GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
    GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_NOOP);
    GXSetAlphaCompare(GX_GREATER,0,GX_AOP_AND,GX_GREATER,0);
    for(unsigned i=0;i<SPRITES;++i) {
        const MenuSprite* p=&scene->sprites[i];if(!p->pixels||p->w<=0||p->h<=0||p->face==NativeMenuFont_Tile)continue;
        GXTexObj texture;GXInitTexObj(&texture,p->pixels,p->width,p->height,GX_TF_RGBA8,GX_CLAMP,GX_CLAMP,GX_FALSE);
        GXLoadTexObj(&texture,GX_TEXMAP0);
        GXBegin(GX_QUADS,GX_VTXFMT0,4);
        GXPosition3f32(p->x,p->y,0);GXTexCoord2f32(0,0);
        GXPosition3f32(p->x+p->w,p->y,0);GXTexCoord2f32(1,0);
        GXPosition3f32(p->x+p->w,p->y-p->h,0);GXTexCoord2f32(1,1);
        GXPosition3f32(p->x,p->y-p->h,0);GXTexCoord2f32(0,1);GXEnd();
    }
    for(unsigned i=0;i<SPRITES;++i) {
        const MenuSprite* p=&scene->sprites[i];
        if(!p->pixels||!p->title[0]||p->w<=0||p->h<=0||p->face==NativeMenuFont_Tile)continue;
        char lines[2][128];
        unsigned line_count = nativeMenuFont_LayoutTileName(p->title,
            p->w*.94f, p->h*.16f, lines);
        float name_top = line_count == 2 ? .64f : .80f;
        matrix(scene->root,scene->inverse,view);
        GXSetTevColor(GX_TEVREG0,(GXColor){15,10,13,255});
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_C0);
        GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_A0);
        GXBegin(GX_QUADS,GX_VTXFMT0,4);
        GXPosition3f32(p->x,p->y-p->h*name_top,0);GXTexCoord2f32(0,0);
        GXPosition3f32(p->x+p->w,p->y-p->h*name_top,0);GXTexCoord2f32(0,0);
        GXPosition3f32(p->x+p->w,p->y-p->h,0);GXTexCoord2f32(0,0);
        GXPosition3f32(p->x,p->y-p->h,0);GXTexCoord2f32(0,0);GXEnd();
        for (unsigned line = 0; line < line_count; ++line) {
            nativeMenuFont_Draw(p->face,lines[line],p->x+p->w*.03f,
                p->y-p->h*(name_top+.02f+line*.16f),0,p->w*.94f,p->h*.16f,
                0xF5EFEAFF,true,true);
        }
    }
    matrix(scene->root,scene->inverse,view);
    // Tile bounds and CSS pointer projection use the stock portrait plane
    // at Z=0. The shared paint primitives draw at Z=-5 by default.
    Mtx tile_plane, tile_view;
    C_MTXTrans(tile_plane, 0, 0, 5);
    C_MTXConcat(view, tile_plane, tile_view);
    nativeMenuPaint_Begin(tile_view);
    for (unsigned i = 0; i < SPRITES; ++i) {
        const MenuSprite* p = &scene->sprites[i];
        if (p->pixels && p->face == NativeMenuFont_Tile)
            nativeMenuTile_Draw(p->pixels,p->width,p->height,p->title,p->x,-p->y,p->w,p->h);
    }
    for(unsigned i=0;i<scene->count;++i) {
        const MenuLabel* label=&scene->labels[i];
        matrix(label->anchor,(float(*)[4])label->inverse,view);
        nativeMenuFont_Draw(label->face,label->text,label->x,label->y,label->z,label->width,label->height,label->face==NativeMenuFont_ResultWinner?0xF5CC67FF:0xF5EFEAFF,true,true);
    }
    HSD_StateInvalidate(-1);HSD_StateInitTev();HSD_ClearVtxDesc();
}
static void destroy(void* data)
{
    NativeMenuVisuals* scene=data;
    nativeMenuFont_Release();
    nativeMenuTile_Release();
    nativeMenuVisuals_ClearSprites(scene);
    *scene->owner=NULL;free(scene);
}
void nativeMenuVisuals_End(NativeMenuVisuals** owner)
{
    if(owner&&*owner)HSD_GObjFree((*owner)->gobj);
}
void nativeMenuVisuals_Begin(NativeMenuVisuals** owner,HSD_JObj* root,u8 gx_link,u8 priority)
{
    nativeMenuVisuals_End(owner);
    NativeMenuVisuals* scene=calloc(1,sizeof(*scene));if(!scene)return;
    scene->owner=owner;scene->root=root;HSD_JObjSetupMatrix(root);
    C_MTXInverse(root->mtx,scene->inverse);
    scene->gobj=GObj_Create(4,5,0x80);if(!scene->gobj){free(scene);return;}
    *owner=scene;nativeMenuFont_Acquire();nativeMenuTile_Acquire();GObj_InitUserData(scene->gobj,0,destroy,scene);
    GObj_SetupGXLink(scene->gobj,draw,gx_link,priority);
}
void nativeMenuVisuals_Text(NativeMenuVisuals* scene,HSD_JObj* anchor,
                           float x,float y,float z,float width,float height,
                           NativeMenuFontFace face,const char* title)
{
    if(!scene||scene->count==LABELS||!title||!*title)return;
    MenuLabel* label=&scene->labels[scene->count++];
    label->anchor=anchor;HSD_JObjSetupMatrix(anchor);C_MTXInverse(anchor->mtx,label->inverse);
    label->x=x;label->y=y;label->z=z;label->width=width;label->height=height;label->face=face;
    size_t length=strlen(title);if(length>=sizeof(label->text))length=sizeof(label->text)-1;
    memcpy(label->text,title,length);label->text[length]=0;
}
void nativeMenuVisuals_ClearSprites(NativeMenuVisuals* scene)
{
    if(!scene)return;
    for(unsigned i=0;i<SPRITES;++i)nativeGxImage_Free(scene->sprites[i].pixels);
    memset(scene->sprites,0,sizeof(scene->sprites));
}
bool nativeMenuVisuals_Sprite(NativeMenuVisuals* scene,unsigned slot,const u8* encoded,unsigned bytes,const char* title,NativeMenuFontFace face)
{
    if(!scene||slot>=SPRITES)return false;
    MenuSprite* sprite=&scene->sprites[slot];nativeGxImage_Free(sprite->pixels);
    sprite->pixels=nativeGxImage_Decode(encoded,bytes,&sprite->width,&sprite->height);
    sprite->face=face;
    size_t length=title?strlen(title):0;if(length>=sizeof(sprite->title))length=sizeof(sprite->title)-1;
    if(length)memcpy(sprite->title,title,length);sprite->title[length]=0;
    return sprite->pixels!=NULL;
}
void nativeMenuVisuals_Rect(NativeMenuVisuals* scene,unsigned slot,float x,float y,float width,float height)
{
    if(!scene||slot>=SPRITES)return;
    MenuSprite* sprite=&scene->sprites[slot];sprite->x=x;sprite->y=y;sprite->w=width;sprite->h=height;
}
#endif
