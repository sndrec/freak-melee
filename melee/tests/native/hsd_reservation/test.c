#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/dobj.h>
#include <melee/it/native_article_model_demand.h>
#include <sysdolphin/baselib/dat_import.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%u: %s\n",__FILE__,__LINE__,#c); exit(1); } } while (0)
static unsigned char heap[1024 * 1024] ATTRIBUTE_ALIGN(64);
static unsigned char backing[65536] ATTRIBUTE_ALIGN(64);
static unsigned char second_backing[4096] ATTRIBUTE_ALIGN(64);
static NativeHsdReservation owner, second;
static HSD_ObjAllocData satellite;

void __assert(char* file, u32 line, char* condition)
{
    fprintf(stderr,"native assertion %s:%lu: %s\n",file,line,condition);
    exit(2);
}

void OSReport(char* format, ...)
{
    va_list args; va_start(args,format); vfprintf(stderr,format,args); va_end(args);
}

void HSD_Panic(char* file, u32 line, char* condition)
{
    __assert(file,line,condition);
}

static int initialize(HSD_Class* object)
{
    CHECK(object->class_info != NULL);
    return 0;
}

static void test_allocators(void)
{
    void* start = OSInitAlloc(heap, heap + sizeof(heap), 1);
    CHECK(start != NULL);
    int handle = OSCreateHeap(start, heap + sizeof(heap));
    CHECK(handle >= 0);
    HSD_SetHeap(handle);
    long initial_heap = OSCheckHeap(handle);
    void* ordinary = hsdAllocMemPiece(48);
    CHECK(ordinary != NULL);
    hsdFreeMemPiece(ordinary,48);
    CHECK(hsdAllocMemPiece(48) == ordinary);
    hsdFreeMemPiece(ordinary,48);
    HSD_ObjAllocInit(&satellite,24,16);
    void* ordinary_obj = HSD_ObjAlloc(&satellite);
    CHECK(ordinary_obj != NULL && satellite.used == 1);
    HSD_ObjFree(&satellite,ordinary_obj);
    CHECK(satellite.used == 0 && satellite.free == 1);
    CHECK(OSCheckHeap(handle) < initial_heap);

    NativeHsdDemand plan[] = {{NULL,48,4},{NULL,128,2},{&satellite,satellite.size,4}};
    size_t alignment = 0;
    size_t bytes = nativeHsdReservation_Bytes(plan,3,&alignment);
    CHECK(bytes != 0 && bytes <= sizeof(backing) && alignment == 32);
    memset(&owner,0xa6,sizeof(owner)); memset(backing,0x53,sizeof(backing));
    NativeHsdReservation prior = owner;
    CHECK(!nativeHsdReservation_Bind(&owner,backing,bytes-1,plan,3,1));
    CHECK(!memcmp(&owner,&prior,sizeof(owner)) && backing[0] == 0x53);
    CHECK(!nativeHsdReservation_Bind(&owner,backing+1,sizeof(backing)-1,plan,3,1));
    CHECK(!nativeHsdReservation_Bind(&owner,backing,sizeof(backing),plan,3,0));
    NativeHsdDemand duplicate[] = {{NULL,33,1},{NULL,64,1}};
    CHECK(nativeHsdReservation_Bytes(duplicate,2,&alignment) == 0);
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),plan,3,1));
    CHECK(!nativeHsdReservation_Bind(&owner,backing,sizeof(backing),plan,3,2));
    CHECK(!nativeHsdReservation_Bind(&second,backing,sizeof(backing),plan,3,2));
    NativeHsdDemand other = {NULL,64,2};
    CHECK(nativeHsdReservation_Bind(&second,second_backing,sizeof(second_backing),&other,1,2));

    long before_claim = OSCheckHeap(handle);
    CHECK(nativeHsdReservation_Begin(&owner,plan,3));
    CHECK(!nativeHsdReservation_Begin(&second,&other,1));
    void* a = hsdAllocMemPiece(33);
    void* b = hsdAllocMemPiece(64);
    void* c = HSD_ObjAlloc(&satellite);
    CHECK(a && b && c && a != b && ((uintptr_t)c & 15) == 0);
    CHECK((uintptr_t)a >= (uintptr_t)backing && (uintptr_t)a < (uintptr_t)(backing+sizeof(backing)));
    CHECK(satellite.used == 1 && satellite.free == 1);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(OSCheckHeap(handle) == before_claim);
    CHECK(!nativeHsdReservation_Release(&owner));
    CHECK(nativeHsdReservation_Free(NULL,(char*)a+1,64) == -1);
    CHECK(nativeHsdReservation_Free(NULL,a,128) == -1);
    CHECK(nativeHsdReservation_Free(&satellite,a,64) == -1);
    hsdFreeMemPiece(a,64); hsdFreeMemPiece(b,33); HSD_ObjFree(&satellite,c);
    CHECK(satellite.used == 0 && satellite.free == 1);
    CHECK(nativeHsdReservation_Free(NULL,a,64) == -1);

    HSD_ClassInfo info = {0};
    hsdInitClassInfo(&info,&hsdClass,"fixture","reserved_class",sizeof(info),128);
    info.init = initialize;
    CHECK(nativeHsdReservation_Begin(&owner,plan,3));
    HSD_Class* cls = hsdNew(&info);
    CHECK(cls && cls->class_info == &info && info.head.nb_exist == 1);
    CHECK(nativeHsdReservation_End(&owner));
    hsdDelete(cls);
    CHECK(info.head.nb_exist == 0 && info.head.nb_peak == 1);

    NativeHsdDemand once = {NULL,64,1};
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    a = hsdAllocMemPiece(48); hsdFreeMemPiece(a,48);
    CHECK(hsdAllocMemPiece(48) == NULL);
    CHECK(nativeHsdReservation_Failed());
    CHECK(hsdAllocMemPiece(128) == NULL);
    CHECK(!nativeHsdReservation_End(&owner));
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    CHECK(hsdAllocMemPiece(129) == NULL);
    CHECK(!nativeHsdReservation_End(&owner));
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    CHECK(HSD_MemAlloc(32) == NULL && nativeHsdReservation_Failed());
    CHECK(hsdAllocMemPiece(48) == NULL);
    CHECK(!nativeHsdReservation_End(&owner));
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    CHECK(HSD_ObjAllocAddFree(&satellite,1) == 0 && nativeHsdReservation_Failed());
    CHECK(!nativeHsdReservation_End(&owner));
    CHECK(OSCheckHeap(handle) == before_claim);
    CHECK(!nativeHsdReservation_End(&owner));

    satellite.num_limit_flag = 1; satellite.num_limit = 1;
    CHECK(!nativeHsdReservation_Begin(&owner,plan,3));
    NativeHsdDemand one_obj = {&satellite,satellite.size,1};
    CHECK(nativeHsdReservation_Begin(&owner,&one_obj,1));
    c = HSD_ObjAlloc(&satellite); CHECK(c != NULL);
    CHECK(HSD_ObjAlloc(&satellite) == NULL && nativeHsdReservation_Failed());
    CHECK(!nativeHsdReservation_End(&owner));
    HSD_ObjFree(&satellite,c); satellite.num_limit_flag = 0;
    satellite.heap_limit_flag = 1;
    CHECK(!nativeHsdReservation_Begin(&owner,&one_obj,1));
    satellite.heap_limit_flag = 0;
    satellite.align = 31;
    CHECK(!nativeHsdReservation_Begin(&owner,&one_obj,1));
    satellite.align = 15;

    // Explicitly copy this reservation's exact metadata/backing and allocator
    // counters. This is a local restore test, not the game's broad snapshot proof.
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    a = hsdAllocMemPiece(48); CHECK(a != NULL); memset(a,0x9a,48);
    CHECK(nativeHsdReservation_End(&owner));
    NativeHsdReservation saved = owner;
    unsigned char saved_backing[sizeof(backing)]; memcpy(saved_backing,backing,sizeof(backing));
    NativeMatchPoolReference oldref;
    CHECK(nativeMatchPool_Reference(&owner.bins[0].pool,a,&oldref));
    hsdFreeMemPiece(a,48);
    CHECK(nativeHsdReservation_Begin(&owner,&once,1));
    b = hsdAllocMemPiece(48); CHECK(b == a);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(nativeMatchPool_Resolve(oldref) == NULL);
    owner = saved; memcpy(backing,saved_backing,sizeof(backing));
    CHECK(nativeMatchPool_Resolve(oldref) == a && ((unsigned char*)a)[47] == 0x9a);
    hsdFreeMemPiece(a,48);
    CHECK(nativeHsdReservation_Release(&owner));
    CHECK(nativeHsdReservation_Release(&second));
    CHECK(!nativeHsdReservation_Release(&owner));
    CHECK(OSCheckHeap(handle) == before_claim);
    void* raw = HSD_MemAlloc(64); CHECK(raw != NULL); HSD_Free(raw);
    CHECK(OSCheckHeap(handle) == before_claim);
}

