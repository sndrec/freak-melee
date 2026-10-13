#include <sysdolphin/baselib/native_material_texture_snapshot.h>
#include <sysdolphin/baselib/mobj.h>

static HSD_ClassInfo fixture_texture_class;
static const uint8_t* material_class_index[NATIVE_CLASS_SNAPSHOT_BLOCKS];
static void fixture_texture_init(void)
{
    hsdInitClassInfo(&fixture_texture_class,&hsdClass,"fixture","shadow-texture",
                     sizeof(fixture_texture_class),sizeof(HSD_TObj));
}
static void test_material_textures(void)
{
    unsigned first=checks;camera_idle=true;
    fixture_texture_class.head.info_init=fixture_texture_init;
    HSD_TObj* a=hsdNew(&fixture_texture_class);HSD_TObj* b=hsdNew(&fixture_texture_class);CHECK(a && b);
    tobj_shadows=NULL;tobj_toon=a;
    HSD_MObjAddShadowTexture(a);HSD_MObjAddShadowTexture(b);HSD_MObjAddShadowTexture(a);
    CHECK(tobj_shadows==b && b->next==a && !a->next);
    NativeMaterialTextureSnapshot saved,bad,after;size_t bytes,class_bytes;
    CHECK(nativeMaterialTexture_Save(&saved,sizeof(saved),&bytes));CHECK(saved.shadows==2);
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&class_bytes));
    NativeClassSnapshotView view=nativeClass_ViewSnapshot(stage_graph_saved,class_bytes,material_class_index,NATIVE_CLASS_SNAPSHOT_BLOCKS);
    CHECK(nativeMaterialTexture_ValidateClass(&saved,sizeof(saved),view));
    HSD_MObjDeleteShadowTexture(a);HSD_MObjDeleteShadowTexture(b);tobj_toon=NULL;
    CHECK(!tobj_shadows && !a->next && !b->next);
    CHECK(nativeMaterialTexture_Validate(&saved,sizeof(saved)));
    CHECK(!nativeMaterialTexture_Load(&saved,sizeof(saved)));CHECK(!tobj_shadows && !tobj_toon);
    CHECK(nativeClass_Load(stage_graph_saved,class_bytes));
    CHECK(nativeMaterialTexture_Load(&saved,sizeof(saved)));
    CHECK(tobj_shadows==b && tobj_toon==a && b->next==a && !a->next);
    CHECK(nativeMaterialTexture_Save(&after,sizeof(after),&bytes));CHECK(!memcmp(&saved,&after,sizeof(saved)));
    // Source cleanup after a rewind must unlink the same nodes again.
    HSD_MObjDeleteShadowTexture(a);HSD_MObjDeleteShadowTexture(b);
    CHECK(!tobj_shadows && !a->next && !b->next);
    CHECK(nativeClass_Load(stage_graph_saved,class_bytes));CHECK(nativeMaterialTexture_Load(&saved,sizeof(saved)));
#define MATERIAL_BAD(change) do {bad=saved;change;CHECK(!nativeMaterialTexture_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeMaterialTexture_Load(&bad,sizeof(bad)));}while(0)
    MATERIAL_BAD(++bad.object_epoch);MATERIAL_BAD(bad.reserved=1);MATERIAL_BAD(bad.shadows=0);
    MATERIAL_BAD(bad.roots[0]=(void*)1);MATERIAL_BAD(++bad.backing[0].offset);MATERIAL_BAD(bad.types[0]=(void*)1);
#undef MATERIAL_BAD
    bad=saved;bad.shadows=3;CHECK(nativeMaterialTexture_Validate(&bad,sizeof(bad)));
    CHECK(!nativeMaterialTexture_ValidateClass(&bad,sizeof(bad),view));
    CHECK(!nativeMaterialTexture_Load(&bad,sizeof(bad)));
    CHECK(!nativeMaterialTexture_Save(&after,sizeof(after),(size_t*)b));
    CHECK(!nativeMaterialTexture_Save(&tobj_shadows,sizeof(after),&bytes));
    b->next=b;CHECK(!nativeMaterialTexture_Save(&after,sizeof(after),&bytes));
    CHECK(nativeClass_Load(stage_graph_saved,class_bytes));
    camera_idle=false;CHECK(!nativeMaterialTexture_Load(&saved,sizeof(saved)));camera_idle=true;
    HSD_MObjDeleteShadowTexture(NULL);tobj_toon=NULL;
    CHECK(!tobj_shadows && !a->next && !b->next);
    hsdFreeMemPiece(a,sizeof(*a));hsdFreeMemPiece(b,sizeof(*b));camera_idle=false;
    fprintf(stderr,"material-textures checks=%u bytes=%zu actual add/delete replay, companion and candidate checks passed\n",checks-first,sizeof(saved));
}
