#include "boundary.h"
#include <melee/it/native_source_item_visual.h>
#include <melee/it/native_holdable.h>
#include <melee/it/types.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_source_resources.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pixels.h"
#include "item_rebind_actual.h"

static unsigned checks;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"visual %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeScriptCompileBank banks[3];
static NativeScriptItemPrograms candidate,accepted,published;
static NativeSourceItemVisuals resources,attempt;
static Article owned_model,other_model;
static Item item;
static HSD_GObj object;

static void initialize(NativeScriptItemPrograms* programs,NativeScriptCompileBank* bank)
{
    *programs=(NativeScriptItemPrograms){0};bank->valid=true;
    programs->count=4;programs->recipe_count=3;
    for(unsigned i=0;i<4;++i) {
        uint64_t identity=UINT64_C(8127)^(UINT64_C(0x9e3779b97f4a7c15)*(i+1));
        programs->programs[i]=(NativeScriptItemProgram){.bank=bank,.identity=identity?identity:1,.owner=i+1,
            .author_kind=Ft_Kind_CustomFirst,.kind=It_Kind_NativeHoldable,.recipe_index=i,
            .states=programs->states,.article=&owned_model};
        if(i<3)programs->programs[i].recipe=&programs->recipes[i];
    }
    programs->recipes[0]=(NativeItemDefinition){.billboard_png=png_first,.billboard_png_size=sizeof(png_first),.billboard_size=8};
    programs->recipes[1]=(NativeItemDefinition){.billboard_png=png_second,.billboard_png_size=sizeof(png_second),.billboard_size=12};
    programs->recipes[2]=(NativeItemDefinition){.billboard_size=8};
}
static void pixels(const NativeSourceItemVisual* visual,unsigned seed)
{
    CHECK(visual->width==5 && visual->height==3 && ((uintptr_t)visual->pixels&31)==0);
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<8;++x) {
        unsigned sx=x<5?x:4,sy=y<3?y:2;
        unsigned tile=(x/4)*64,index=y*4+x%4;
        const uint8_t* bytes=visual->pixels+tile;
        CHECK(bytes[index*2]==(sx==0?0:sx==1?127:255));
        CHECK(bytes[index*2+1]==(uint8_t)(17+sx*31+seed*7));
        CHECK(bytes[32+index*2]==(uint8_t)(9+sy*41+seed*3));
        CHECK(bytes[32+index*2+1]==(uint8_t)(sx*13+sy*19+seed*11));
    }
}
static bool zero(const void* memory,size_t size)
{const uint8_t* p=memory;for(size_t i=0;i<size;++i)if(p[i])return false;return true;}
static void no_draw(void)
{visualFixture_Reset();nativeHoldable_Draw(&object,2);CHECK(!visual_trace.quads && !visual_trace.models && !visual_trace.pass_tail);}

