#include "native_menu_font.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dolphin/gx.h>
#include <sysdolphin/baselib/tev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Glyph { u16 x,y,w,h,advance; } Glyph;
typedef struct Font { u16 width,height,cap_height,count; Glyph glyphs[95]; u8* pixels; } Font;
static Font fonts[NativeMenuFont_Count];
static unsigned references;
void nativeMenuFont_Acquire(void)
{
    if(references++)return;
    const char* names[]={"menu-heading","result-name","result-winner","notice","css-tile"};
    char path[512];DWORD length=GetModuleFileNameA(NULL,path,sizeof(path));
    char* basename=length>0&&length<sizeof(path)?strrchr(path,'\\'):NULL;
    if(!basename)return;
    for(unsigned i=0;i<NativeMenuFont_Count;++i) {
        snprintf(basename,sizeof(path)-(basename-path),"\\ui\\fonts\\%s.bin",names[i]);
        FILE* f=fopen(path,"rb");if(!f)continue;
        Font* font=&fonts[i];
        bool valid=fread(font,sizeof(u16),4,f)==4 &&font->width==1024&&font->height==384&&font->count==95&&font->cap_height;
        if(valid)valid=fread(font->glyphs,sizeof(Glyph),95,f)==95;
        for(unsigned g=0;valid&&g<95;++g) {const Glyph* c=&font->glyphs[g];
            valid=c->w<=62&&c->h<=62&&c->advance<=128&&c->x+c->w<=font->width&&c->y+c->h<=font->height;
        }
        const unsigned bytes=1024*384/2;
        if(valid){font->pixels=_aligned_malloc(bytes,32);valid=font->pixels&&fread(font->pixels,1,bytes,f)==bytes;}
        fclose(f);
        if(!valid){_aligned_free(font->pixels);memset(font,0,sizeof(*font));}
    }
}
void nativeMenuFont_Release(void)
{
    if(!references||--references)return;
    for(unsigned i=0;i<NativeMenuFont_Count;++i){_aligned_free(fonts[i].pixels);memset(&fonts[i],0,sizeof(Font));}
}
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height)
{
    if(face>=NativeMenuFont_Count||!text||!*text)return;
    const Font* font=&fonts[face];if(!font->pixels)return;
    float advance=0;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p) {
        unsigned c=*p;if(c>=0x80){if((c&0xC0)==0x80)continue;c='?';}
        if(c<32||c>126)c='?';advance+=font->glyphs[c-32].advance;
    }
    float scale=height/font->cap_height;
    if(advance*scale>width)scale=width/advance;
    // Stock name/banner rectangles are centered; keep the original typography
    // and the available width rather than stretching every title to fill it.
    x+=(width-advance*scale)*0.5f;
    GXTexObj texture;GXInitTexObj(&texture,font->pixels,font->width,font->height,GX_TF_I4,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXLoadTexObj(&texture,GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG0,face==NativeMenuFont_ResultWinner?(GXColor){245,204,103,255}:(GXColor){245,239,234,255});
    GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_C0);
    GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_A0,GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    for(const unsigned char* p=(const unsigned char*)text;*p;++p) {
        unsigned c=*p;if(c>=0x80){if((c&0xC0)==0x80)continue;c='?';}if(c<32||c>126)c='?';
        const Glyph* g=&font->glyphs[c-32];
        const float u=(float)g->x/font->width,v=(float)g->y/font->height;
        const float r=(float)(g->x+g->w)/font->width,b=(float)(g->y+g->h)/font->height;
        float w=g->w*scale,h=g->h*scale;
        float shear=face==NativeMenuFont_ResultWinner?height*.22f:0;
        GXBegin(GX_QUADS,GX_VTXFMT0,4);
        GXPosition3f32(x+shear,y,z);GXTexCoord2f32(u,v);
        GXPosition3f32(x+w+shear,y,z);GXTexCoord2f32(r,v);
        GXPosition3f32(x+w,y-h,z);GXTexCoord2f32(r,b);
        GXPosition3f32(x,y-h,z);GXTexCoord2f32(u,b);GXEnd();
        x+=g->advance*scale;
    }
}
#endif
