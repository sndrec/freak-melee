#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/lb/native_disc_archive.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeSourcePackage package,rejected;
static NativeScriptDiagnostic diagnostic;
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"item package %u: %s: %s\n",__LINE__,#x,diagnostic.message);return 1;}}while(0)
static const char source[]=
    "fighter:\n    behavior independent\naction Main:\n    on load:\n        pass\n"
    "item Snack damage 3 angle 45 lifetime 180:\n    blueprint items/base.fib\n    icon items/snack0.png\n    billboard_size 8\n    on thrown:\n        velocity.x = velocity.x * 0.65\n"
    "item Plain damage 1 angle 45 lifetime 90:\n    blueprint items/base.fib\n    on thrown:\n        velocity.y = 1\n";

int main(int argc,char** argv)
{
    CHECK(argc==4&&aurora_dvd_open(argv[1]));
    NativeFighterProject project;CHECK(nativeFighterProject_Open(argv[2],&project));
    void* common=NULL;size_t common_size=0;CHECK(nativeDiscArchive_Read("ItCo.dat",&common,&common_size));
    HSD_Dat dat;u32 root,table,article;const char* external;
    CHECK(HSD_DatOpen(&dat,common,common_size)==HSD_DAT_OK&&HSD_DatFindRoot(&dat,"itPublicData",&root));
    CHECK(HSD_DatResolve(&dat,root+4,&table,&external)==HSD_DAT_REF_LOCAL);
    CHECK(HSD_DatResolve(&dat,table+It_Kind_Capsule*4,&article,&external)==HSD_DAT_REF_LOCAL);
    size_t blueprint_size=common_size+NATIVE_SOURCE_BLUEPRINT_HEADER;uint8_t* blueprint=malloc(blueprint_size);CHECK(blueprint);
    CHECK(nativeSourceItemBlueprint_Write(common,common_size,article,blueprint,blueprint_size,&blueprint_size,&diagnostic));
    FILE* png_file=fopen(argv[3],"rb");CHECK(png_file&&fseek(png_file,0,SEEK_END)==0);long png_size=ftell(png_file);
    CHECK(png_size>0&&fseek(png_file,0,SEEK_SET)==0);uint8_t* png=malloc((size_t)png_size);CHECK(png);
    CHECK(fread(png,1,(size_t)png_size,png_file)==(size_t)png_size&&fclose(png_file)==0);
    NativeSourceResourceInput resources[]={{"items/base.fib",blueprint,blueprint_size,NativeSourceResource_Blob},
        {"items/snack0.png",png,(size_t)png_size,NativeSourceResource_PNG},
        {"ui/stock-icon.png",png,(size_t)png_size,NativeSourceResource_PNG},
        {"ui/emblem.png",png,(size_t)png_size,NativeSourceResource_PNG}};
    size_t bytes=0;CHECK(nativeSourceResources_Measure(resources,4,&bytes,diagnostic.message,sizeof(diagnostic.message)));
    uint8_t* bundle=malloc(bytes);CHECK(bundle);
    CHECK(nativeSourceResources_Write(resources,4,bundle,bytes,&bytes,diagnostic.message,sizeof(diagnostic.message)));
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=project.asset_record,.facts_bytes=project.asset_record_size,
        .assets={project.fighter_dat,project.fighter_size,project.animation_dat,project.animation_size,project.costume_dat,project.costume_size},
        .source=source,.source_bytes=sizeof(source)-1,.resources=bundle,.resource_bytes=bytes};
    CHECK(nativeSourcePackage_Prepare(&package,&input,Ft_Kind_CustomFirst,NULL,&diagnostic));
    CHECK(package.program.items.count==2&&package.program.items.recipe_count==2&&package.entities.blueprint_count==1);
    CHECK(package.program.items.programs[0].article==package.program.items.programs[1].article);
    CHECK(package.program.items.programs[0].visual&&package.program.items.programs[0].visual->pixels);
    CHECK(!package.program.items.programs[1].visual&&package.program.items.programs[1].article->x10_modelDesc->x0_joint);
    CHECK(package.program.items.programs[0].identity!=package.program.items.programs[1].identity);
    CHECK(package.owned_input.resources!=bundle&&package.entities.blueprints[0].bytes!=common);
    CHECK(package.hud.images[NativeSourceHud_Stock].image.image_ptr);
    CHECK(package.hud.images[NativeSourceHud_Emblem].image.image_ptr);
    CHECK(package.hud.images[NativeSourceHud_Stock].image.width==package.visuals.records[0].width);
    NativeSourcePackage* before=malloc(sizeof(package));CHECK(before);memcpy(before,&package,sizeof(package));
    input.resources=NULL;input.resource_bytes=0;
    CHECK(!nativeSourcePackage_Prepare(&rejected,&input,Ft_Kind_CustomFirst,&package,&diagnostic));
    CHECK(!rejected.input_memory&&!rejected.ready&&!memcmp(before,&package,sizeof(package)));
    CHECK(nativeSourcePackage_Pin(&package)&&!nativeSourcePackage_Release(&package));
    CHECK(nativeSourcePackage_Unpin(&package)&&nativeSourcePackage_Release(&package));
    CHECK(!package.visuals.count&&!package.entities.blueprint_count);
    CHECK(!package.hud.images[NativeSourceHud_Stock].image.image_ptr &&
          !package.hud.images[NativeSourceHud_Emblem].image.image_ptr);
    free(before);free(bundle);free(png);free(blueprint);_aligned_free(common);nativeFighterProject_Close(&project);aurora_dvd_close();
    printf("{\"status\":\"PASS real source item package\",\"checks\":%u,\"owned_blueprints\":1,\"items\":2,\"game\":false}\n",checks);return 0;
}
