#ifndef NATIVE_SOURCE_RESOURCE_TOOL_H
#define NATIVE_SOURCE_RESOURCE_TOOL_H
int nativeSourceResourceTool_Import(const char* project,const char* name,
    const char* kind,const char* input);
int nativeSourceResourceTool_Item(const char* project,const char* name,
    const char* dat,const char* article_offset);
int nativeSourceResourceTool_StockItem(const char* project,const char* name,
    const char* iso,const char* kind);
int nativeSourceResourceTool_ExportFacts(const char* template_name,const char* output);
#endif
