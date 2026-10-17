#include "native_fighter_project.h"
#include "native_fighter_authoring.h"
#include "native_fighter_animation_editor.h"
#include "native_fighter_model_references.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ftdat.h"
#include "ftdata.h"
#include "native_fighter_runtime.h"
#include "native_fighter_animation_copy.h"
#include "native_fighter_dat_copy.h"
#include "native_fighter_demo_copy.h"
#include "native_source_package.h"
#include <melee/gm/native_content_store.h>
#include <melee/gm/gm_1601.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/pl/player.h>
#include <melee/vi/vi.h>
#include <sysdolphin/baselib/dat.h>

enum { PROJECT_PATH_CAPACITY = 512 };

static bool project_path(char output[PROJECT_PATH_CAPACITY], const char* root,
                         const char* name)
{
    int length = snprintf(output, PROJECT_PATH_CAPACITY, "%s\\%s", root, name);
    return length > 0 && length < PROJECT_PATH_CAPACITY;
}

static bool write_asset(const char* root, const char* name, const void* bytes,
                        size_t size)
{
    char path[PROJECT_PATH_CAPACITY];
    if (!project_path(path, root, name)) {
        return false;
    }
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    bool ok = fwrite(bytes, 1, size, file) == size;
    return fclose(file) == 0 && ok;
}

static bool read_asset(const char* root, const char* name, uint8_t** bytes,
                       size_t* size)
{
    char path[PROJECT_PATH_CAPACITY];
    if (!project_path(path, root, name)) {
        return false;
    }
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    long length = 0;
    bool valid = fseek(file, 0, SEEK_END) == 0 &&
                 (length = ftell(file)) >= 0 &&
                 (size_t) length <= NATIVE_CONTENT_MAX_PACKAGE_BYTES &&
                 fseek(file, 0, SEEK_SET) == 0;
    uint8_t* data = valid ? malloc((size_t) length + 1) : NULL;
    bool ok = data != NULL &&
              fread(data, 1, (size_t) length, file) == (size_t) length;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        free(data);
        return false;
    }
    *bytes = data;
    *size = (size_t) length;
    return true;
}

static bool read_optional_asset(const char* root, const char* name,
                                uint8_t** bytes, size_t* size)
{
    char path[PROJECT_PATH_CAPACITY];
    if (!project_path(path, root, name)) return false;
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return GetLastError() == ERROR_FILE_NOT_FOUND;
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
           read_asset(root, name, bytes, size);
}

static bool title_valid(const char* title)
{
    size_t length = strnlen(title, 64);
    if (length == 0 || length >= 64) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (title[i] == '\n' || title[i] == '\r' || (unsigned char) title[i] < 32) {
            return false;
        }
    }
    return true;
}

static bool template_key_valid(const char* key)
{
    size_t length = strnlen(key, NATIVE_FIGHTER_CODE_KEY_BYTES);
    if (length == 0 || length >= NATIVE_FIGHTER_CODE_KEY_BYTES) return false;
    for (size_t i = 0; i < length; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) return false;
    }
    return true;
}

const char* nativeFighterProject_StockBaseName(FighterKind kind)
{
    switch (kind) {
    case Ft_Kind_Mario: return "Mario";
    case Ft_Kind_DrMario: return "DrMario";
    case Ft_Kind_Ganon: return "Ganon";
    case Ft_Kind_Captain: return "Captain Falcon";
    case Ft_Kind_Luigi: return "Luigi";
    case Ft_Kind_Purin: return "Jigglypuff";
    case Ft_Kind_Fox: return "Fox";
    case Ft_Kind_Falco: return "Falco";
    case Ft_Kind_Mars: return "Marth";
    case Ft_Kind_Emblem: return "Roy";
    case Ft_Kind_Donkey: return "Donkey Kong";
    case Ft_Kind_Koopa: return "Bowser";
    case Ft_Kind_Pikachu: return "Pikachu";
    case Ft_Kind_Pichu: return "Pichu";
    case Ft_Kind_Samus: return "Samus";
    case Ft_Kind_Link: return "Link";
    case Ft_Kind_CLink: return "Young Link";
    case Ft_Kind_Mewtwo: return "Mewtwo";
    case Ft_Kind_Yoshi: return "Yoshi";
    case Ft_Kind_Ness: return "Ness";
    case Ft_Kind_Peach: return "Peach";
    case Ft_Kind_GameWatch: return "Game & Watch";
    case Ft_Kind_Zelda: return "Zelda";
    case Ft_Kind_Seak: return "Sheik";
    case Ft_Kind_Popo: return "Popo";
    case Ft_Kind_Nana: return "Nana";
    default: return NULL;
    }
}