// Shader-only synthetic cases also pass draw-input admission. These owned
// bytes are a valid I4 image, never a substituted texture for imported models.
static unsigned char draw_pixels[32];
static HSD_ImageDesc draw_image={.image_ptr=draw_pixels,.width=8,.height=8,.format=GX_TF_I4};

static void census(void)
{
    HSD_IDInitAllocData(); HSD_VecInitAllocData(); HSD_MtxInitAllocData();
    HSD_RObjInitAllocData();
    Mtx matrix = {{0}};
    HSD_Joint child = { .mtx = matrix }, root = { .child = &child };
    NativeArticleModelDemand d;
    CHECK(nativeArticleModel_Census(&root,&d));
    CHECK(d.uncovered == 0 && d.joints == 2 && d.construction_count == 3);
    CHECK(d.scale_vectors.count == 2 && d.scale_vectors.allocator == HSD_VecGetAllocData());
    HSD_Material mat = {0}; HSD_TlutDesc tlut = {0}; HSD_TObjTevDesc tev = {0};
    HSD_TObjDesc texture = {.tlutdesc=&tlut,.tev=&tev,.repeat_s=1,.repeat_t=1,.imagedesc=&draw_image};
    HSD_MObjDesc material = {.mat=&mat,.texdesc=&texture};
    HSD_VtxDescList vertices[]={{.attr=GX_VA_NULL}};
    HSD_PObjDesc polygon = {.verts=vertices};
    HSD_DObjDesc drawable = {.mobjdesc=&material,.pobjdesc=&polygon};
    root.u.dobjdesc = &drawable;
    CHECK(nativeArticleModel_Census(&root,&d));
    CHECK(d.uncovered == (NativeArticleModel_TevCompiler|NativeArticleModel_MaterialEnvironment) && d.drawables == 1 &&
          d.polygons == 1 && d.materials == 1 && d.textures == 1);
    polygon.flags = POBJ_SHAPEANIM;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered & NativeArticleModel_Shape));
    polygon.flags = POBJ_ENVELOPE;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered & NativeArticleModel_Envelope));
    root.flags = JOBJ_INSTANCE;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered & NativeArticleModel_Instance));
    root.flags = JOBJ_PTCL;
    CHECK(nativeArticleModel_Census(&root,&d) && d.uncovered == NativeArticleModel_ParticleOrSpline);
    root.flags = 0; root.u.dobjdesc = NULL;
    root.class_name = "arbitrary_custom";
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered & NativeArticleModel_CustomClass));
    root.class_name = NULL;
    HSD_RObjDesc reference = {.flags=REFTYPE_EXP}; root.robjdesc = &reference;
    CHECK(nativeArticleModel_Census(&root,&d) && d.references == 1 &&
          (d.uncovered & NativeArticleModel_References));
    root.robjdesc = NULL; root.next = &root;
    NativeArticleModelDemand unchanged = d;
    CHECK(!nativeArticleModel_Census(&root,&d) && !memcmp(&d,&unchanged,sizeof(d)));
}

