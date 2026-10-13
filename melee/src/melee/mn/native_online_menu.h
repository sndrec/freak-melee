#ifndef MELEE_MN_NATIVE_ONLINE_MENU_H
#define MELEE_MN_NATIVE_ONLINE_MENU_H

#ifdef MELEE_NATIVE
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NativeOnlineMenuAction {
    NativeOnlineMenuAction_None,
    NativeOnlineMenuAction_Create,
    NativeOnlineMenuAction_Join,
    NativeOnlineMenuAction_Leave,
    NativeOnlineMenuAction_StartMatch,
} NativeOnlineMenuAction;

// UI events queue intentions; the home selection/practice tick consumes them.
bool nativeOnlineMenu_Request(NativeOnlineMenuAction action, const char* name,
                              const char* code, uint8_t local_players);
bool nativeOnlineMenu_IsAvailable(void);
void nativeOnlineMenu_Pump(void);
void nativeOnlineMenu_Focus(void);

#ifdef __cplusplus
}
#endif
#endif
#endif