bool nativeFighterProject_CreateStock(const char* path, const char* title,
                                      FighterKind source_kind,
                                      const void* fighter, size_t fighter_size,
                                      const void* animation,
                                      size_t animation_size,
                                      const void* costume, size_t costume_size,
                                      const NativeFighterCpuProfile* cpu)
{
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_FindStockDefinition(source_kind);
    const char* base_name = nativeFighterProject_StockBaseName(source_kind);
    NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic;size_t fact_bytes=0;
    uint8_t* record=malloc(NATIVE_SOURCE_FACT_BYTES);
    bool fact_ok=record&&definition&&nativeSourceAssets_Copy(definition,&facts,&diagnostic)&&
        nativeSourceAssets_Write(&facts,record,NATIVE_SOURCE_FACT_BYTES,&fact_bytes,&diagnostic);
    if (path == NULL || title == NULL || !title_valid(title) ||
        definition == NULL || base_name == NULL || !fact_ok ||
        fighter == NULL || animation == NULL || costume == NULL ||
        fighter_size < 32 || animation_size < 32 || costume_size < 32 ||
        fighter_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
        animation_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES - fighter_size ||
        costume_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES - fighter_size -
                           animation_size ||
        !CreateDirectoryA(path, NULL)) {
        free(record);return false;
    }
    bool ok = write_asset(path, "fighter.dat", fighter, fighter_size) &&
              write_asset(path, "animation.dat", animation, animation_size) &&
              write_asset(path, "costume.dat", costume, costume_size)&&
              write_asset(path, "assets.fma", record, fact_bytes);
    free(record);
    if (ok && cpu != NULL) {
        char cpu_path[PROJECT_PATH_CAPACITY];
        FILE* output = project_path(cpu_path, path, "cpu_profile.fcp")
            ? fopen(cpu_path, "wb") : NULL;
        ok = output != NULL && nativeFighterCpu_Write(output, cpu);
        if (output != NULL) ok = fclose(output) == 0 && ok;
    }
    if (ok) {
        char manifest[128];
        int length = snprintf(manifest, sizeof(manifest),
                              "title=%s\ntemplate=%s\n",title,definition->code_key);
        ok = length > 0 && length < sizeof(manifest) &&
             write_asset(path, "project.txt", manifest, (size_t) length);
    }
    return ok;
}

static bool copy_demo_from_disc(const NativeFighterDefinition* definition,
                                FighterKind source_kind,
                                const NativeFighterDemoCopyInput* input,
                                NativeFighterDemoCopyOutput* output)
{
    // The stock source catalog chooses archives at construction only. The
    // copied descriptor table and clips need no stock identity at runtime.
    CharacterKind character = ChKind_None;
    for (int i = 0; i < 26; ++i) {
        if (Player_800325C8((CharacterKind)i, false) == source_kind) {
            character = (CharacterKind)i;
            break;
        }
    }
    if (character == ChKind_None) for (int i = 0; i < 26; ++i) {
        if (Player_800325C8((CharacterKind)i, true) == source_kind) {
            character = (CharacterKind)i;
            break;
        }
    }
    if (character == ChKind_None ||
        (definition->demo_count != 14 && definition->demo_count != 16) ||
        definition->demo_motion_names == NULL) return false;
    const Fighter_DemoStrings* names = definition->demo_motion_names;
    const Fighter_DemoStrings* wait_names = names;
    if (source_kind == Ft_Kind_Nana) {
        // Retail Nana's video-wait descriptor names the primary clip. Copy
        // those immutable bytes explicitly rather than adding a live fallback.
        const NativeFighterDefinition* primary =
            nativeFighterRuntime_FindStockDefinition(Ft_Kind_Popo);
        if (primary == NULL || primary->demo_motion_names == NULL) return false;
        wait_names = primary->demo_motion_names;
    }
    const char* filenames[6] = {gm_80160438(character), "IrAls.dat",
        "GmRegEnd.dat", viGetCharAnimByIndex(character), "Vi0102.dat", "Vi1101.dat"};
    const char* roots[6] = {names->result_filename, names->intro_filename,
        names->ending_filename, wait_names->vi_wait_filename, NULL, NULL};
    unsigned source_count = definition->demo_count == 16 ? 6 : 4;
    if (source_count == 6) {
        Fighter_MotionFileStringGetter getter = ftData_803C24EC[source_kind];
        if (getter == NULL) return false;
        roots[4] = getter(9);
        roots[5] = getter(10);
    }
    const uint32_t first[6] = {0, 10, 12, 13, 14, 15}, counts[6] = {10, 2, 1, 1, 1, 1};
    void* archives[6] = {0};
    size_t sizes[6] = {0};
    NativeFighterDemoCopySource sources[6] = {0};
    bool ok = true;
    for (unsigned i = 0; i < source_count && ok; ++i) {
        ok = filenames[i] != NULL && roots[i] != NULL &&
            nativeDiscArchive_Read(filenames[i], archives + i, sizes + i);
        sources[i] = (NativeFighterDemoCopySource){archives[i], sizes[i],
                                                  roots[i], first[i], counts[i]};
    }
    if (ok) ok = nativeFighterDemoCopy_Materialize(input, sources, source_count, output);
    for (unsigned i = 0; i < source_count; ++i) _aligned_free(archives[i]);
    return ok;
}