static unsigned draw_cases;
static void draw_case(const HSD_Joint* root, bool accepted)
{
    NativeArticleModelDemand demand, previous;
    memset(&demand,0x5a,sizeof(demand));previous=demand;
    CHECK(nativeArticleModel_Census(root,&demand)==accepted);
    if (!accepted) CHECK(!memcmp(&demand,&previous,sizeof(demand)));
    ++draw_cases;
}

static void draw_inputs(void)
{
    HSD_Material mat={0};
    HSD_ImageDesc image=draw_image;
    HSD_TlutDesc palette={.lut=draw_pixels};
    HSD_TObjDesc texture={.repeat_s=1,.repeat_t=1,.imagedesc=&image};
    HSD_MObjDesc material={.mat=&mat,.texdesc=&texture};
    HSD_VtxDescList vertices[GX_VA_MAX_ATTR+2]={0};
    vertices[0].attr=GX_VA_NULL;
    HSD_PObjDesc polygon={.verts=vertices};
    HSD_DObjDesc drawable={.mobjdesc=&material,.pobjdesc=&polygon};
    HSD_Joint root={.u.dobjdesc=&drawable};
    draw_case(&root,true);
    texture.repeat_s=0;draw_case(&root,false);texture.repeat_s=1;
    texture.repeat_t=0;draw_case(&root,false);texture.repeat_t=1;
    texture.imagedesc=NULL;draw_case(&root,false);texture.imagedesc=&image;
    image.image_ptr=NULL;draw_case(&root,false);image.image_ptr=draw_pixels;
    image.format=0xff;draw_case(&root,false);
    unsigned formats[]={GX_TF_I4,GX_TF_I8,GX_TF_IA4,GX_TF_IA8,
                        GX_TF_RGB565,GX_TF_RGB5A3,GX_TF_RGBA8,GX_TF_CMPR,
                        GX_TF_C4,GX_TF_C8,GX_TF_C14X2};
    for(unsigned i=0;i<sizeof(formats)/sizeof(formats[0]);++i){
        image.format=formats[i];texture.tlutdesc=i<8?NULL:&palette;
        draw_case(&root,true);
        if(i>=8){
            texture.tlutdesc=NULL;draw_case(&root,false);texture.tlutdesc=&palette;
            palette.lut=NULL;draw_case(&root,false);palette.lut=draw_pixels;
        }
    }
    image.format=GX_TF_I4;texture.tlutdesc=NULL;
    // Native matrix construction accepts zero texture scale. Unused textures
    // still receive hardware slots, so flags do not waive their draw inputs.
    texture.scale.x=texture.scale.y=0;draw_case(&root,true);
    texture.blend_flags=TEX_LIGHTMAP_DIFFUSE;draw_case(&root,true);
    texture.blend_flags=0;image.image_ptr=NULL;draw_case(&root,false);
    image.image_ptr=draw_pixels;
    polygon.verts=NULL;draw_case(&root,false);polygon.verts=vertices;
    polygon.n_display=1;draw_case(&root,false);
    polygon.display=draw_pixels;draw_case(&root,true);polygon.n_display=0;
    vertices[0]=(HSD_VtxDescList){.attr=GX_VA_POS,.attr_type=GX_INDEX8,
                               .vertex=draw_pixels,.stride=12};
    vertices[1].attr=GX_VA_NULL;draw_case(&root,true);
    vertices[0].vertex=NULL;draw_case(&root,false);vertices[0].vertex=draw_pixels;
    vertices[0].stride=0;draw_case(&root,false);
    vertices[0].stride=256;draw_case(&root,false);vertices[0].stride=12;
    vertices[0].attr_type=GX_INDEX16;draw_case(&root,true);
    vertices[0].attr_type=GX_INDEX16+1;draw_case(&root,false);
    vertices[0].attr_type=GX_DIRECT;vertices[0].vertex=NULL;
    vertices[0].stride=0;draw_case(&root,true);
    vertices[0].attr_type=GX_NONE;draw_case(&root,true);
    vertices[1]=vertices[0];vertices[2].attr=GX_VA_NULL;draw_case(&root,false);
    vertices[0].attr=GX_VA_NRM;vertices[1].attr=GX_VA_NBT;
    draw_case(&root,false); // Native NBT normalizes to the same normal channel.
    vertices[0].attr=GX_VA_MAX_ATTR;vertices[1].attr=GX_VA_NULL;
    draw_case(&root,false);
    polygon.flags=POBJ_CULLFRONT|POBJ_CULLBACK;
    polygon.verts=NULL;polygon.n_display=1;polygon.display=NULL;
    draw_case(&root,true); // Native HSD_PObjDisp skips the entire primitive.
    drawable.mobjdesc=NULL;draw_case(&root,false);drawable.mobjdesc=&material;
    drawable.pobjdesc=NULL;draw_case(&root,true); // Empty drawable still has MObj.
}

