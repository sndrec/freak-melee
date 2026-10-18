#include "native_match_stream.h"
#ifdef MELEE_NATIVE
#include "native_pad_wire.h"
#include <limits.h>
#include <string.h>

static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static void put(uint8_t* out,uint64_t value,unsigned bytes)
{for(unsigned i=0;i<bytes;++i)out[i]=(uint8_t)(value>>(i*8));}
static uint64_t get(const uint8_t* in,unsigned bytes)
{uint64_t value=0;for(unsigned i=0;i<bytes;++i)value|=(uint64_t)in[i]<<(i*8);return value;}
static uint64_t hash(uint64_t value,const uint8_t* bytes,size_t count)
{for(size_t i=0;i<count;++i)value=(value^bytes[i])*UINT64_C(1099511628211);return value;}

size_t nativeMatchStream_EncodeDescription(const NativeMatchStreamDescription* d,
                                          uint8_t* out,size_t capacity)
{
    if(!d || !out || capacity<8 || !d->match.session_id ||
       overlap(d,sizeof(*d),out,capacity))return 0;
    size_t n=nativeLobbyMatch_Encode(&d->match,out+8,capacity-8);
    if(!n)return 0;
    memcpy(out,"FMWM",4);put(out+4,d->initial_seed,4);return n+8;
}
bool nativeMatchStream_DecodeDescription(NativeMatchStreamDescription* d,
                                        const uint8_t* in,size_t bytes)
{
    if(!d || !in || bytes<8 || bytes>NATIVE_MATCH_STREAM_DESCRIPTION_BYTES ||
       overlap(d,sizeof(*d),in,bytes) || memcmp(in,"FMWM",4))return false;
    NativeMatchStreamDescription candidate={0};
    if(!nativeLobbyMatch_Decode(&candidate.match,in+8,bytes-8) ||
       !candidate.match.session_id)return false;
    candidate.initial_seed=(uint32_t)get(in+4,4);
    uint8_t canonical[NATIVE_MATCH_STREAM_DESCRIPTION_BYTES];
    if(nativeMatchStream_EncodeDescription(&candidate,canonical,sizeof(canonical))!=bytes ||
       memcmp(canonical,in,bytes))return false;
    *d=candidate;return true;
}
bool nativeMatchStream_Bind(NativeMatchStream* s,const NativeMatchStreamDescription* d,
                            void* storage,uint32_t capacity)
{
    if(!s || !d || !storage || !capacity)return false;
    size_t bytes=(size_t)capacity*NATIVE_MATCH_STREAM_RECORD_BYTES;
    if(bytes/NATIVE_MATCH_STREAM_RECORD_BYTES!=capacity)return false;
    if(overlap(s,sizeof(*s),storage,bytes) || overlap(d,sizeof(*d),storage,bytes))return false;
    uint8_t encoded[NATIVE_MATCH_STREAM_DESCRIPTION_BYTES];
    size_t count=nativeMatchStream_EncodeDescription(d,encoded,sizeof(encoded));
    if(!count)return false;
    NativeMatchStream candidate={.description=*d,.records=storage,.capacity=capacity,
        .digest=hash(UINT64_C(14695981039346656037),encoded,count)};
    *s=candidate;return true;
}
static bool advance(NativeMatchStream* s,const uint8_t* record)
{
    uint32_t frame=(uint32_t)get(record,4);
    uint16_t phase=(uint16_t)get(record+4,2);
    uint8_t kind=record[6],continues=record[7];
    if(s->finished || frame!=s->next_frame || phase!=s->phase ||
       kind>NativeMatchStream_PhaseEnd || continues>1)return false;
    if(kind==NativeMatchStream_Frame) {
        if(continues || s->next_frame==UINT32_MAX)return false;
        ++s->next_frame;
    } else {
        // A phase marker has no controller input. The next scene retains its
        // simulation state/RNG and starts its own frame counter at zero.
        for(unsigned i=16;i<NATIVE_MATCH_STREAM_RECORD_BYTES;++i)
            if(record[i])return false;
        if(continues) {
            if(s->phase==UINT16_MAX)return false;
            ++s->phase;s->next_frame=0;
        } else s->finished=true;
    }
    ++s->count;s->digest=hash(s->digest,record,NATIVE_MATCH_STREAM_RECORD_BYTES);
    return true;
}
static uint8_t* next_record(NativeMatchStream* s)
{
    if(!s || !s->records || s->finished || s->unavailable)return NULL;
    if(s->count==s->capacity) {s->unavailable=true;return NULL;}
    return s->records+(size_t)s->count*NATIVE_MATCH_STREAM_RECORD_BYTES;
}
bool nativeMatchStream_AppendFrame(NativeMatchStream* s,uint64_t frame,
                                   const PADStatus pads[4],uint64_t digest)
{
    if(!s || !pads || frame!=s->next_frame || frame>=UINT32_MAX)return false;
    uint8_t* out=next_record(s);if(!out)return false;
    put(out,frame,4);put(out+4,s->phase,2);
    out[6]=NativeMatchStream_Frame;out[7]=0;put(out+8,digest,8);
    for(unsigned p=0;p<4;++p)nativePadWire_Write(out+16+p*NATIVE_PAD_WIRE_BYTES,&pads[p]);
    return advance(s,out);
}
bool nativeMatchStream_EndPhase(NativeMatchStream* s,bool continues,uint64_t digest)
{
    if(!s || (continues && s->phase==UINT16_MAX))return false;
    uint8_t* out=next_record(s);if(!out)return false;
    memset(out,0,NATIVE_MATCH_STREAM_RECORD_BYTES);
    put(out,s->next_frame,4);put(out+4,s->phase,2);
    out[6]=NativeMatchStream_PhaseEnd;out[7]=continues;put(out+8,digest,8);
    return advance(s,out);
}
bool nativeMatchStream_Read(const NativeMatchStream* s,uint32_t index,NativeMatchStreamSample* out)
{
    if(!s || !s->records || !out || index>=s->count ||
       overlap(out,sizeof(*out),s,sizeof(*s)) ||
       overlap(out,sizeof(*out),s->records,(size_t)s->capacity*NATIVE_MATCH_STREAM_RECORD_BYTES))return false;
    const uint8_t* in=s->records+(size_t)index*NATIVE_MATCH_STREAM_RECORD_BYTES;
    memset(out,0,sizeof(*out));out->frame=(uint32_t)get(in,4);
    out->phase=(uint16_t)get(in+4,2);out->kind=in[6];out->continues=in[7]!=0;
    out->state_digest=get(in+8,8);
    for(unsigned p=0;p<4;++p)nativePadWire_Read(&out->pads[p],in+16+p*NATIVE_PAD_WIRE_BYTES);
    return true;
}
size_t nativeMatchStream_EncodeRecords(const NativeMatchStream* s,uint32_t first,
                                      uint8_t* out,size_t capacity)
{
    if(!s || !s->records || s->unavailable || !out || first>=s->count ||
       capacity<NATIVE_MATCH_STREAM_PACKET_HEADER+NATIVE_MATCH_STREAM_RECORD_BYTES ||
       overlap(out,capacity,s,sizeof(*s)) ||
       overlap(out,capacity,s->records,(size_t)s->capacity*NATIVE_MATCH_STREAM_RECORD_BYTES))return 0;
    size_t count=(capacity-NATIVE_MATCH_STREAM_PACKET_HEADER)/NATIVE_MATCH_STREAM_RECORD_BYTES;
    if(count>NATIVE_MATCH_STREAM_PACKET_RECORDS)count=NATIVE_MATCH_STREAM_PACKET_RECORDS;
    if(count>s->count-first)count=s->count-first;
    memset(out,0,NATIVE_MATCH_STREAM_PACKET_HEADER);memcpy(out,"FMWD",4);
    put(out+4,s->description.match.session_id,8);put(out+12,first,4);out[16]=(uint8_t)count;
    size_t bytes=count*NATIVE_MATCH_STREAM_RECORD_BYTES;
    memcpy(out+NATIVE_MATCH_STREAM_PACKET_HEADER,
           s->records+(size_t)first*NATIVE_MATCH_STREAM_RECORD_BYTES,bytes);
    return NATIVE_MATCH_STREAM_PACKET_HEADER+bytes;
}
bool nativeMatchStream_AcceptRecords(NativeMatchStream* s,const uint8_t* in,size_t bytes)
{
    if(!s || !s->records || s->unavailable || !in || bytes<NATIVE_MATCH_STREAM_PACKET_HEADER ||
       overlap(in,bytes,s,sizeof(*s)) ||
       overlap(in,bytes,s->records,(size_t)s->capacity*NATIVE_MATCH_STREAM_RECORD_BYTES) ||
       memcmp(in,"FMWD",4) || get(in+4,8)!=s->description.match.session_id ||
       in[17] || in[18] || in[19])return false;
    uint32_t first=(uint32_t)get(in+12,4),count=in[16];
    if(!count || count>NATIVE_MATCH_STREAM_PACKET_RECORDS ||
       bytes!=NATIVE_MATCH_STREAM_PACKET_HEADER+(size_t)count*NATIVE_MATCH_STREAM_RECORD_BYTES ||
       first>s->count || first>s->capacity || count>s->capacity-first)return false;
    uint32_t duplicate=s->count-first;if(duplicate>count)duplicate=count;
    const uint8_t* records=in+NATIVE_MATCH_STREAM_PACKET_HEADER;
    if(memcmp(s->records+(size_t)first*NATIVE_MATCH_STREAM_RECORD_BYTES,records,
              (size_t)duplicate*NATIVE_MATCH_STREAM_RECORD_BYTES))return false;
    NativeMatchStream candidate=*s;
    for(unsigned i=duplicate;i<count;++i)
        if(!advance(&candidate,records+(size_t)i*NATIVE_MATCH_STREAM_RECORD_BYTES))return false;
    memcpy(s->records+(size_t)s->count*NATIVE_MATCH_STREAM_RECORD_BYTES,
           records+(size_t)duplicate*NATIVE_MATCH_STREAM_RECORD_BYTES,
           (size_t)(count-duplicate)*NATIVE_MATCH_STREAM_RECORD_BYTES);
    *s=candidate;return true;
}
#endif
