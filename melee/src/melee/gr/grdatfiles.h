#ifndef GALE01_1C5FC0
#define GALE01_1C5FC0

#include <Runtime/platform.h>

#include <melee/gr/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <melee/gm/native_match_arena.h>

/* 1C5FC0 */ void grDatFiles_801C5FC0(HSD_Archive*, void* data, size_t length);
/* 1C6038 */ void grDatFiles_801C6038(void*, s32, s32);
/* 1C6288 */ void grDatFiles_801C6288(void);
/* 1C6324 */ UnkArchiveStruct* grDatFiles_GetArchive(void);
/* 1C6330 */ UnkArchiveStruct* grDatFiles_801C6330(s32);
UnkArchiveStruct* grDatFiles_ActivateFragment(const char* filename);
bool grDatFiles_AdmitHistory(NativeMatchArena* arena);

#endif