bool nativeFighterProject_CreateStockFromDisc(const char* path,
                                              const char* title,
                                              FighterKind source_kind)
{
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_FindStockDefinition(source_kind);
    if (definition == NULL)
        return false;
    const Fighter_CostumeStrings* names = ftData_803C2360[source_kind];
    if (names == NULL || names[0].dat_filename == NULL) {
        return false;
    }
    void *fighter = NULL, *animation = NULL, *costume = NULL, *common = NULL;
    void *shared_fighter = NULL, *shared_animation = NULL, *owned_articles = NULL;
    size_t shared_fighter_size = 0, shared_animation_size = 0;
    NativeFighterAnimationCopyOutput owned = {0};
    NativeFighterDemoCopyOutput demos = {0};
    size_t common_size = 0;
    NativeFighterCpuProfile cpu;
    size_t fighter_size = 0, animation_size = 0, costume_size = 0;
    bool ok = nativeDiscArchive_Read(ftData_803C1F40[source_kind].filename,
                                &fighter, &fighter_size) &&
              nativeDiscArchive_Read(ftData_803C23E4[source_kind], &animation,
                                &animation_size) &&
              nativeDiscArchive_Read(names[0].dat_filename, &costume,
                                &costume_size) &&
              nativeDiscArchive_Read("PlCo.dat", &common, &common_size) &&
              nativeFighterCpu_ImportStock(common, common_size, source_kind, &cpu);
    if (ok && source_kind == Ft_Kind_Nana) {
        // Retail Nana borrows Popo's action clips and registered article table.
        // Copy these at construction; saved assets need no live donor fighter.
        const NativeFighterDefinition* primary =
            nativeFighterRuntime_FindStockDefinition(Ft_Kind_Popo);
        HSD_Dat destination, source;
        u32 destination_root, source_root;
        size_t article_size = 0;
        ok = primary != NULL &&
             nativeDiscArchive_Read(ftData_803C1F40[Ft_Kind_Popo].filename,
                                    &shared_fighter, &shared_fighter_size) &&
             nativeDiscArchive_Read(ftData_803C23E4[Ft_Kind_Popo],
                                    &shared_animation, &shared_animation_size) &&
             HSD_DatOpen(&destination, fighter, fighter_size) == HSD_DAT_OK &&
             HSD_DatOpen(&source, shared_fighter, shared_fighter_size) == HSD_DAT_OK &&
             HSD_DatFindRoot(&destination, definition->fighter_root, &destination_root) &&
             HSD_DatFindRoot(&source, primary->fighter_root, &source_root) &&
             HSD_DatBytes(&destination, destination_root, 0x4C) != NULL &&
             HSD_DatBytes(&source, source_root, 0x4C) != NULL &&
             nativeFighterDatCopy_PointerField(fighter, fighter_size,
                 (uint32_t)destination_root + 0x48, shared_fighter, shared_fighter_size,
                 (uint32_t)source_root + 0x48, NULL, 0, &article_size, NULL);
        if (ok) {
            owned_articles = malloc(article_size);
            ok = owned_articles != NULL &&
                 nativeFighterDatCopy_PointerField(fighter, fighter_size,
                     (uint32_t)destination_root + 0x48, shared_fighter, shared_fighter_size,
                     (uint32_t)source_root + 0x48, owned_articles, article_size, NULL, NULL);
        }
        if (ok) {
            const NativeFighterAnimationCopyInput destination_input = {
                owned_articles, article_size, definition->fighter_root,
                definition->action_count, animation, animation_size,
            };
            const NativeFighterAnimationCopyInput source_input = {
                shared_fighter, shared_fighter_size, primary->fighter_root,
                primary->action_count, shared_animation, shared_animation_size,
            };
            ok = nativeFighterAnimationCopy_Materialize(
                &destination_input, &source_input, &owned);
        }
    }
    if (ok) {
        const NativeFighterDemoCopyInput input = {
            owned.fighter_dat ? owned.fighter_dat : fighter,
            owned.fighter_dat ? owned.fighter_size : fighter_size,
            definition->fighter_root, definition->demo_count,
            owned.animation_dat ? owned.animation_dat : animation,
            owned.animation_dat ? owned.animation_size : animation_size,
        };
        ok = copy_demo_from_disc(definition, source_kind, &input, &demos);
    }
    if (ok) ok = nativeFighterProject_CreateStock(path, title, source_kind,
        demos.fighter_dat, demos.fighter_size, demos.animation_dat,
        demos.animation_size, costume, costume_size, &cpu);
    nativeFighterDemoCopy_Free(&demos);
    nativeFighterAnimationCopy_Free(&owned);
    free(owned_articles);
    _aligned_free(shared_fighter);
    _aligned_free(shared_animation);
    _aligned_free(fighter);
    _aligned_free(animation);
    _aligned_free(costume);
    _aligned_free(common);
    return ok;
}

