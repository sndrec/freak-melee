// Full native OnLoad/LoadSpecialAttrs decisions. Asset registration is checked
// against immutable article kind metadata; actual graph creation has separate
// backend evidence. Clip decoding and insertion are explicit shared boundaries.
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/lb/lbanim.h>
static unsigned load_cases,registered_kinds[6],registered_count;
static unsigned char attribute_sources[2][FT_CHARACTER_EXT_ATTR_CAPACITY];
static unsigned char attribute_backups[2][FT_CHARACTER_EXT_ATTR_CAPACITY];
static Article* load_assets[7];
static Article load_asset_tokens[7];
static struct Fighter_804D6540_t omission_record;
static struct Fighter_804D6540_x0_t omission_value;
static struct Fighter_804D6540_t* omission_records[Ft_Kind_RuntimeCapacity];
struct Fighter_804D6540_t** Fighter_804D6540=omission_records;
FigaTree* ftData_80085E50(Fighter* f,enum_t index)
{CHECK(f==&fighters[current]&&index==72);event(47,index,0);return &clip_token;}
float lbAnim_8001E8F8(FigaTree* clip){CHECK(clip==&clip_token);event(48,float_bits(clip_end),0);return clip_end;}
void it_8026B3F8(Article* a,s32 kind)
{CHECK(current==0&&registered_count<6&&a==load_assets[registered_count]);registered_kinds[registered_count++]=kind;}
void ftParts_800753D4(Fighter* f,struct Fighter_804D6540_x0_t* omission,HSD_Joint* graph)
{CHECK(f==&fighters[current]&&omission==&omission_value&&graph==(HSD_Joint*)load_assets[6]);event(49,6,0);}
static bool load_graph(Fighter_GObj* g,int graph,int omission)
{CHECK(g==&creators[1]&&graph==6&&omission==0);ftParts_800753D4(&fighters[1],&omission_value,(HSD_Joint*)load_assets[6]);return true;}
#include "load_originals.h"
static void load_init(NativeFighterHost* host)
{host->animation_clip=ftData_80085E50;host->clip_length=lbAnim_8001E8F8;host->copy_ext_attributes=copy_ext_attributes;host->reload_ext_attributes=reload_ext_attributes;host->install_model_graph=load_graph;}
static void load_matrix(void)
{
 for(unsigned i=0;i<7;i++)load_assets[i]=&load_asset_tokens[i];
 float scales[]={.5f,1,1.25f,2};float ends[]={1,43.25f,67};
 for(unsigned scale=0;scale<4;scale++)for(unsigned end=0;end<3;end++){
  common_reset();registered_count=0;clip_end=ends[end];memset(attribute_sources,0,sizeof(attribute_sources));
  for(unsigned side=0;side<2;side++){
   Fighter* f=&fighters[side];f->x34_scale.y=scales[scale];f->dat_attrs_backup=attribute_backups[side];fdata[side].ext_attr=attribute_sources[side];fdata[side].x48_items=(void**)load_assets;
   ftLk_DatAttrs* a=(void*)attribute_sources[side];a->specialhi_pos_y_offset=parameter_float("boomerang_vertical_offset");a->attackairlw_hit_anim_frame_end=-77;
   a->x48=family==Ft_Kind_Link?It_Kind_Link_Bomb:It_Kind_CLink_Bomb;a->x2C=family==Ft_Kind_Link?It_Kind_Link_Boomerang:It_Kind_CLink_Boomerang;
   a->xBC=family==Ft_Kind_Link?It_Kind_Link_HShot:It_Kind_CLink_HShot;a->xC=family==Ft_Kind_Link?It_Kind_Link_Arrow:It_Kind_CLink_Arrow;a->x10=family==Ft_Kind_Link?It_Kind_Link_Bow:It_Kind_CLink_Bow;
  }
  omission_record.x0=&omission_value;omission_records[family]=&omission_record;
  current=0;if(family==Ft_Kind_Link)ftLk_Init_OnLoad(&creators[0]);else ftCl_Init_OnLoad(&creators[0]);current=1;definition->on_load(&creators[1]);phase_compare();
  CHECK(((ftLk_DatAttrs*)fighters[0].dat_attrs)->attackairlw_hit_anim_frame_end==timing(1));CHECK(fighters[0].can_walljump==fighters[1].can_walljump);
  CHECK(registered_count==(family==Ft_Kind_Link?5:6));
  for(unsigned i=0;i<registered_count;i++){bool found=false;for(unsigned j=0;j<definition->article_logic_count;j++)if(definition->article_logics[j].kind==registered_kinds[i])found=true;CHECK(found);}
  CHECK(definition->article_logic_count==registered_count);
  memset(trace_count,0,sizeof(trace_count));current=0;ftLk_Init_LoadSpecialAttrs(&creators[0]);current=1;definition->on_load_special_attrs(&creators[1]);phase_compare();
  float offset;int32_t b=get(definition,&fighters[1],"boomerang_vertical_offset");memcpy(&offset,&b,4);
  CHECK(((ftLk_DatAttrs*)fighters[0].dat_attrs)->specialhi_pos_y_offset==offset);
  // Owned immutable FMP value is unchanged by per-instance source-attr scale.
  CHECK(parameter_float("boomerang_vertical_offset")==((ftLk_DatAttrs*)attribute_sources[1])->specialhi_pos_y_offset);
  ++load_cases;
 }
 clip_end=43.25f;no_leaks();fprintf(stderr,"load cases: %u\n",load_cases);
}
