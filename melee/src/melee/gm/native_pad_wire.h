#pragma once
#include <dolphin/pad.h>
#include <stdint.h>
#include <string.h>

enum { NATIVE_PAD_WIRE_BYTES = 15 };

static inline void nativePadWire_Write(uint8_t out[NATIVE_PAD_WIRE_BYTES], const PADStatus* p)
{
    out[0]=(uint8_t)p->button;out[1]=(uint8_t)(p->button>>8);
    out[2]=(uint8_t)p->stickX;out[3]=(uint8_t)p->stickY;
    out[4]=(uint8_t)p->substickX;out[5]=(uint8_t)p->substickY;
    out[6]=p->triggerLeft;out[7]=p->triggerRight;
    out[8]=p->analogA;out[9]=p->analogB;out[10]=(uint8_t)p->err;
    for(unsigned i=0;i<4;++i)out[11+i]=(uint8_t)(p->extButton>>(i*8));
}

static inline void nativePadWire_Read(PADStatus* p, const uint8_t in[NATIVE_PAD_WIRE_BYTES])
{
    memset(p,0,sizeof(*p));
    p->button=(uint16_t)in[0]|(uint16_t)in[1]<<8;
    p->stickX=(int8_t)in[2];p->stickY=(int8_t)in[3];
    p->substickX=(int8_t)in[4];p->substickY=(int8_t)in[5];
    p->triggerLeft=in[6];p->triggerRight=in[7];
    p->analogA=in[8];p->analogB=in[9];p->err=(int8_t)in[10];
    for(unsigned i=0;i<4;++i)p->extButton|=(uint32_t)in[11+i]<<(i*8);
}