bool nativeFighterProject_ImportCpuFromDisc(const char* path, FighterKind kind)
{
    void* common = NULL;
    size_t size = 0;
    NativeFighterCpuProfile cpu;
    char temporary[PROJECT_PATH_CAPACITY], destination[PROJECT_PATH_CAPACITY];
    bool ok = project_path(temporary, path, "cpu_profile.fcp.new") &&
              project_path(destination, path, "cpu_profile.fcp") &&
              nativeDiscArchive_Read("PlCo.dat", &common, &size) &&
              nativeFighterCpu_ImportStock(common, size, kind, &cpu);
    _aligned_free(common);
    if (!ok) return false;
    FILE* output = fopen(temporary, "wb");
    ok = output != NULL && nativeFighterCpu_Write(output, &cpu);
    if (output != NULL) ok = fclose(output) == 0 && ok;
    if (ok) ok = MoveFileExA(temporary, destination,
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) DeleteFileA(temporary);
    return ok;
}

bool nativeFighterProject_CreateFromMember(const char* path,const char* title,
    const NativeFighterPackageMember* member,const NativeFighterMemberData* data,const void* portrait_png,size_t portrait_size)
{
    NativeFighterCpuProfile cpu;NativeScriptDiagnostic diagnostic;NativeSourceAssetFacts facts;
    if(!path||!member||!data||!portrait_png||!portrait_size||!title_valid(title)||!data->script_size||
       !nativeFighterCpu_Parse((const char*)data->cpu_text,data->cpu_size,&cpu)||
       !nativeSourceAssets_Read(data->asset_record,data->asset_record_size,&facts,&diagnostic))return false;
    NativeSourcePackage* candidate=calloc(1,sizeof(*candidate));if(!candidate)return false;
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=data->asset_record,.facts_bytes=data->asset_record_size,
        .assets={data->fighter_dat,data->fighter_size,data->animation_dat,data->animation_size,data->costume_dat,data->costume_size},
        .source=(const char*)data->script_text,.source_bytes=data->script_size,.parameters=(const char*)data->parameter_text,.parameter_bytes=data->parameter_size,
        .resources=data->resources,.resource_bytes=data->resource_size};
    bool validated=nativeSourcePackage_Prepare(candidate,&input,Ft_Kind_CustomFirst,NULL,&diagnostic);nativeSourcePackage_Release(candidate);free(candidate);
    if(!validated||!CreateDirectoryA(path,NULL))return false;
    bool ok=write_asset(path,"fighter.dat",data->fighter_dat,data->fighter_size)&&write_asset(path,"animation.dat",data->animation_dat,data->animation_size)&&
        write_asset(path,"costume.dat",data->costume_dat,data->costume_size)&&write_asset(path,"assets.fma",data->asset_record,data->asset_record_size)&&
        write_asset(path,"portrait.png",portrait_png,portrait_size)&&write_asset(path,"behavior.fms",data->script_text,data->script_size)&&
        write_asset(path,"cpu_profile.fcp",data->cpu_text,data->cpu_size);
    if(ok&&data->parameter_size)ok=write_asset(path,"fighter_parameters.fmp",data->parameter_text,data->parameter_size);
    if(ok&&data->resource_size)ok=write_asset(path,"resources.fmr",data->resources,data->resource_size);
    if(ok){char manifest[160];int length=snprintf(manifest,sizeof(manifest),"title=%s\ntemplate=custom\n",title);ok=length>0&&length<sizeof(manifest)&&write_asset(path,"project.txt",manifest,(size_t)length);}
    return ok;
}

static bool asset_size(const char* root, const char* name, size_t* size)
{
    char path[PROJECT_PATH_CAPACITY];
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!project_path(path, root, name) ||
        !GetFileAttributesExA(path, GetFileExInfoStandard, &data) ||
        (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return false;
    }
    uint64_t length = ((uint64_t) data.nFileSizeHigh << 32) |
                      data.nFileSizeLow;
    if (length < 32 || length > NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
        return false;
    }
    *size = (size_t) length;
    return true;
}

