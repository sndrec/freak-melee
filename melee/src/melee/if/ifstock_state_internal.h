#ifndef MELEE_IF_STOCK_STATE_INTERNAL_H
#define MELEE_IF_STOCK_STATE_INTERNAL_H
#include "types.h"
enum { IfStock_ObjectCount=6+1+16+131+1+130 };
typedef struct IfStockState {
    struct ifStock_804A1378 hud;
    struct ifStock_804A1774 score_queue;
    HSD_GObj* stage_indicators[16];
    struct ifStock_804A1ACC fall_queue;
} IfStockState;
extern IfStockState if_stock_state;
#endif
