#include "native_presentation.h"
#include "native_content_store.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static bool custom_models=true;
static char settings_path[560];

void nativePresentation_Load(void)
{
    char root[512];
    custom_models=true; settings_path[0]=0;
    if (!nativeContentStore_DefaultRoot(root,sizeof(root))) return;
    snprintf(settings_path,sizeof(settings_path),"%s/presentation.txt",root);
    FILE* file=fopen(settings_path,"rb");
    if (!file) return;
    unsigned value;
    if (fscanf(file,"custom_models=%u",&value)==1 && value<=1) custom_models=value!=0;
    fclose(file);
}

bool nativePresentation_CustomModels(void) { return custom_models; }

bool nativePresentation_SetCustomModels(bool show)
{
    if (!settings_path[0]) return false;
    char temporary[576]; snprintf(temporary,sizeof(temporary),"%s.new",settings_path);
    FILE* file=fopen(temporary,"wb");
    if (!file) return false;
    bool ok=fprintf(file,"custom_models=%u\n",show)>0;
    ok=fclose(file)==0 && ok;
    if (!ok || !MoveFileExA(temporary,settings_path,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary); return false;
    }
    custom_models=show; return true;
}
