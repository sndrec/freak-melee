#ifndef NATIVE_SOURCE_CONTENT_TOOL_H
#define NATIVE_SOURCE_CONTENT_TOOL_H
#include <stdbool.h>
int nativeSourceContentTool_Check(const char* project,const char* source,bool save,bool audit);
int nativeSourceContentTool_Package(const char* cache,const char* project,const char* portrait);
int nativeSourceContentTool_Verify(const char* cache,const char* id);
#endif
