#include "../fixture_portrait.h"
#include <melee/gm/native_content_store.h>
#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_source_resources.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/native_fighter_pose.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codec_dependencies.inc"
static unsigned checks,cases;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"packet %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t dat[32],record[65536];static size_t record_bytes;static NativeSourceAssetFacts facts;
static const char source[]="fighter:\n    behavior independent\naction Main:\n    on load:\n        pass\n";
static void fixture(void)
{
 strcpy(facts.fighter_root,"ftDataFixture");strcpy(facts.costume_joint,"fixture_joint");facts.actions=facts.demo_count=1;facts.joints=7;facts.effect_bank=UINT8_MAX;
 facts.moves.common_count=facts.moves.first=ftCo_MS_Count;
 const unsigned roles[]={FtPart_TopN,FtPart_TransN,FtPart_XRotN,FtPart_YRotN,FtPart_HipN,FtPart_TransN2,FT_COMMON_PART_COUNT-1};
 for(unsigned i=0;i<7;++i){bool seen=false;for(unsigned j=0;j<facts.bone_count;++j)if((unsigned)facts.bones[j].part==roles[i])seen=true;if(!seen)facts.bones[facts.bone_count++]=(NativeFighterBone){(Fighter_Part)roles[i],(uint8_t)i};}
 NativeScriptDiagnostic diagnostic;CHECK(nativeSourceAssets_Write(&facts,record,sizeof(record),&record_bytes,&diagnostic));
}
static void reject(const void* packet,size_t bytes)
{
 NativeFighterPackage metadata,before;NativeFighterPackageData data,prior;memset(&metadata,0xA5,sizeof(metadata));memset(&data,0xA5,sizeof(data));before=metadata;prior=data;
 CHECK(!nativeContentStore_ParseFighter(packet,bytes,&metadata,&data));CHECK(!memcmp(&metadata,&before,sizeof(metadata))&&!memcmp(&data,&prior,sizeof(data)));
}
static void check_compressed(const NativeFighterPackage* metadata,
                             const NativeFighterPackageData* fixture_data,
                             size_t stored_size)
{
 const size_t animation_size = NATIVE_FIGHTER_MAX_EXPANDED_BYTES - stored_size + sizeof(dat);
 uint8_t* animation = malloc(animation_size + 1);
 CHECK(animation);
 for (size_t i = 0; i <= animation_size; ++i) animation[i] = (uint8_t)(i * 7);
 NativeFighterPackageData data = *fixture_data;
 data.members[0].animation_dat = animation;
 data.members[0].animation_size = animation_size;
 uint8_t* packet = NULL;
 size_t size = 0;
 NativeContentId id;
 CHECK(nativeContentStore_BuildFighter(metadata, &data, &packet, &size, &id));
 CHECK(packet[4] == 1 && size < NATIVE_CONTENT_MAX_PACKAGE_BYTES);
 NativeContentPreview preview = {0};
 nativeContentStore_UpdatePreview(packet, 208 + sizeof(fixture_portrait), &preview);
 CHECK(preview.portrait_size == sizeof(fixture_portrait) && !memcmp(preview.portrait_png, fixture_portrait, sizeof(fixture_portrait)));
 NativeFighterPackage parsed;
 NativeFighterPackageData spans;
 CHECK(nativeContentStore_ParseFighter(packet, size, &parsed, NULL));
 CHECK(nativeContentStore_ParseFighter(packet, size, &parsed, &spans));
 CHECK(spans.members[0].animation_size == animation_size);
 CHECK(!memcmp(spans.members[0].animation_dat, animation, animation_size));
 CHECK(!memcmp(spans.members[0].script_text, source, sizeof(source) - 1));
 nativeContentStore_ReleaseFighterData(&spans);
 CHECK(!spans.expanded_payload);

 reject(packet, size - 1);
 reject(packet, NATIVE_CONTENT_MAX_PACKAGE_BYTES + 1);
 uint8_t* bad = malloc(size + 1);
 CHECK(bad);
 memcpy(bad, packet, size);
 bad[size] = 0;
 reject(bad, size + 1);
 const unsigned animation_length_offset = 116;
 for (unsigned i = 0; i < 4; ++i)
     bad[animation_length_offset + i] = (uint8_t)((animation_size + 1) >> (8 * i));
 reject(bad, size);
 for (unsigned i = 0; i < 4; ++i)
     bad[animation_length_offset + i] = (uint8_t)((animation_size - 1) >> (8 * i));
 reject(bad, size);
 free(bad);
 free(packet);
 packet = NULL;
 ++data.members[0].animation_size;
 CHECK(!nativeContentStore_BuildFighter(metadata, &data, &packet, &size, &id));
 CHECK(!packet);
 free(animation);
 ++cases;
}
int main(void)
{
 fixture();NativeFighterPackage metadata={.mode=NativeFighterFamily_Single,.member_count=1};strcpy(metadata.title,"Source fixture");strcpy(metadata.members[0].key,"main");
 NativeFighterPackageData data={.portrait_png=fixture_portrait,.portrait_size=sizeof(fixture_portrait),.members={{.fighter_dat=dat,.fighter_size=32,.animation_dat=dat,.animation_size=32,.costume_dat=dat,.costume_size=32,.asset_record=record,.asset_record_size=record_bytes,.script_text=(const uint8_t*)source,.script_size=sizeof(source)-1}}};
 uint8_t* packet=NULL;size_t bytes=0;NativeContentId id;CHECK(nativeContentStore_BuildFighter(&metadata,&data,&packet,&bytes,&id));CHECK(bytes==208+sizeof(fixture_portrait)+96+record_bytes+sizeof(source)-1);
 CHECK(!memcmp(packet,"FMSC",4)&&packet[4]==0&&packet[5]==0&&packet[6]==1);CHECK(!memcmp(packet+80,"main",5));for(unsigned i=144;i<208;++i)CHECK(!packet[i]);
 NativeFighterPackage parsed;NativeFighterPackageData spans;CHECK(nativeContentStore_ParseFighter(packet,bytes,&parsed,&spans));CHECK(!strcmp(parsed.members[0].key,"main")&&spans.members[0].asset_record_size==record_bytes&&spans.members[0].script_size==sizeof(source)-1);
 CHECK(!memcmp(spans.members[0].asset_record,record,record_bytes)&&!memcmp(spans.members[0].script_text,source,sizeof(source)-1));++cases;
 check_compressed(&metadata, &data, bytes);
 for(size_t truncated=0;truncated<bytes;++truncated)reject(packet,truncated);++cases;
 uint8_t* bad=malloc(bytes+1);CHECK(bad);memcpy(bad,packet,bytes);bad[bytes]=0;reject(bad,bytes+1);++cases;
 const unsigned offsets[]={0,4,5,6,7,12,79,84,144};for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);++i){memcpy(bad,packet,bytes);bad[offsets[i]]=255;reject(bad,bytes);}++cases;
 memcpy(bad,packet,bytes);memcpy(bad,"FMCO",4);bad[4]=3;reject(bad,bytes);++cases;
 memcpy(bad,packet,bytes);memcpy(bad+(spans.members[0].asset_record-packet),"MZxx",4);reject(bad,bytes);++cases;
 memcpy(bad,packet,bytes);bad[spans.members[0].script_text-packet]=0;reject(bad,bytes);++cases;
 uint8_t* rejected=NULL;size_t ignored=0;metadata.mode=NativeFighterFamily_AlternateForm;metadata.member_count=2;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&rejected,&ignored,&id));metadata.mode=NativeFighterFamily_SimultaneousCompanion;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&rejected,&ignored,&id));++cases;
 metadata.mode=NativeFighterFamily_Single;metadata.member_count=1;data.members[0].script_size=0;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&rejected,&ignored,&id));data.members[0].script_size=sizeof(source)-1;
 data.members[0].asset_record=(const uint8_t*)"MZ fake native DLL";data.members[0].asset_record_size=18;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&rejected,&ignored,&id));++cases;
 free(bad);free(packet);
 data.members[0].asset_record=record;data.members[0].asset_record_size=record_bytes;
 const uint8_t payload[]={1,2,3,4};uint8_t bundle[512];size_t bundle_size=0;
 NativeSourceResourceInput resource={"items/test.blob",payload,sizeof(payload),NativeSourceResource_Blob};
 CHECK(nativeSourceResources_Write(&resource,1,bundle,sizeof(bundle),&bundle_size,NULL,0));
 data.members[0].resources=bundle;data.members[0].resource_size=bundle_size;
 CHECK(nativeContentStore_BuildFighter(&metadata,&data,&packet,&bytes,&id));
 CHECK(nativeContentStore_ParseFighter(packet,bytes,&parsed,&spans));
 CHECK(spans.members[0].resource_size==bundle_size&&!memcmp(spans.members[0].resources,bundle,bundle_size));
 NativeSourceResources view;CHECK(nativeSourceResources_Read(spans.members[0].resources,bundle_size,&view,NULL,0));
 CHECK(nativeSourceResources_Find(&view,"items/test.blob",NativeSourceResource_Blob)->size==4);++cases;
 bad=malloc(bytes);CHECK(bad);memcpy(bad,packet,bytes);bad[spans.members[0].resources-packet]=0;reject(bad,bytes);++cases;
 free(bad);free(packet);
 const char partner_source[]="fighter:\n    behavior independent\nstate int companion_value\naction Main:\n    on load:\n        state.companion_value = 17\n";
 strcpy(metadata.members[0].key,"caster");strcpy(metadata.members[1].key,"counterpart");
 metadata.member_count=2;data.members[1]=data.members[0];
 data.members[1].script_text=(const uint8_t*)partner_source;data.members[1].script_size=sizeof(partner_source)-1;
 data.members[1].resources=NULL;data.members[1].resource_size=0;
 for(unsigned mode=NativeFighterFamily_AlternateForm;mode<=NativeFighterFamily_SimultaneousCompanion;++mode) {
  metadata.mode=(NativeFighterFamilyMode)mode;
  CHECK(nativeContentStore_BuildFighter(&metadata,&data,&packet,&bytes,&id));
  CHECK(packet[5]==mode&&packet[6]==2);
  CHECK(nativeContentStore_ParseFighter(packet,bytes,&parsed,&spans));
  CHECK(parsed.mode==metadata.mode&&parsed.member_count==2);
  CHECK(!strcmp(parsed.members[0].key,"caster")&&!strcmp(parsed.members[1].key,"counterpart"));
  CHECK(spans.members[0].script_size==sizeof(source)-1&&spans.members[1].script_size==sizeof(partner_source)-1);
  CHECK(!memcmp(spans.members[0].script_text,source,sizeof(source)-1)&&!memcmp(spans.members[1].script_text,partner_source,sizeof(partner_source)-1));
  CHECK(spans.members[0].resource_size==bundle_size&&spans.members[1].resources==NULL);
  CHECK(spans.members[1].asset_record_size==record_bytes&&!memcmp(spans.members[1].asset_record,record,record_bytes));
  ++cases;
  for(size_t cut=0;cut<bytes;++cut)reject(packet,cut);++cases;
  bad=malloc(bytes);CHECK(bad);
  memcpy(bad,packet,bytes);memcpy(bad+144,bad+80,32);reject(bad,bytes);++cases;
  memcpy(bad,packet,bytes);bad[spans.members[1].asset_record-packet]=0;reject(bad,bytes);++cases;
  memcpy(bad,packet,bytes);bad[spans.members[1].script_text-packet]=0;reject(bad,bytes);++cases;
  memcpy(bad,packet,bytes);bad[6]=1;reject(bad,bytes);++cases;
  NativeContentId before=id;size_t untouched=123;uint8_t* pointer=(uint8_t*)(uintptr_t)0x1234;
  metadata.member_count=1;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&pointer,&untouched,&id));
  CHECK(pointer==(uint8_t*)(uintptr_t)0x1234&&untouched==123&&!memcmp(&id,&before,sizeof(id)));metadata.member_count=2;++cases;
  free(bad);free(packet);
 }
 metadata.mode=NativeFighterFamily_Single;CHECK(!nativeContentStore_BuildFighter(&metadata,&data,&rejected,&ignored,&id));++cases;
 printf("{\"status\":\"PASS source family packet\",\"cases\":%u,\"checks\":%u}\n",cases,checks);return 0;
}