bool nativeFighterProject_ReadInfo(const char* path,NativeFighterProjectInfo* info)
{
    if(!path||!info)return false;
    uint8_t* record=NULL;size_t record_bytes=0;NativeScriptDiagnostic diagnostic;
    NativeFighterProjectInfo candidate={0};
    if(!read_asset(path,"assets.fma",&record,&record_bytes))return false;
    bool decoded=nativeSourceAssets_Read(record,record_bytes,&candidate.facts,&diagnostic);free(record);if(!decoded)return false;
    uint8_t* text=NULL;size_t size=0;
    if(!read_asset(path,"project.txt",&text,&size)||!size||size>=160||text[size-1]!='\n'||memchr(text,0,size)){free(text);return false;}
    char manifest[160];memcpy(manifest,text,size);manifest[size]=0;free(text);
    if(strncmp(manifest,"title=",6))return false;char* title_end=strchr(manifest,'\n');if(!title_end)return false;*title_end=0;
    char* hint=title_end+1;if(strncmp(hint,"template=",9))return false;hint+=9;char* hint_end=strchr(hint,'\n');if(!hint_end||hint_end[1])return false;*hint_end=0;
    if(!title_valid(manifest+6)||!template_key_valid(hint))return false;
    size_t fighter_size,animation_size,costume_size;
    if(!asset_size(path,"fighter.dat",&fighter_size)||!asset_size(path,"animation.dat",&animation_size)||!asset_size(path,"costume.dat",&costume_size)||
       fighter_size+animation_size+costume_size>NATIVE_CONTENT_MAX_PACKAGE_BYTES)return false;
    candidate.base_kind=(FighterKind)candidate.facts.animation_tag; // Authoring hint, never a profile lookup.
    strcpy(candidate.fighter_root,candidate.facts.fighter_root);candidate.action_count=candidate.facts.actions;candidate.demo_count=candidate.facts.demo_count;
    strcpy(candidate.title,manifest+6);strcpy(candidate.template_key,hint);*info=candidate;return true;
}

bool nativeFighterProject_Open(const char* path, NativeFighterProject* project)
{
    if (project == NULL) {
        return false;
    }
    *project = (NativeFighterProject){ 0 };
    NativeFighterProjectInfo info;
    if (!nativeFighterProject_ReadInfo(path, &info)) {
        return false;
    }
    project->base_kind = info.base_kind;
    strcpy(project->fighter_root, info.fighter_root);
    project->action_count = info.action_count;
    project->demo_count = info.demo_count;
    strcpy(project->title, info.title);
    strcpy(project->template_key, info.template_key);
    project->facts=info.facts;
    bool ok = read_asset(path, "assets.fma", &project->asset_record, &project->asset_record_size) && read_asset(path, "fighter.dat", &project->fighter_dat,
                    &project->fighter_size) &&
         read_asset(path, "animation.dat", &project->animation_dat,
                    &project->animation_size) &&
         read_asset(path, "costume.dat", &project->costume_dat,
                    &project->costume_size);
    ok = ok &&
         read_optional_asset(path, "behavior.fms", &project->script_text,
                             &project->script_size) &&
         read_optional_asset(path, "fighter_parameters.fmp",
                             &project->parameter_text,
                             &project->parameter_size) &&
         read_optional_asset(path, "cpu_profile.fcp", &project->cpu_text,
                             &project->cpu_size) &&
         read_optional_asset(path, "resources.fmr", &project->resources,
                             &project->resource_size);
    NativeSourceResources resources;
    ok = ok && nativeSourceResources_Read(project->resources,project->resource_size,
                                          &resources,NULL,0);
    NativeFighterCpuProfile cpu;
    ok = ok && nativeFighterCpu_Parse((const char*) project->cpu_text,
                                     project->cpu_size, &cpu) && nativeFighterAuthoring_Load(path, project);
    if (!ok) {
        nativeFighterProject_Close(project);
    }
    return ok;
}

static bool common_attribute_record(const NativeFighterProject* project,
                                    HSD_Dat* dat, u32* attributes)
{
    u32 root;
    const char* external;
    return project != NULL && project->action_count > 0 &&
           HSD_DatOpen(dat, project->fighter_dat,
                       project->fighter_size) == HSD_DAT_OK &&
           HSD_DatFindRoot(dat, project->fighter_root, &root) &&
           HSD_DatResolve(dat, root, attributes, &external) ==
               HSD_DAT_REF_LOCAL &&
           HSD_DatBytes(dat, *attributes, 0x181) != NULL;
}

bool nativeFighterProject_GetAttribute(const NativeFighterProject* project,
                                       size_t property, double* value)
{
    HSD_Dat dat;
    u32 attributes;
    if (value == NULL || property >= NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT ||
        !common_attribute_record(project, &dat, &attributes)) return false;
    const NativeFighterAttribute* field = &nativeFighterCommonAttributes[property];
    if (field->type == NativeAttribute_Mask) {
        const u8* byte = HSD_DatBytes(&dat, attributes + field->offset, 1);
        if (byte == NULL) return false;
        *value = *byte;
    } else if (field->type == NativeAttribute_Int) {
        u32 bits;
        if (!HSD_DatReadU32(&dat, attributes + field->offset, &bits)) return false;
        *value = (s32)bits;
    } else {
        float number;
        if (!HSD_DatReadF32(&dat, attributes + field->offset, &number) || !isfinite(number)) return false;
        *value = number;
    }
    return true;
}

