#include <melee/ft/native_script_binding_attachments.h>
#include <melee/ft/native_fighter_attachment.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/lb/lb_00F9.h>

static FtCostumeAssets attachment_costume;
static UnkCostumeStruct attachment_selected;
struct UnkCostumeList CostumeListsForeachCharacter[Ft_Kind_RuntimeCapacity];
static bool attachment_skin;
static HSD_Joint attachment_descriptor;
static HSD_JObj attachment_joints[3];
static HSD_DObj attachment_draws[2];
static FighterBone attachment_parts[8];
static FighterPartsTable* attachment_part_tables[Ft_Kind_RuntimeCapacity];
static uint8_t attachment_arena[256];
static uint8_t attachment_roots[8]={0,0,0,16,0,0,0,0};
static unsigned attachment_imports,attachment_loads,attachment_frees,attachment_allocs,attachment_chain_frees;
static unsigned attachment_visibility_calls,attachment_retire;
static Fighter* attachment_fighter;
static struct {
    FtPartsDesc descriptor;
    void* table[5][4];
    FtPartsVisLookup rows[1];
    TempS variants[1];
    uint8_t indices[2];
} attachment_visibility;

const UnkCostumeStruct* nativeCostumeRuntime_ForFighter(const Fighter* fp)
{(void)fp;return attachment_skin?&attachment_selected:NULL;}
void* HSD_DatImportGraph(HSD_DatImport* import,u32 offset,HSD_DatImportType type)
{
    CHECK(import==&attachment_costume.import && offset==16 && type==DAT_JOINT);
    ++attachment_imports;import->arena_used+=8;++import->record_count;
    return &attachment_descriptor;
}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{
    if(accessory_case)return accessory_load(descriptor);
    CHECK(descriptor==&attachment_descriptor);++attachment_loads;
    memset(attachment_joints,0,sizeof(attachment_joints));
    attachment_joints[0].child=&attachment_joints[1];
    attachment_joints[1].parent=&attachment_joints[0];
    attachment_joints[1].next=&attachment_joints[2];
    attachment_joints[2].parent=&attachment_joints[0];
    if(attachment_retire)++attachment_fighter->x8_spawnNum;
    return attachment_joints;
}
void HSD_JObjRemoveAll(HSD_JObj* root)
{if(accessory_case){accessory_remove(root);return;}if(root==attachment_joints)++attachment_frees;}
void ftPartsPObjSetDefaultClass(void){}
void ftPartsPObjClearDefaultClass(void){}
void ftParts_80075650(Fighter_GObj* object,HSD_JObj* joint,DObjList* draws)
{
    CHECK(object->user_data==attachment_fighter && joint==attachment_joints);
    draws->data[0]=&attachment_draws[0];draws->data[1]=&attachment_draws[1];
}
void ftParts_8007487C(FtPartsDesc* desc,FtPartsVis* vis,u32 costume,DObjList* first,DObjList* second)
{
    CHECK(desc && costume<desc->costume_count && first==second);
    vis->model_num=desc->model_num;++attachment_visibility_calls;
}
void ftParts_80074CA0(FtPartsVis* vis,int group,DObjList* draws)
{(void)vis;(void)group;(void)draws;abort();}
void ftParts_80074D7C(FtPartsVis* vis,int group,DObjList* draws)
{(void)vis;(void)group;(void)draws;abort();}
void lb_8000FD48(HSD_JObj* joint,DynamicsDesc* desc,size_t count)
{
    CHECK(joint==&attachment_joints[1]||joint==&attachment_joints[2]);
    memset(desc,0,sizeof(*desc));desc->count=count;++attachment_allocs;
}
void lb_80011710(DynamicsDesc* source,DynamicsDesc* target)
{target->pos=source->pos;}
void lb_8000FD18(DynamicsDesc* desc)
{CHECK(desc->count);++attachment_chain_frees;}
// This lifecycle fixture does not execute the rendered attachment pass.
void HSD_JObjSetupMatrixSub(HSD_JObj* joint){(void)joint;abort();}
void C_MTXCopy(Mtx source,Mtx destination)
{(void)source;(void)destination;abort();}
