#include "native_disc_files.h"
#include <stdint.h>
#include <aurora/dvd.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <stdio.h>
#include <string.h>

enum { FILES = 256, PATH_BYTES = 1024 };
static struct {
    AuroraOverlayFile entries[FILES];
    char paths[FILES][PATH_BYTES], names[FILES][PATH_BYTES];
    unsigned count, root_length;
} mounted;

static void* open_file(void* path) { return SDL_IOFromFile(path,"rb"); }
static void close_file(void* handle) { SDL_CloseIO(handle); }
static int64_t read_file(void* handle, uint8_t* bytes, size_t size) { return (int64_t)SDL_ReadIO(handle,bytes,size); }
static int64_t seek_file(void* handle, int64_t offset, int32_t whence) { return SDL_SeekIO(handle,offset,(SDL_IOWhence)whence); }

static SDL_EnumerationResult discover(void* unused, const char* directory, const char* name)
{
    (void)unused;
    char path[PATH_BYTES];
    if (snprintf(path,sizeof(path),"%s%s",directory,name) >= sizeof(path)) return SDL_ENUM_FAILURE;
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path,&info)) return SDL_ENUM_FAILURE;
    if (info.type == SDL_PATHTYPE_DIRECTORY)
        return SDL_EnumerateDirectory(path,discover,NULL) ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
    if (info.type != SDL_PATHTYPE_FILE) return SDL_ENUM_CONTINUE;
    if (mounted.count == FILES || info.size > UINT32_MAX) return SDL_ENUM_FAILURE;
    unsigned i = mounted.count++;
    strcpy(mounted.paths[i],path);
    snprintf(mounted.names[i],PATH_BYTES,"/%s",path+mounted.root_length);
    for (char* p = mounted.names[i]; *p; ++p) if (*p == '\\') *p = '/';
    mounted.entries[i] = (AuroraOverlayFile){mounted.names[i],mounted.paths[i],(size_t)info.size};
    return SDL_ENUM_CONTINUE;
}

bool nativeDiscFiles_Mount(void)
{
    char root[PATH_BYTES];
    if (snprintf(root,sizeof(root),"%sdisc/",SDL_GetBasePath()) >= sizeof(root)) return false;
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(root,&info)) return true;
    mounted.count = 0; mounted.root_length = (unsigned)strlen(root);
    if (!SDL_EnumerateDirectory(root,discover,NULL)) return false;
    const AuroraOverlayCallbacks callbacks = {open_file,close_file,read_file,seek_file};
    aurora_dvd_overlay_callbacks(&callbacks);
    aurora_dvd_overlay_files(mounted.entries,mounted.count,NULL);
    fprintf(stderr,"[disc-files] Mounted %u native assets.\n",mounted.count);
    return true;
}