bool nativeFighterProject_SetAttribute(NativeFighterProject* project,
                                       size_t property, double value)
{
    if (property >= NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT || !isfinite(value)) return false;
    const NativeFighterAttribute* field = &nativeFighterCommonAttributes[property];
    const bool integral = field->type != NativeAttribute_Float;
    if (integral && (floor(value) != value ||
        value < (field->type == NativeAttribute_Mask ? 0.0 : -2147483648.0) ||
        value > (field->type == NativeAttribute_Mask ? 255.0 : 2147483647.0))) return false;
    if (field->type == NativeAttribute_Float && !isfinite((float)value)) return false;
    if ((field->offset == 0x88 || field->offset == 0x8c) && value <= 0.0) return false;
    if (field->offset == 0x58 && value < 1.0) return false;
    HSD_Dat dat;
    u32 attributes;
    if (!common_attribute_record(project, &dat, &attributes)) return false;
    const size_t size = field->type == NativeAttribute_Mask ? 1 : 4;
    const u8* original = HSD_DatBytes(&dat, attributes + field->offset, size);
    if (original == NULL) return false;
    u8* bytes = project->fighter_dat + (original - (const u8*)project->fighter_dat);
    u8 previous[4];
    memcpy(previous, bytes, size);
    if (field->type == NativeAttribute_Mask) bytes[0] = (u8)value;
    else {
        u32 bits;
        if (integral) bits = (u32)(s32)value;
        else { float number = (float)value; memcpy(&bits, &number, sizeof(bits)); }
        bytes[0] = (u8)(bits >> 24); bytes[1] = (u8)(bits >> 16);
        bytes[2] = (u8)(bits >> 8); bytes[3] = (u8)bits;
    }
    ftCo_DatAttrs decoded;
    if (ftData_DecodeCommonAttrs(&dat, attributes, &decoded) != HSD_DAT_OK) {
        memcpy(bytes, previous, size); return false;
    }
    return true;
}

static bool save_asset_with_previous(const char* path, const char* name,
                                     const void* bytes, size_t size)
{
    char temp_name[48], previous_name[48], previous_temp_name[48];
    if (snprintf(temp_name, sizeof(temp_name), "%s.tmp", name) >=
            sizeof(temp_name) ||
        snprintf(previous_name, sizeof(previous_name), "%s.previous", name) >=
            sizeof(previous_name) ||
        snprintf(previous_temp_name, sizeof(previous_temp_name),
                 "%s.previous.tmp", name) >= sizeof(previous_temp_name))
        return false;
    char temporary[PROJECT_PATH_CAPACITY];
    char destination[PROJECT_PATH_CAPACITY];
    char previous[PROJECT_PATH_CAPACITY];
    char previous_temporary[PROJECT_PATH_CAPACITY];
    if (!project_path(temporary, path, temp_name) ||
        !project_path(destination, path, name) ||
        !project_path(previous, path, previous_name) ||
        !project_path(previous_temporary, path, previous_temp_name))
        return false;
    bool saved = write_asset(path, temp_name, bytes, size) &&
                 CopyFileA(destination, previous_temporary, FALSE) &&
                 MoveFileExA(previous_temporary, previous,
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) &&
                 MoveFileExA(temporary, destination,
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!saved) {
        DeleteFileA(temporary);
        DeleteFileA(previous_temporary);
    }
    return saved;
}

bool nativeFighterProject_Rename(const char* path, const char* title)
{
    NativeFighterProjectInfo info;
    if (path == NULL || title == NULL || !title_valid(title) ||
        !nativeFighterProject_ReadInfo(path, &info))
        return false;
    char manifest[160];
    int size = snprintf(manifest, sizeof(manifest), "title=%s\ntemplate=%s\n",
                        title, info.template_key);
    return size > 0 && size < sizeof(manifest) &&
        save_asset_with_previous(path, "project.txt", manifest, (size_t)size);
}

bool nativeFighterProject_Save(const char* path,
                               const NativeFighterProject* project)
{
    if (path == NULL || project == NULL ||
        project->action_count == 0 ||
        project->fighter_dat == NULL || !title_valid(project->title) ||
        !template_key_valid(project->template_key)) {
        return false;
    }
    HSD_Dat dat;
    u32 attributes;
    ftCo_DatAttrs decoded;
    if (!common_attribute_record(project, &dat, &attributes) ||
        ftData_DecodeCommonAttrs(&dat, attributes, &decoded) != HSD_DAT_OK) {
        return false;
    }
    NativeFighterProject disk;
    if (!nativeFighterProject_Open(path, &disk)) {
        return false;
    }
    bool same_project = disk.asset_record_size==project->asset_record_size&&project->asset_record&&
                        !memcmp(disk.asset_record,project->asset_record,project->asset_record_size)&&disk.base_kind == project->base_kind &&
                        strcmp(disk.title, project->title) == 0 &&
                        strcmp(disk.template_key, project->template_key) == 0;
    bool changed = disk.fighter_size != project->fighter_size ||
                   memcmp(disk.fighter_dat, project->fighter_dat,
                          project->fighter_size) != 0;
    nativeFighterProject_Close(&disk);
    if (!same_project) {
        return false;
    }
    if (!changed) return true;
    return save_asset_with_previous(path, "fighter.dat", project->fighter_dat,
                                    project->fighter_size);
}

