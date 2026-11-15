#include "native_menu_font.h"
#include <melee/lb/native_utf8.h>
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dolphin/gx.h>
#include <sysdolphin/baselib/tev.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_GLYPHS = 512, MAX_COMPOSITIONS = 256 };
typedef struct Glyph {
    uint32_t codepoint;
    u16 x, y, w, h, advance;
    s16 left, top;
    u16 reserved;
} Glyph;
typedef struct Composition { u32 base, mark, composed; } Composition;
typedef struct Font {
    u16 width, height, cap_height, count, format, composition_count;
    Glyph glyphs[MAX_GLYPHS];
    Composition compositions[MAX_COMPOSITIONS];
    u8* pixels;
    u8* fill;
} Font;
_Static_assert(sizeof(Glyph) == 20, "GX font glyph record size");
static Font fonts[NativeMenuFont_Count];
static unsigned references;

static const Glyph* find_glyph(const Font* font, u32 codepoint)
{
    if (codepoint >= 32 && codepoint <= 126) return &font->glyphs[codepoint - 32];
    unsigned first = 0, last = font->count;
    while (first < last) {
        unsigned middle = (first + last) / 2;
        const Glyph* glyph = &font->glyphs[middle];
        if (glyph->codepoint == codepoint) return glyph;
        if (glyph->codepoint < codepoint) first = middle + 1;
        else last = middle;
    }
    return NULL;
}
static u32 compose(const Font* font, u32 base, u32 mark)
{
    unsigned first = 0, last = font->composition_count;
    while (first < last) {
        unsigned middle = (first + last) / 2;
        const Composition* pair = &font->compositions[middle];
        if (pair->base == base && pair->mark == mark) return pair->composed;
        if (pair->base < base || (pair->base == base && pair->mark < mark))
            first = middle + 1;
        else last = middle;
    }
    return 0;
}
static const Glyph* next_glyph(const Font* font, const u8** cursor, const u8* end)
{
    uint32_t codepoint;
    nativeUtf8_Decode(cursor, end, &codepoint);
    while (*cursor != end && **cursor >= 0xcc && **cursor <= 0xcd) {
        const u8* next = *cursor;
        uint32_t mark;
        if (!nativeUtf8_Decode(&next, end, &mark)) break;
        u32 combined = compose(font, codepoint, mark);
        if (!combined) break;
        codepoint = combined;
        *cursor = next;
    }
    const Glyph* glyph = find_glyph(font, codepoint);
    return glyph ? glyph : find_glyph(font, 0xfffd);
}
static float text_advance(const Font* font, const char* text, const u8* end)
{
    const u8* cursor = (const u8*)text;
    unsigned advance = 0;
    while (cursor != end) advance += next_glyph(font, &cursor, end)->advance;
    return (float)advance;
}
void nativeMenuFont_Acquire(void)
{
    if(references++)return;
    const char* names[]={"menu-heading","result-name","result-winner","notice","css-tile","menu-option","menu-caption"};
    char path[512];DWORD length=GetModuleFileNameA(NULL,path,sizeof(path));
    char* basename=length>0&&length<sizeof(path)?strrchr(path,'\\'):NULL;
    if(!basename)return;
    for(unsigned i=0;i<NativeMenuFont_Count;++i) {
        snprintf(basename,sizeof(path)-(basename-path),"\\ui\\fonts\\%s.bin",names[i]);
        FILE* f=fopen(path,"rb");if(!f)continue;
        Font* font=&fonts[i];
        bool valid=fread(font,sizeof(u16),6,f)==6 &&font->width==1024&&font->height&&font->height<=1024&&!(font->height&7)&&
            font->count>=95&&font->count<=MAX_GLYPHS&&font->composition_count<=MAX_COMPOSITIONS&&font->cap_height&&font->cap_height<=96&&
            (font->format==GX_TF_I4||font->format==GX_TF_IA4);
        if(valid)valid=fread(font->glyphs,sizeof(Glyph),font->count,f)==font->count;
        for(unsigned g=0;valid&&g<font->count;++g) {const Glyph* c=&font->glyphs[g];
            valid=c->w<=128&&c->h<=96&&c->advance<=160&&c->x+c->w<=font->width&&c->y+c->h<=font->height&&
                c->left>=-64&&c->left<=64&&c->top>=-64&&c->top<=64&&c->reserved==0&&
                c->codepoint>=32&&c->codepoint<=0x10ffff&&!(c->codepoint>=0xd800&&c->codepoint<=0xdfff)&&
                (g>=95||c->codepoint==g+32)&&
                (!g||c->codepoint>font->glyphs[g-1].codepoint);
        }
        if(valid)valid=find_glyph(font,'?')&&find_glyph(font,0xfffd)&&
            fread(font->compositions,sizeof(Composition),font->composition_count,f)==font->composition_count;
        for(unsigned g=0;valid&&g<font->composition_count;++g) {
            const Composition* c=&font->compositions[g];
            const Composition* previous=g?&font->compositions[g-1]:NULL;
            valid=find_glyph(font,c->base)&&find_glyph(font,c->composed)&&c->mark>=0x300&&c->mark<=0x36f&&
                (!previous||previous->base<c->base||(previous->base==c->base&&previous->mark<c->mark));
        }
        const unsigned bytes=(unsigned)font->width*font->height/(font->format==GX_TF_I4?2:1);
        if(valid){font->pixels=_aligned_malloc(bytes+(font->format==GX_TF_IA4?bytes/2:0),32);valid=font->pixels&&fread(font->pixels,1,bytes,f)==bytes;}
        if(valid)valid=fgetc(f)==EOF;
        fclose(f);
        if(!valid){_aligned_free(font->pixels);memset(font,0,sizeof(*font));}
        else if(font->format==GX_TF_IA4) {
            font->fill=font->pixels+bytes;
            unsigned columns=font->width/8;
            for(unsigned y=0;y<font->height;++y)for(unsigned x=0;x<font->width;x+=2) {
                unsigned source=((y/4)*columns+x/8)*32+(y%4)*8+x%8;
                unsigned target=((y/8)*columns+x/8)*32+(y%8)*4+(x%8)/2;
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
    const Font* font=&fonts[face];
    return text_advance(font,text,(const u8*)text+strlen(text))*height/font->cap_height;
}
void nativeMenuFont_Inspect(const char* text)
{
    if (!text) text = "";
    for (unsigned face = 0; face < NativeMenuFont_Count; ++face) {
        const Font* font = &fonts[face];
        const u8* cursor = (const u8*)text;
        const u8* end = cursor + strlen(text);
        const Glyph* first = font->pixels && cursor != end ? next_glyph(font, &cursor, end) : NULL;
        fprintf(stderr, "[hub-inspect] font=%u glyphs=%u pairs=%u atlas=%ux%u first=U+%04lX width=%.4f\n",
            face, font->count, font->composition_count, font->width, font->height,
            (unsigned long)(first ? first->codepoint : 0), nativeMenuFont_Width(face, text, 10));
    }
}
unsigned nativeMenuFont_LayoutTileName(const char* text, float width,
                                      float height, char lines[2][128])
{
    if (!text) text = "";
    const u8* start = (const u8*)text;
    const u8* cursor = start;
    const u8* end = start + strlen(text);
    while (cursor != end) {
        const u8* next = cursor; uint32_t codepoint;
        nativeUtf8_Decode(&next, end, &codepoint);
        if (next - start >= 128) break;
        cursor = next;
    }
    memcpy(lines[0], start, cursor - start);
    lines[0][cursor - start] = 0;
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
        const u8* next = (const u8*)lines[0] + i; uint32_t codepoint;
        nativeUtf8_Decode(&next, (const u8*)lines[0] + length, &codepoint);
        if (codepoint >= 0x300 && codepoint <= 0x36f) continue;
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
static float clip_left, clip_right, clip_fade;
void nativeMenuFont_Clip(float left, float right, float fade)
{
    clip_left = left; clip_right = right;
    clip_fade = fade > 0 && right > left ? fminf(fade, (right-left)*.5f) : 0;
}
typedef struct FontVertex { float x, y, u, v; } FontVertex;
static unsigned clip_edge(const FontVertex* input, unsigned count, FontVertex* output, float bound, bool minimum)
{
    unsigned written = 0;
    for (unsigned i = 0; i < count; ++i) {
        FontVertex a = input[(i + count - 1) % count], b = input[i];
        const bool inside_a = minimum ? a.x >= bound : a.x <= bound;
        const bool inside_b = minimum ? b.x >= bound : b.x <= bound;
        if (inside_a != inside_b) {
            const float t = (bound - a.x) / (b.x - a.x);
            output[written++] = (FontVertex){bound, a.y+(b.y-a.y)*t, a.u+(b.u-a.u)*t, a.v+(b.v-a.v)*t};
        }
        if (inside_b) output[written++] = b;
    }
    return written;
}
void nativeMenuFont_Draw(NativeMenuFontFace face,const char* text,
                        float x,float y,float z,float width,float height,
                        u32 rgba, bool centered, bool outline)
{
    if((unsigned)face>=NativeMenuFont_Count||!text||!*text||width<=0||height<=0)return;
    const Font* font=&fonts[face];if(!font->pixels)return;
    const u8* end=(const u8*)text+strlen(text);
    float advance=text_advance(font,text,end);
    if(advance<=0)return;
    float scale=height/font->cap_height;
    float horizontal=advance*scale>width?width/advance:scale;
    if(centered)x+=(width-advance*horizontal)*0.5f;
    const bool border=outline&&font->format==GX_TF_IA4;
    const bool fading=clip_fade>0;
    if (fading) {
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    }
    GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,fading?GX_COLOR0A0:GX_COLOR_NULL);
    GXSetTevColor(GX_TEVREG0,(GXColor){rgba>>24,rgba>>16,rgba>>8,rgba});
    GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,fading?GX_CA_RASA:GX_CA_A0,GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    const float origin=x;
    // Draw all black coverage first, then all coloured fill. Overlapping
    // outlines must never paint over the neighbouring letter's fill.
    for(unsigned pass=border?0:1;pass<2;++pass) {
        x=origin;
        GXTexObj texture;GXInitTexObj(&texture,pass&&font->fill?font->fill:font->pixels,
            font->width,font->height,!pass?GX_TF_IA4:GX_TF_I4,GX_CLAMP,GX_CLAMP,GX_FALSE);
        GXLoadTexObj(&texture,GX_TEXMAP0);
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,pass?GX_CC_C0:GX_CC_ZERO);
        for(const u8* p=(const u8*)text;p!=end;) {
            const Glyph* g=next_glyph(font,&p,end);
            const float u=(float)g->x/font->width,v=(float)g->y/font->height;
            const float r=(float)(g->x+g->w)/font->width,b=(float)(g->y+g->h)/font->height;
            float w=g->w*horizontal,h=g->h*scale;
            float left=x+g->left*horizontal,top=y-g->top*scale;
            float shear=face==NativeMenuFont_ResultWinner?height*.22f:
                face==NativeMenuFont_MenuCaption?(h+g->top*scale)*.30f:0;
            FontVertex vertices[8] = {{left+shear,top,u,v},{left+w+shear,top,r,v},
                {left+w,top-h,r,b},{left,top-h,u,b}};
            unsigned count = 4;
            if (clip_right > clip_left) {
                FontVertex scratch[8];
                count = clip_edge(vertices, count, scratch, clip_left, true);
                if (count) count = clip_edge(scratch, count, vertices, clip_right, false);
            }
            if (count >= 3) {
                GXBegin(GX_TRIANGLEFAN,GX_VTXFMT0,count);
                for (unsigned i=0;i<count;++i) {
                    GXPosition3f32(vertices[i].x,vertices[i].y,z);
                    if(fading) {
                        float alpha=fminf(1,fminf((vertices[i].x-clip_left)/clip_fade,(clip_right-vertices[i].x)/clip_fade));
                        GXColor4u8(255,255,255,(u8)((rgba&255)*fmaxf(0,alpha)));
                    }
                    GXTexCoord2f32(vertices[i].u,vertices[i].v);
                }
                GXEnd();
            }
            x+=g->advance*horizontal;
        }
    }
    if(fading) {
        GXSetNumChans(0);GXSetVtxDesc(GX_VA_CLR0,GX_NONE);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR_NULL);
    }
}
#endif
