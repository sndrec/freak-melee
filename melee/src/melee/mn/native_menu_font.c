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
typedef struct Font { u16 width,height,cap_height,count,format; Glyph glyphs[95]; u8* pixels; u8* fill; } Font;
static Font fonts[NativeMenuFont_Count];
static unsigned references;
void nativeMenuFont_Acquire(void)
{
    if(references++)return;
    const char* names[]={"menu-heading","result-name","result-winner","notice","css-tile","menu-option"};
    char path[512];DWORD length=GetModuleFileNameA(NULL,path,sizeof(path));
    char* basename=length>0&&length<sizeof(path)?strrchr(path,'\\'):NULL;
    if(!basename)return;
    for(unsigned i=0;i<NativeMenuFont_Count;++i) {
        snprintf(basename,sizeof(path)-(basename-path),"\\ui\\fonts\\%s.bin",names[i]);
        FILE* f=fopen(path,"rb");if(!f)continue;
        Font* font=&fonts[i];
        bool valid=fread(font,sizeof(u16),5,f)==5 &&font->width==1024&&font->height==384&&font->count==95&&font->cap_height&&
            (font->format==GX_TF_I4||font->format==GX_TF_IA4);
        if(valid)valid=fread(font->glyphs,sizeof(Glyph),95,f)==95;
        for(unsigned g=0;valid&&g<95;++g) {const Glyph* c=&font->glyphs[g];
            valid=c->w<=62&&c->h<=62&&c->advance<=128&&c->x+c->w<=font->width&&c->y+c->h<=font->height;
        }
        const unsigned bytes=1024*384/(font->format==GX_TF_I4?2:1);
        if(valid){font->pixels=_aligned_malloc(bytes+(font->format==GX_TF_IA4?bytes/2:0),32);valid=font->pixels&&fread(font->pixels,1,bytes,f)==bytes;}
        fclose(f);
        if(!valid){_aligned_free(font->pixels);memset(font,0,sizeof(*font));}
        else if(font->format==GX_TF_IA4) {
            font->fill=font->pixels+bytes;
            for(unsigned y=0;y<384;++y)for(unsigned x=0;x<1024;x+=2) {
                unsigned source=((y/4)*128+x/8)*32+(y%4)*8+x%8;
                unsigned target=((y/8)*128+x/8)*32+(y%8)*4+(x%8)/2;
                font->fill[target]=((font->pixels[source]&15)<<4)|(font->pixels[source+1]&15);
            }
        }
    }
}
void nativeMenuFont_Release(void)
{
    if(!references||--references)return;
    for(unsigned i=0;i<NativeMenuFont_Count;++i){_aligned_free(fonts[i].pixels);memset(&fonts[i],0,sizeof(Font));}
}
float nativeMenuFont_Width(NativeMenuFontFace face,const char* text,float height)
{
    if((unsigned)face>=NativeMenuFont_Count||!text||!fonts[face].cap_height)return 0;
    const Font* font=&fonts[face];unsigned width=0;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p){
        unsigned c=*p;
        if(c>=0x80){if((c&0xC0)==0x80)continue;c='?';}
        if(c<32||c>126)c='?';width+=font->glyphs[c-32].advance;
    }
    return width*height/font->cap_height;
}
unsigned nativeMenuFont_LayoutTileName(const char* text, float width,
                                      float height, char lines[2][128])
{
    snprintf(lines[0], 128, "%s", text ? text : "");
    lines[1][0] = 0;
    const unsigned length = (unsigned)strlen(lines[0]);
    if (length < 2 || nativeMenuFont_Width(NativeMenuFont_Tile, lines[0], height)
            <= width * 2.2f) return 1;
    unsigned split = 0;
    float best = 1e30f;
    bool has_space = strchr(lines[0] + 1, ' ') != NULL;
    for (unsigned i = 1; i < length; ++i) {
        if (((unsigned char)lines[0][i] & 0xC0) == 0x80 ||
            (has_space && lines[0][i] != ' ')) continue;
        char saved = lines[0][i];
        lines[0][i] = 0;
        float a = nativeMenuFont_Width(NativeMenuFont_Tile, lines[0], height);
        lines[0][i] = saved;
        float b = nativeMenuFont_Width(NativeMenuFont_Tile,
                                      lines[0] + i + (saved == ' '), height);
        if (a > 0 && b > 0 && (a > b ? a : b) < best) {
            best = a > b ? a : b;
            split = i;
        }
    }
    if (!split) return 1;
    snprintf(lines[1], 128, "%s", lines[0] + split + (lines[0][split] == ' '));
    lines[0][split] = 0;
    return 2;
}
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height,
                        u32 rgba, bool centered, bool outline)
{
    if(face>=NativeMenuFont_Count||!text||!*text)return;
    const Font* font=&fonts[face];if(!font->pixels)return;
    float advance=0;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p) {
        unsigned c=*p;if(c>=0x80){if((c&0xC0)==0x80)continue;c='?';}
        if(c<32||c>126)c='?';advance+=font->glyphs[c-32].advance;
    }
    float scale=height/font->cap_height;
    float horizontal=advance*scale>width?width/advance:scale;
    if(centered)x+=(width-advance*horizontal)*0.5f;
    bool border=outline&&font->format==GX_TF_IA4;
    GXTexObj texture;GXInitTexObj(&texture,!outline&&font->fill?font->fill:font->pixels,
        font->width,font->height,border?GX_TF_IA4:GX_TF_I4,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXLoadTexObj(&texture,GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG0,(GXColor){rgba>>24,rgba>>16,rgba>>8,rgba});
    if(border)
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_TEXC,GX_CC_C0,GX_CC_ZERO);
    else GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_C0);
    GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_A0,GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    for(const unsigned char* p=(const unsigned char*)text;*p;++p) {
        unsigned c=*p;if(c>=0x80){if((c&0xC0)==0x80)continue;c='?';}if(c<32||c>126)c='?';
        const Glyph* g=&font->glyphs[c-32];
        const float u=(float)g->x/font->width,v=(float)g->y/font->height;
        const float r=(float)(g->x+g->w)/font->width,b=(float)(g->y+g->h)/font->height;
        float w=g->w*horizontal,h=g->h*scale;
        float shear=face==NativeMenuFont_ResultWinner?height*.22f:0;
        GXBegin(GX_QUADS,GX_VTXFMT0,4);
        GXPosition3f32(x+shear,y,z);GXTexCoord2f32(u,v);
        GXPosition3f32(x+w+shear,y,z);GXTexCoord2f32(r,v);
        GXPosition3f32(x+w,y-h,z);GXTexCoord2f32(r,b);
        GXPosition3f32(x,y-h,z);GXTexCoord2f32(u,b);GXEnd();
        x+=g->advance*horizontal;
    }
}
#endif