bool nativeFighterProject_SaveAnimation(const char* path,
                                        const NativeFighterProject* project)
{
    if (path == NULL || project == NULL ||
        project->action_count == 0 ||
        project->animation_dat == NULL || project->animation_size < 32 ||
        project->animation_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES) return false;
    NativeFighterProject disk;
    if (!nativeFighterProject_Open(path, &disk)) return false;
    bool same_project = disk.asset_record_size==project->asset_record_size&&project->asset_record&&
                        !memcmp(disk.asset_record,project->asset_record,project->asset_record_size)&&disk.base_kind == project->base_kind &&
                        strcmp(disk.title, project->title) == 0 &&
                        strcmp(disk.template_key, project->template_key) == 0;
    bool changed = disk.animation_size != project->animation_size ||
                   memcmp(disk.animation_dat, project->animation_dat,
                          project->animation_size) != 0;
    HSD_Dat dat;
    u32 attributes;
    ftCo_DatAttrs decoded;
    same_project = same_project && common_attribute_record(project, &dat, &attributes) &&
        ftData_DecodeCommonAttrs(&dat, attributes, &decoded) == HSD_DAT_OK;
    if (!same_project || !changed) {
        nativeFighterProject_Close(&disk);
        return same_project;
    }
    // A growing clip changes addresses in both descriptor tables. Stage the
    // matching fighter/bank pair and their predecessor before replacing either
    // current file. This is an operation-level rollback, not a power-loss journal.
    const char* names[] = { "fighter.dat", "animation.dat",
        "animation.fighter.previous", "animation.dat.previous" };
    const char* temporary_names[] = { "fighter.dat.animation.tmp", "animation.dat.tmp",
        "animation.fighter.previous.tmp", "animation.dat.previous.tmp" };
    const void* bytes[] = { project->fighter_dat, project->animation_dat,
                           disk.fighter_dat, disk.animation_dat };
    const size_t sizes[] = { project->fighter_size, project->animation_size,
                            disk.fighter_size, disk.animation_size };
    char destinations[4][PROJECT_PATH_CAPACITY], temporary[4][PROJECT_PATH_CAPACITY];
    bool saved = true;
    for (unsigned i = 0; i < 4 && saved; ++i)
        saved = project_path(destinations[i], path, names[i]) &&
            project_path(temporary[i], path, temporary_names[i]);
    if (!saved) { nativeFighterProject_Close(&disk); return false; }
    for (unsigned i = 0; i < 4 && saved; ++i)
        saved = write_asset(path, temporary_names[i], bytes[i], sizes[i]);
    const DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
    if (saved) saved = MoveFileExA(temporary[2], destinations[2], flags) &&
                       MoveFileExA(temporary[3], destinations[3], flags);
    bool fighter_replaced = saved && MoveFileExA(temporary[0], destinations[0], flags);
    saved = fighter_replaced && MoveFileExA(temporary[1], destinations[1], flags);
    if (fighter_replaced && !saved) {
        // Keep the current pair consistent when the second file is locked or
        // cannot be replaced. The independent undo snapshot remains available.
        if (!write_asset(path, temporary_names[0], disk.fighter_dat, disk.fighter_size) ||
            !MoveFileExA(temporary[0], destinations[0], flags))
            fprintf(stderr, "Could not roll back fighter animation addresses in %s\n", path);
    }
    for (unsigned i = 0; i < 4; ++i) DeleteFileA(temporary[i]);
    nativeFighterProject_Close(&disk);
    return saved;
}

bool nativeFighterProject_UndoLastAnimationSave(const char* path)
{
    if (path == NULL) return false;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return false;
    uint8_t* previous = NULL;
    uint8_t* previous_fighter = NULL;
    size_t previous_size = 0;
    size_t previous_fighter_size = 0;
    bool read = read_asset(path, "animation.dat.previous", &previous,
                           &previous_size) &&
        read_asset(path, "animation.fighter.previous", &previous_fighter, &previous_fighter_size) &&
        nativeFighterAnimation_RestoreDescriptors(&project, previous_fighter, previous_fighter_size);
    free(previous_fighter);
    if (read) {
        free(project.animation_dat);
        project.animation_dat = previous;
        project.animation_size = previous_size;
    }
    if (!read) free(previous);
    bool restored = read && nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    return restored;
}