extern HSD_TObj *tobj_toon, *tobj_shadows;
static unsigned material_cases;

static void compare_shader(const HSD_MObj* first, const HSD_MObj* second)
{
    const HSD_TExpTevDesc *a=first->tevdesc, *b=second->tevdesc;
    while (a && b) {
        CHECK(a->desc.flags==b->desc.flags && a->desc.stage==b->desc.stage &&
              a->desc.color==b->desc.color && a->tobj==b->tobj);
        CHECK(!memcmp(&a->desc.u,&b->desc.u,sizeof(a->desc.u)));
        a=(const HSD_TExpTevDesc*)a->desc.next;
        b=(const HSD_TExpTevDesc*)b->desc.next;
    }
    CHECK(!a && !b);
    const HSD_TExp *c=first->texp, *e=second->texp;
    while (c && e) {
        CHECK(c->type==HSD_TE_CNST && e->type==HSD_TE_CNST);
        CHECK(c->cnst.comp==e->cnst.comp && c->cnst.ctype==e->cnst.ctype &&
              c->cnst.reg==e->cnst.reg && c->cnst.idx==e->cnst.idx &&
              c->cnst.val==e->cnst.val && c->cnst.ref==e->cnst.ref);
        c=c->comm.next; e=e->comm.next;
    }
    CHECK(!c && !e);
}

