#include "native_stock_content.h"

#ifdef MELEE_NATIVE

#include <melee/ft/forward.h>
#include <melee/gr/forward.h>

#include <stddef.h>

#include "gm_1601.h"

static const uint8_t stock_content_digest[NATIVE_STOCK_CONTENT_DIGEST_BYTES] = {
    0xE7, 0x3B, 0x33, 0xE5, 0x60, 0x06, 0x16, 0x3E,
    0x77, 0x4F, 0xB4, 0x17, 0xDC, 0x66, 0x16, 0xB8,
    0xF9, 0x8C, 0x10, 0x86, 0x12, 0xD3, 0x7B, 0x47,
    0x36, 0x57, 0x11, 0x4F, 0x52, 0xEB, 0x68, 0xDC,
};

const uint8_t* nativeStockContent_Digest(void)
{
    return stock_content_digest;
}

uint64_t nativeStockContent_CharacterId(uint8_t character)
{
    return UINT64_C(0x464D434800000000) |
           ((uint64_t) character + 1U);
}

uint64_t nativeStockContent_CostumeId(uint8_t character, uint8_t color)
{
    return UINT64_C(0x464D435300000000) |
           (((uint64_t) character + 1U) << 8) |
           ((uint64_t) color + 1U);
}

uint64_t nativeStockContent_StageId(uint8_t stage)
{
    return UINT64_C(0x464D535400000000) | ((uint64_t) stage + 1U);
}

bool nativeStockContent_CharacterFromId(uint64_t id, uint8_t* character)
{
    if (character == NULL ||
        (id & UINT64_C(0xFFFFFFFFFFFFFF00)) !=
            UINT64_C(0x464D434800000000))
    {
        return false;
    }
    uint8_t value = (uint8_t) id;
    if (value == 0) {
        return false;
    }
    value -= 1;
    switch (value) {
    case CKind_Captain:
    case CKind_Donkey:
    case CKind_Fox:
    case CKind_GameWatch:
    case CKind_Kirby:
    case CKind_Koopa:
    case CKind_Link:
    case CKind_Luigi:
    case CKind_Mario:
    case CKind_Mars:
    case CKind_Mewtwo:
    case CKind_Ness:
    case CKind_Peach:
    case CKind_Pikachu:
    case CKind_PopoNana:
    case CKind_Purin:
    case CKind_Samus:
    case CKind_Yoshi:
    case CKind_Zelda:
    case CKind_Seak:
    case CKind_Falco:
    case CKind_CLink:
    case CKind_DrMario:
    case CKind_Emblem:
    case CKind_Pichu:
    case CKind_Ganon:
        *character = value;
        return true;
    default:
        return false;
    }
}

bool nativeStockContent_CostumeFromId(uint64_t id, uint8_t character,
                                      uint8_t* color)
{
    if (color == NULL ||
        (id & UINT64_C(0xFFFFFFFFFFFF0000)) !=
            UINT64_C(0x464D435300000000))
    {
        return false;
    }
    uint8_t encoded_character = (uint8_t) (id >> 8);
    uint8_t encoded_color = (uint8_t) id;
    if (encoded_character == 0 || encoded_color == 0 ||
        encoded_character - 1 != character)
    {
        return false;
    }
    uint8_t value = encoded_color - 1;
    if (value >= gm_GetNumCostumesForCKind(character)) {
        return false;
    }
    *color = value;
    return true;
}

bool nativeStockContent_StageFromId(uint64_t id, uint8_t* stage)
{
    if (stage == NULL ||
        (id & UINT64_C(0xFFFFFFFFFFFFFF00)) !=
            UINT64_C(0x464D535400000000))
    {
        return false;
    }
    uint8_t value = (uint8_t) id;
    if (value == 0) {
        return false;
    }
    value -= 1;
    if (value < St_Kind_Izumi || value > St_Kind_Last ||
        value == St_Kind_Akaneia || value == St_Kind_Icetop)
    {
        return false;
    }
    *stage = value;
    return true;
}

#endif