bool nativeFighterProject_SaveCostume(const char* path,
                                     const NativeFighterProject* project)
{
    if (path == NULL || project == NULL || project->costume_dat == NULL ||
        project->costume_size < 32 ||
        project->costume_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES)
        return false;
    HSD_Dat dat;
    if (HSD_DatOpen(&dat, project->costume_dat,
                    project->costume_size) != HSD_DAT_OK) return false;
    NativeSourceImportedAssets* imported=calloc(1,sizeof(*imported));if(!imported)return false;
    NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic;
    NativeSourceAssetInput input={project->fighter_dat,project->fighter_size,project->animation_dat,project->animation_size,project->costume_dat,project->costume_size};
    bool profile_valid=nativeSourceAssets_Read(project->asset_record,project->asset_record_size,&facts,&diagnostic)&&
        nativeSourceAssets_Import(&facts,&input,Ft_Kind_CustomFirst,imported,&diagnostic);
    nativeSourceAssets_Release(imported);free(imported);if(!profile_valid)return false;
    NativeFighterProject disk;
    if (!nativeFighterProject_Open(path, &disk)) return false;
    const bool same_project = disk.asset_record_size==project->asset_record_size&&project->asset_record&&
        !memcmp(disk.asset_record,project->asset_record,project->asset_record_size)&&disk.base_kind == project->base_kind &&
        strcmp(disk.title, project->title) == 0 &&
        strcmp(disk.template_key, project->template_key) == 0;
    const bool changed = disk.costume_size != project->costume_size ||
        memcmp(disk.costume_dat, project->costume_dat,
               project->costume_size) != 0;
    if(!same_project||!changed) { nativeFighterProject_Close(&disk); return same_project; }
    // Mesh insertion changes the costume traversal and the fighter's numeric
    // draw/texture references together. Publish the coherent pair.
    const char* names[]={"fighter.dat","costume.dat","costume.dat.previous"};
    const char* temporary_names[]={"fighter.dat.model.tmp","costume.dat.tmp","costume.dat.previous.tmp"};
    const void* bytes[]={project->fighter_dat,project->costume_dat,disk.costume_dat};
    size_t sizes[]={project->fighter_size,project->costume_size,disk.costume_size};
    char destinations[3][PROJECT_PATH_CAPACITY]={{0}},temporary[3][PROJECT_PATH_CAPACITY]={{0}};
    bool saved=true;
    for(unsigned i=0;i<3&&saved;++i)
        saved=project_path(destinations[i],path,names[i])&&project_path(temporary[i],path,temporary_names[i])&&write_asset(path,temporary_names[i],bytes[i],sizes[i]);
    DWORD flags=MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH;
    if(saved)saved=MoveFileExA(temporary[2],destinations[2],flags);
    bool replaced=saved&&MoveFileExA(temporary[0],destinations[0],flags);
    saved=replaced&&MoveFileExA(temporary[1],destinations[1],flags);
    if(replaced&&!saved) {
        if(!write_asset(path,temporary_names[0],disk.fighter_dat,disk.fighter_size)||!MoveFileExA(temporary[0],destinations[0],flags))
            fprintf(stderr,"Could not restore model references in %s\n",path);
    }
    for(unsigned i=0;i<3;++i)if(temporary[i][0])DeleteFileA(temporary[i]);
    nativeFighterProject_Close(&disk);
    return saved;
}

bool nativeFighterProject_UndoLastCostumeSave(const char* path)
{
    if (path == NULL) return false;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return false;
    uint8_t* previous = NULL;
    size_t previous_size = 0;
    const bool read = read_asset(path, "costume.dat.previous", &previous,
                                 &previous_size);
    if (read) {
        NativeFighterModelEditor* before=calloc(1,sizeof(*before));
        NativeFighterModelEditor* after=calloc(1,sizeof(*after));
        NativeFighterProject candidate=project;candidate.costume_dat=previous;candidate.costume_size=previous_size;
        bool mapped=before&&after&&nativeFighterModel_Open(before,&project)&&nativeFighterModel_Open(after,&candidate)&&
            nativeFighterModel_RemapSelectors(before,after,&project);
        free(before);free(after);
        if(!mapped) {free(previous);nativeFighterProject_Close(&project);return false;}
        free(project.costume_dat);
        project.costume_dat = previous;
        project.costume_size = previous_size;
    }
    const bool restored = read && nativeFighterProject_SaveCostume(
        path, &project);
    nativeFighterProject_Close(&project);
    return restored;
}

bool nativeFighterProject_UndoLastSave(const char* path)
{
    if (path == NULL) return false;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return false;
    uint8_t* previous = NULL;
    size_t previous_size = 0;
    bool read = read_asset(path, "fighter.dat.previous", &previous,
                           &previous_size);
    if (read) {
        uint8_t* current = project.fighter_dat;
        size_t current_size = project.fighter_size;
        project.fighter_dat = previous;
        project.fighter_size = previous_size;
        // This is fighter-data undo, not animation undo. Preserve the current
        // bank's addresses after any intervening animation save.
        read = nativeFighterAnimation_RestoreDescriptors(&project, current, current_size) &&
            nativeFighterModel_RestoreSelectors(&project,current,current_size);
        free(current);
    }
    bool restored = read && nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    return restored;
}

void nativeFighterProject_Close(NativeFighterProject* project)
{
    if (project == NULL) {
        return;
    }
    free(project->fighter_dat);
    free(project->animation_dat);
    free(project->costume_dat);
    free(project->asset_record);
    free(project->resources);
    free(project->script_text);
    free(project->parameter_text);
    free(project->cpu_text);
    free(project->authoring);
    *project = (NativeFighterProject){ 0 };
}

#endif