static void shader_case(const HSD_Joint* root,const HSD_MObjDesc* desc)
{
    NativeArticleModelDemand demand;
    CHECK(nativeArticleModel_Census(root,&demand));
    CHECK(demand.uncovered==NativeArticleModel_MaterialEnvironment);
    CHECK(demand.tev_expressions>0 && demand.tev_descriptors==demand.tev_expressions);
    // Claim ONLY the shader budgets here. Unused JObj/TObj/core class bins
    // must not hide an underestimated shader allocation count of equal size.
    NativeHsdDemand shader[3]={0},capacity[3]={0};unsigned bins=0;
    const uint32_t sizes[]={sizeof(HSD_TETev),sizeof(HSD_TECnst),sizeof(HSD_TExpTevDesc)};
    const uint32_t counts[]={demand.tev_expressions,demand.tev_constants,demand.tev_descriptors};
    for(unsigned i=0;i<3;++i){if(!counts[i])continue;unsigned size=(sizes[i]+31)&~31u,b=0;
        while(b<bins&&shader[b].size!=size)++b;
        if(b==bins){shader[b].size=size;++bins;}shader[b].count+=counts[i];}
    memcpy(capacity,shader,sizeof(capacity));
    for(unsigned i=0;i<bins;++i) capacity[i].count *= 2;
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),capacity,
                                   bins,100+material_cases));
    HSD_TObj textures[8] = {0}; HSD_TObjTev custom[8]; unsigned count=0;
    for(const HSD_TObjDesc* t=desc->texdesc;t;t=t->next) {
        CHECK(count<8); HSD_TObj* texture=&textures[count];
        texture->parent.parent.class_info=(HSD_ClassInfo*)&hsdTObj;
        texture->flags=t->blend_flags; texture->blending=t->blending;
        texture->imagedesc=t->imagedesc;
        if(t->tev){CHECK(sizeof(custom[count])==sizeof(*t->tev));
            memcpy(&custom[count],t->tev,sizeof(*t->tev));texture->tev=&custom[count];}
        if(count)textures[count-1].next=texture;
        ++count;
    }
    HSD_MObj ordinary = {0}, reserved={0};
    ordinary.parent.class_info=reserved.parent.class_info=(HSD_ClassInfo*)&hsdMObj;
    ordinary.mat=reserved.mat=desc->mat;
    ordinary.rendermode=reserved.rendermode=desc->rendermode|RENDER_TOON;
    ordinary.tobj=reserved.tobj=count?textures:NULL;
    CHECK(tobj_toon==NULL && tobj_shadows==NULL); // Controlled environment, not admission.
    HSD_MObjCompileTev(&ordinary,HSD_TExpCompileMode_Abort);
    long before=OSCheckHeap(HSD_GetHeap());
    CHECK(nativeHsdReservation_Begin(&owner,shader,bins));
    HSD_MObjCompileTev(&reserved,HSD_TExpCompileMode_Abort);
    CHECK(!nativeHsdReservation_Failed());
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(OSCheckHeap(HSD_GetHeap())==before);
    compare_shader(&ordinary,&reserved);
    // Recompile real live material outputs. The native path frees the previous
    // descriptors/constants before rebuilding; backing has room for admission
    // even while those persistent objects are still occupied at Begin.
    HSD_MObjCompileTev(&ordinary,HSD_TExpCompileMode_Abort);
    long before_recompile=OSCheckHeap(HSD_GetHeap());
    CHECK(nativeHsdReservation_Begin(&owner,shader,bins));
    HSD_MObjCompileTev(&reserved,HSD_TExpCompileMode_Abort);
    CHECK(nativeHsdReservation_End(&owner));
    CHECK(OSCheckHeap(HSD_GetHeap())==before_recompile);
    compare_shader(&ordinary,&reserved);
    HSD_TExpFreeTevDesc(ordinary.tevdesc);
    HSD_TExpFreeList(ordinary.texp,HSD_TE_ALL,1);
    HSD_TExpFreeTevDesc(reserved.tevdesc);
    HSD_TExpFreeList(reserved.texp,HSD_TE_ALL,1);
    CHECK(nativeHsdReservation_Release(&owner));
    ++material_cases;
}