int main(void)
{
    initialize(&accepted,&banks[0]);
    CHECK(nativeSourceItemVisuals_Prepare(&resources,&accepted,&diagnostic));
    CHECK(resources.ready && resources.address==&resources && resources.count==2);
    CHECK(accepted.programs[0].visual==&resources.records[0] && accepted.programs[1].visual==&resources.records[1]);
    CHECK(resources.records[0].identity==accepted.programs[0].identity);
    CHECK(resources.records[1].identity==accepted.programs[1].identity);
    CHECK(resources.records[0].identity!=resources.records[1].identity);
    CHECK(!accepted.programs[2].visual && !accepted.programs[3].visual);
    pixels(&resources.records[0],0);pixels(&resources.records[1],1);
    published=accepted;nativeScriptItemPrograms_Rebind(&published,&accepted);
    CHECK(published.programs[0].recipe==&published.recipes[0]);
    CHECK(published.programs[0].visual==accepted.programs[0].visual);
    CHECK(published.programs[0].visual->png==published.programs[0].recipe->billboard_png);

    item=(Item){.pos={10,20,30},.kind=It_Kind_NativeHoldable,.xC4_article_data=&owned_model};
    object=(HSD_GObj){.user_data=&item};fixture_spec=&published.programs[0];
    visualFixture_Reset();nativeHoldable_Draw(&object,0);nativeHoldable_Draw(&object,1);
    CHECK(!visual_trace.quads && !visual_trace.models && visual_trace.pass_tail==2 && visual_trace.invalidations==2);
    visualFixture_Reset();nativeHoldable_Draw(&object,2);
    CHECK(visual_trace.quads==1 && visual_trace.positions==4 && visual_trace.uvs==4 && visual_trace.models==0);
    CHECK(visual_trace.pixels==resources.records[0].pixels && visual_trace.width==5 && visual_trace.height==3);
    const Vec3 expected[]={{10,16,34},{10,24,34},{10,24,26},{10,16,26}};
    const float uv[][2]={{0,0},{1,0},{1,1},{0,1}};
    for(unsigned i=0;i<4;++i) {
        CHECK(!memcmp(&expected[i],&visual_trace.points[i],sizeof(Vec3)));
        CHECK(!memcmp(uv[i],visual_trace.uv[i],sizeof(uv[i])));
    }
    visualFixture_Reset();fixture_camera=false;nativeHoldable_Draw(&object,2);CHECK(!visual_trace.quads);
    // Stored native numeric indices are not visual selectors anymore.
    item.xDD4_itemVar.native_holdable.owner_kind=UINT16_MAX;
    item.xDD4_itemVar.native_holdable.definition_index=UINT16_MAX;
    visualFixture_Reset();nativeHoldable_Draw(&object,2);CHECK(visual_trace.pixels==resources.records[0].pixels);
    fixture_spec=&published.programs[1];visualFixture_Reset();nativeHoldable_Draw(&object,2);
    CHECK(visual_trace.pixels==resources.records[1].pixels && visual_trace.points[0].y==14);
    fixture_spec=&published.programs[0];
    NativeScriptItemProgram saved=published.programs[0];
    published.programs[0].owner=99;no_draw();published.programs[0]=saved;
    published.programs[0].identity++;no_draw();published.programs[0]=saved;
    published.programs[0].bank=&banks[1];no_draw();published.programs[0]=saved;
    published.programs[0].visual=NULL;no_draw();published.programs[0]=saved;
    published.recipes[0].billboard_size=7;no_draw();published.recipes[0].billboard_size=8;
    published.recipes[0].billboard_png=png_second;no_draw();published.recipes[0].billboard_png=png_first;
    published.recipes[0].billboard_png_size--;no_draw();published.recipes[0].billboard_png_size++;
    fixture_spec=NULL;no_draw();fixture_spec=&published.programs[0];
    fixture_spec=&published.programs[2];visualFixture_Reset();nativeHoldable_Draw(&object,2);
    CHECK(visual_trace.models==1 && !visual_trace.quads);
    item.xC4_article_data=&other_model;no_draw();item.xC4_article_data=&owned_model;
    published.programs[2].article=NULL;no_draw();published.programs[2].article=&owned_model;

    // The first candidate image decodes before the second's invalid CRC fails.
    uint16_t width=0,height=0;
    CHECK(!nativeSourceResources_PNG(png_invalid,sizeof(png_invalid),&width,&height,NULL,0));
    initialize(&candidate,&banks[1]);candidate.recipes[1].billboard_png=png_invalid;
    candidate.recipes[1].billboard_png_size=sizeof(png_invalid);
    const void* accepted_pixels=resources.records[0].pixels;
    uint8_t accepted_sample=resources.records[0].pixels[1];
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    CHECK(zero(&attempt,sizeof(attempt)) && !candidate.programs[0].visual && !candidate.programs[1].visual);
    CHECK(resources.records[0].pixels==accepted_pixels && resources.records[0].pixels[1]==accepted_sample);
    initialize(&candidate,&banks[1]);candidate.recipes[1].billboard_size=NAN;
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic) && zero(&attempt,sizeof(attempt)));
    initialize(&candidate,&banks[1]);candidate.programs[1].owner=candidate.programs[0].owner;
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    initialize(&candidate,&banks[1]);candidate.programs[1].bank=&banks[2];banks[2].valid=true;
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    initialize(&candidate,&banks[1]);candidate.programs[1].recipe=&accepted.recipes[1];
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    initialize(&candidate,&banks[1]);fixture_idle=false;
    CHECK(!nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));fixture_idle=true;

    fixture_selected=&banks[0];
    CHECK(!nativeSourceItemVisuals_Release(&resources) && resources.records[0].pixels==accepted_pixels);
    // Failed staging and preparation for a different bank cannot edit active pixels.
    CHECK(nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    CHECK(attempt.records[0].pixels!=accepted_pixels && resources.records[0].pixels==accepted_pixels);
    CHECK(nativeSourceItemVisuals_Release(&attempt));
    fixture_selected=NULL;fixture_retained=&banks[0];
    for(unsigned i=1;i<=4;i*=2) {
        fixture_references=i;CHECK(!nativeSourceItemVisuals_Release(&resources));
        CHECK(resources.records[0].pixels==accepted_pixels);
    }
    fixture_references=0;fixture_idle=false;CHECK(!nativeSourceItemVisuals_Release(&resources));fixture_idle=true;
    HSD_GObj_CurrentInvokedProc=(HSD_GObjProc*)1;CHECK(!nativeSourceItemVisuals_Release(&resources));HSD_GObj_CurrentInvokedProc=NULL;
    HSD_GObj_804D7818=&object;CHECK(!nativeSourceItemVisuals_Release(&resources));HSD_GObj_804D7818=NULL;
    HSD_GObj_DelayedProcInfo.flags=1;CHECK(!nativeSourceItemVisuals_Release(&resources));HSD_GObj_DelayedProcInfo.flags=0;
    CHECK(nativeSourceItemVisuals_Release(&resources) && zero(&resources,sizeof(resources)));
    CHECK(nativeSourceItemVisuals_Release(&resources));
    fixture_spec=&published.programs[0];no_draw();
    // With no PNG resources there is nothing to retain/free here; the package
    // still applies its independent selected-bank/lease retirement preflight.
    initialize(&candidate,&banks[1]);
    for(unsigned i=0;i<candidate.recipe_count;++i) {
        candidate.recipes[i].billboard_png=NULL;candidate.recipes[i].billboard_png_size=0;
    }
    CHECK(nativeSourceItemVisuals_Prepare(&attempt,&candidate,&diagnostic));
    CHECK(attempt.ready && attempt.count==0);
    CHECK(!candidate.programs[0].visual && !candidate.programs[1].visual);
    fixture_selected=&banks[1];CHECK(nativeSourceItemVisuals_Release(&attempt));fixture_selected=NULL;
    printf("{\"status\":\"PASS source item visuals\",\"checks\":%u,\"real_wic_decode\":true,\"renderer_initialized\":false}\n",checks);
    return 0;
}
