#ifndef NATIVE_FIGHTER_ITEM_TABLES_H
#define NATIVE_FIGHTER_ITEM_TABLES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum { SCRIPT_ITEM_TABLE_LIMIT = 16, SCRIPT_ITEM_TABLE_VALUES = 64 };
typedef struct ScriptItemTable {
    char name[48];
    float values[SCRIPT_ITEM_TABLE_VALUES];
    unsigned count;
} ScriptItemTable;

bool nativeScriptItemTable_Parse(ScriptItemTable*, const char* source);
void nativeScriptItemTable_Emit(const ScriptItemTable*, unsigned article,
                               unsigned table, FILE* output);
#endif