static void tev_case(unsigned mode,unsigned color,unsigned alpha,unsigned lightmap)
{
    HSD_Material mat={.diffuse={128,96,64,255},.specular={32,64,96,255},.alpha=.6f};
    HSD_TObjDesc texdesc={.blend_flags=color|alpha|lightmap,.blending=.35f,.repeat_s=1,.repeat_t=1,.imagedesc=&draw_image};
    HSD_MObjDesc desc={.rendermode=mode,.mat=&mat,.texdesc=&texdesc};
    HSD_DObjDesc drawable={.mobjdesc=&desc};HSD_Joint root={.u.dobjdesc=&drawable};
    shader_case(&root,&desc);
}

static void custom_matrix(void)
{
    HSD_Material mat={.diffuse={128,96,64,255},.alpha=.6f};
    HSD_TObjTevDesc tev={.color_clamp=1,.alpha_clamp=1,
        .color_a=GX_CC_ZERO,.color_b=GX_CC_ZERO,.color_c=GX_CC_ZERO,.color_d=GX_CC_TEXC,
        .alpha_a=GX_CA_ZERO,.alpha_b=GX_CA_ZERO,.alpha_c=GX_CA_ZERO,.alpha_d=GX_CA_TEXA,
        .konst={96,64,32,170},.tev0={128,96,64,90},.tev1={32,64,96,120},
        .active=TOBJ_TEVREG_ACTIVE_COLOR_TEV|TOBJ_TEVREG_ACTIVE_ALPHA_TEV};
    HSD_TObjDesc texture={.blend_flags=TEX_LIGHTMAP_DIFFUSE|TEX_COLORMAP_MODULATE|TEX_ALPHAMAP_BLEND,.blending=.35f,.tev=&tev,.repeat_s=1,.repeat_t=1,.imagedesc=&draw_image};
    HSD_MObjDesc desc={.rendermode=RENDER_VERTEX,.mat=&mat,.texdesc=&texture};
    HSD_DObjDesc drawable={.mobjdesc=&desc};HSD_Joint root={.u.dobjdesc=&drawable};
    const u8 colors[]={GX_CC_ZERO,GX_CC_ONE,GX_CC_HALF,GX_CC_TEXC,GX_CC_TEXA,
        TOBJ_TEV_CC_KONST_RGB,TOBJ_TEV_CC_KONST_RRR,TOBJ_TEV_CC_KONST_GGG,
        TOBJ_TEV_CC_KONST_BBB,TOBJ_TEV_CC_KONST_AAA,TOBJ_TEV_CC_TEX0_RGB,
        TOBJ_TEV_CC_TEX0_AAA,TOBJ_TEV_CC_TEX1_RGB,TOBJ_TEV_CC_TEX1_AAA};
    const u8 alphas[]={GX_CA_ZERO,GX_CA_TEXA,TOBJ_TEV_CA_KONST_R,
        TOBJ_TEV_CA_KONST_G,TOBJ_TEV_CA_KONST_B,TOBJ_TEV_CA_KONST_A,
        TOBJ_TEV_CA_TEX0_A,TOBJ_TEV_CA_TEX1_A};
    for(unsigned op=0;op<2;++op){tev.color_op=tev.alpha_op=op?GX_TEV_SUB:GX_TEV_ADD;
        for(unsigned i=0;i<sizeof(colors);++i){tev.color_d=colors[i];shader_case(&root,&desc);}
        tev.color_d=GX_CC_TEXC;
        for(unsigned i=0;i<sizeof(alphas);++i){tev.alpha_d=alphas[i];shader_case(&root,&desc);}
        tev.alpha_d=GX_CA_TEXA;
    }
    tev.color_op=tev.alpha_op=GX_TEV_ADD;
    // Inactive arrays are still scanned for constants by the native builder.
    tev.active=TOBJ_TEVREG_ACTIVE_COLOR_TEV;tev.alpha_d=TOBJ_TEV_CA_KONST_A;
    shader_case(&root,&desc);tev.active=TOBJ_TEVREG_ACTIVE_ALPHA_TEV;
    tev.color_d=TOBJ_TEV_CC_TEX0_RGB;shader_case(&root,&desc);
    tev.active=TOBJ_TEVREG_ACTIVE_COLOR_TEV|TOBJ_TEVREG_ACTIVE_ALPHA_TEV;
    tev.alpha_d=GX_CA_TEXA;desc.rendermode=RENDER_DIFFUSE|RENDER_SPECULAR;
    texture.blend_flags|=TEX_LIGHTMAP_SPECULAR|TEX_LIGHTMAP_EXT;
    shader_case(&root,&desc); // Same custom texture participates three times.
    desc.rendermode=RENDER_VERTEX;texture.blend_flags=TEX_LIGHTMAP_DIFFUSE;
    tev.color_a=tev.color_b=tev.color_c=tev.color_d=TOBJ_TEV_CC_TEX0_RGB;
    tev.alpha_a=tev.alpha_b=tev.alpha_c=tev.alpha_d=TOBJ_TEV_CA_TEX0_A;
    NativeArticleModelDemand demand;
    CHECK(nativeArticleModel_Census(&root,&demand)&&demand.tev_expressions==12&&demand.tev_constants==2);
    shader_case(&root,&desc); // Eight staging allocations, two unique constants.
    HSD_TObjTevDesc healthy=tev;
    for(unsigned bad=0;bad<8;++bad){tev=healthy;
        if(bad==0)tev.color_a=GX_CC_CPREV;
        if(bad==1)tev.alpha_a=GX_CA_APREV;
        if(bad==2)tev.color_op=GX_TEV_COMP_R8_GT;
        if(bad==3)tev.alpha_bias=GX_TB_ADDHALF;
        if(bad==4)tev.color_scale=GX_CS_SCALE_2;
        if(bad==5)tev.alpha_clamp=0;
        if(bad==6)tev.active|=1u<<20;
        if(bad==7)tev.active=0;
        CHECK(nativeArticleModel_Census(&root,&demand)&&(demand.uncovered&NativeArticleModel_TevCompiler));
    }
    tev=healthy;HSD_TObjDesc second=texture;texture.next=&second;
    CHECK(nativeArticleModel_Census(&root,&demand)&&(demand.uncovered&NativeArticleModel_TevCompiler));
}

