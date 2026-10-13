#ifndef MELEE_GM_NATIVE_STOCK_CONTENT_H
#define MELEE_GM_NATIVE_STOCK_CONTENT_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_STOCK_CONTENT_DIGEST_BYTES = 32,
};

const uint8_t* nativeStockContent_Digest(void);
uint64_t nativeStockContent_CharacterId(uint8_t character);
uint64_t nativeStockContent_CostumeId(uint8_t character, uint8_t color);
uint64_t nativeStockContent_StageId(uint8_t stage);
bool nativeStockContent_CharacterFromId(uint64_t id, uint8_t* character);
bool nativeStockContent_CostumeFromId(uint64_t id, uint8_t character,
                                      uint8_t* color);
bool nativeStockContent_StageFromId(uint64_t id, uint8_t* stage);

#ifdef __cplusplus
}
#endif

#endif
#endif