static unsigned imported_roots;
static void imported_arrow(const char* name,unsigned attributes,const char* expected_root)
{
    FILE* file=fopen(name,"rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long size=ftell(file);CHECK(size>0);rewind(file);
    void* bytes=malloc((size_t)size);CHECK(bytes&&fread(bytes,1,(size_t)size,file)==(size_t)size);CHECK(!fclose(file));
    HSD_Dat dat;CHECK(HSD_DatOpen(&dat,bytes,(size_t)size)==HSD_DAT_OK);u32 fighter_root;
    CHECK(HSD_DatFindRoot(&dat,expected_root,&fighter_root));
    void* arena=calloc(1,2*1024*1024);CHECK(arena);HSD_DatImport import;
    CHECK(HSD_DatImportInit(&import,&dat,arena,2*1024*1024,4096)==HSD_DAT_OK);
    for(unsigned offset=36;offset<=40;offset+=4){u32 source;const char* external;
        CHECK(HSD_DatResolve(&dat,attributes+offset,&source,&external)==HSD_DAT_REF_LOCAL);
        HSD_Joint* root=HSD_DatImportGraph(&import,source,DAT_JOINT);CHECK(root&&root->u.dobjdesc&&root->u.dobjdesc->mobjdesc);
        NativeArticleModelDemand demand;CHECK(nativeArticleModel_Census(root,&demand));
        CHECK(demand.uncovered==NativeArticleModel_MaterialEnvironment&&demand.tev_expressions==5&&demand.tev_constants==(offset==36?2u:3u));
        shader_case(root,root->u.dobjdesc->mobjdesc);++imported_roots;
    }free(arena);free(bytes);
}

static void material_matrix(void)
{
    unsigned modes[]={0,RENDER_VERTEX,RENDER_DIFFUSE|RENDER_SPECULAR,
                      RENDER_VERTEX|RENDER_DIFFUSE|RENDER_SPECULAR};
    for(unsigned m=0;m<4;++m)
        for(unsigned c=0;c<=8;++c)
            for(unsigned a=0;a<=7;++a)
                tev_case(modes[m],c<<16,a<<20,TEX_LIGHTMAP_DIFFUSE);
    // Multiple native texture application groups, including blend constants.
    tev_case(RENDER_DIFFUSE|RENDER_SPECULAR,TEX_COLORMAP_BLEND,TEX_ALPHAMAP_BLEND,
             TEX_LIGHTMAP_DIFFUSE|TEX_LIGHTMAP_SPECULAR|TEX_LIGHTMAP_EXT);
    HSD_Material mat={0}; HSD_MObjDesc desc={.mat=&mat};
    HSD_DObjDesc drawable={.mobjdesc=&desc}; HSD_Joint root={.u.dobjdesc=&drawable};
    NativeArticleModelDemand d;
    CHECK(nativeArticleModel_Census(&root,&d) && d.uncovered==NativeArticleModel_MaterialEnvironment);
    desc.rendermode=RENDER_SHADOW;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    desc.rendermode=0; desc.class_name="hsd_mobj";
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    desc.class_name=NULL;
    HSD_TObjDesc textures[9]={0};
    for(unsigned i=0;i<9;++i){textures[i].repeat_s=textures[i].repeat_t=1;textures[i].imagedesc=&draw_image;}
    for(unsigned i=0;i<8;++i) textures[i].next=&textures[i+1];
    desc.texdesc=textures;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[7].next=NULL;
    CHECK(nativeArticleModel_Census(&root,&d) && !(d.uncovered&NativeArticleModel_TevCompiler));
    desc.rendermode=RENDER_SPECULAR;
    for(unsigned i=0;i<8;++i) textures[i].blend_flags=
        TEX_LIGHTMAP_DIFFUSE|TEX_LIGHTMAP_SPECULAR|TEX_LIGHTMAP_EXT;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].next=NULL; desc.rendermode=0;
    textures[0].blend_flags=TEX_COORD_TOON;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].blend_flags=TEX_BUMP;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].blend_flags=9<<16;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].blend_flags=8<<20;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].blend_flags=0;
    HSD_TObjTevDesc custom={0}; textures[0].tev=&custom;
    CHECK(nativeArticleModel_Census(&root,&d) && (d.uncovered&NativeArticleModel_TevCompiler));
    textures[0].tev=NULL; textures[0].next=textures;
    NativeArticleModelDemand previous=d;
    CHECK(!nativeArticleModel_Census(&root,&d) && !memcmp(&d,&previous,sizeof(d)));
}

int main(int argc,char** argv)
{
    CHECK(argc==3);test_allocators(); census(); draw_inputs(); material_matrix();custom_matrix();
    imported_arrow(argv[1],16200,"ftDataLink");imported_arrow(argv[2],16676,"ftDataClink");
    printf("{\"checks\":%u,\"material_cases\":%u,\"imported_roots\":%u,\"draw_input_cases\":%u,\"allocator_paths\":3,\"graph_admission\":false}\n",checks,material_cases,imported_roots,draw_cases);
    return 0;
}
