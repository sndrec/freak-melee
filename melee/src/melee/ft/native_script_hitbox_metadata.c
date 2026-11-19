#include "native_script_hitbox_metadata.h"
#include "native_script_compile.h"
#include "native_script_constants.h"
#include "ftparts.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan at,const char* text)
{if(d){d->source=at;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
static bool equal(const NativeScriptSourceTree* tree,NativeScriptSourceSpan a,NativeScriptSourceSpan b)
{return a.length==b.length&&!memcmp(tree->source+a.offset,tree->source+b.offset,a.length);}
static bool identifier(const NativeScriptSourceTree* tree,NativeScriptSourceSpan name)
{
    if(!name.length||name.length>63||name.offset>tree->source_bytes||name.length>tree->source_bytes-name.offset)return false;
    for(uint32_t i=0;i<name.length;++i){unsigned char c=tree->source[name.offset+i];
        if(!(c=='_'||(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(i&&c>='0'&&c<='9')))return false;}
    return true;
}
bool nativeScriptHitboxMetadata_Owns(const NativeScriptSourceTree* tree,uint32_t at)
{
    if(!tree||!tree->nodes||at>=tree->count)return false;
    const NativeScriptSourceNode* n=&tree->nodes[at];
    if(n->kind!=NativeScriptSource_Metadata)return false;
    return nativeScriptSource_SpanEquals(tree,n->name,"hitbox_definition")||
        (n->parent<tree->count&&tree->nodes[n->parent].kind==NativeScriptSource_Metadata&&
         nativeScriptSource_SpanEquals(tree,tree->nodes[n->parent].name,"hitbox_definition"));
}

typedef struct Field {const char* name;uint16_t offset;uint8_t type;int32_t min,max;} Field;
#define INT_FIELD(name,low,high) {#name,offsetof(NativeScriptHitboxDefinition,name),NativeScriptType_I32,low,high}
#define FLOAT_FIELD(name) {#name,offsetof(NativeScriptHitboxDefinition,name),NativeScriptType_F32,0,0}
#define BOOL_FIELD(name) {#name,offsetof(NativeScriptHitboxDefinition,name),NativeScriptType_Bool,0,0}
static const Field fields[]={
    INT_FIELD(bone,0,INT32_MAX),FLOAT_FIELD(damage),INT_FIELD(angle,0,INT32_MAX),
    INT_FIELD(growth,0,INT32_MAX),INT_FIELD(weight_knockback,0,INT32_MAX),INT_FIELD(base_knockback,0,INT32_MAX),
    INT_FIELD(group,0,INT32_MAX),INT_FIELD(element,0,31),INT_FIELD(shield_damage,INT32_MIN,INT32_MAX),
    INT_FIELD(sound_severity,0,7),INT_FIELD(sound_kind,0,31),FLOAT_FIELD(radius),
    FLOAT_FIELD(offset_x),FLOAT_FIELD(offset_y),FLOAT_FIELD(offset_z),BOOL_FIELD(item_interaction),
    BOOL_FIELD(ignore_fighter_scale),BOOL_FIELD(clank),BOOL_FIELD(rebound),BOOL_FIELD(grounded),
    BOOL_FIELD(aerial),BOOL_FIELD(victim_only),BOOL_FIELD(require_throw_owner),
    BOOL_FIELD(exclude_grabbed),
    FLOAT_FIELD(end_x), FLOAT_FIELD(end_y), FLOAT_FIELD(end_z),
};
#undef INT_FIELD
#undef FLOAT_FIELD
#undef BOOL_FIELD

bool nativeScriptHitboxMetadata_Prepare(NativeScriptCompileBank* bank,NativeScriptDiagnostic* d)
{
    if(!bank)return error(d,(NativeScriptSourceSpan){0},"Hitbox definitions require a compile bank");
    bank->hitbox_definition_count=0;
    const NativeScriptSourceTree* tree=&bank->tree;
    for(uint32_t at=1;at<tree->count;++at){const NativeScriptSourceNode* n=&tree->nodes[at];
        if(n->kind!=NativeScriptSource_Metadata||!nativeScriptSource_SpanEquals(tree,n->name,"hitbox_definition"))continue;
        if(n->parent||!n->opens||!identifier(tree,n->value))
            return error(d,n->source,"Hitbox definition needs a root 'hitbox_definition Name:' block");
        if(bank->hitbox_definition_count==NATIVE_SCRIPT_HITBOX_DEFINITIONS)
            return error(d,n->source,"Hitbox definition capacity exceeded");
        NativeScriptHitboxDefinition value={.name=n->value,.source=n->source,.node=at,
            .angle=361,.growth=100,.clank=true,.grounded=true,.aerial=true};
        for(uint32_t i=0;i<bank->hitbox_definition_count;++i)
            if(equal(tree,value.name,bank->hitbox_definitions[i].name))return error(d,value.name,"Duplicate hitbox definition");
        uint32_t seen=0;
        for(uint32_t child=n->first_child,bound=0;child!=UINT32_MAX;child=tree->nodes[child].next){
            if(child>=tree->count||bound++==tree->count)return error(d,n->source,"Hitbox field child chain is invalid");
            const NativeScriptSourceNode* field=&tree->nodes[child];
            if(field->parent!=at||field->kind!=NativeScriptSource_Metadata||field->opens||!field->value.length)
                return error(d,field->source,"Hitbox field needs one constant expression and no nested block");
            unsigned id;
            for(id=0;id<sizeof(fields)/sizeof(*fields);++id)
                if(nativeScriptSource_SpanEquals(tree,field->name,fields[id].name))break;
            if(id==sizeof(fields)/sizeof(*fields))return error(d,field->source,"Unknown hitbox definition field");
            if(seen&(1u<<id))return error(d,field->source,"Duplicate hitbox definition field");
            const Field* spec=&fields[id];NativeScriptValue scalar;
            NativeScriptIRType type=spec->type==NativeScriptType_I32?NativeScriptType_F64:(NativeScriptIRType)spec->type;
            if(!nativeScriptProgram_EvaluateConstant(&bank->declarations,field->value,type,&scalar,d))return false;
            void* output=(uint8_t*)&value+spec->offset;
            if(spec->type==NativeScriptType_I32){
                if(!isfinite(scalar.as.f64)||scalar.as.f64<spec->min||scalar.as.f64>spec->max||floor(scalar.as.f64)!=scalar.as.f64)
                    return error(d,field->source,"Hitbox integer field must be exact and inside its engine range");
                *(int32_t*)output=(int32_t)scalar.as.f64;
            }else if(spec->type==NativeScriptType_F32){
                if(!isfinite(scalar.as.f32))return error(d,field->source,"Hitbox floating-point fields must be finite");
                *(float*)output=scalar.as.f32;
            }else *(bool*)output=scalar.as.u64!=0;
            seen|=1u<<id;
        }
        const uint32_t required=(1u<<0)|(1u<<1)|(1u<<11);
        if((seen&required)!=required||value.radius<0)
            return error(d,n->source,"Hitbox definition requires bone, damage and nonnegative radius");
        if (value.damage < 0 || (double) value.damage > INT32_MAX)
            return error(d,n->source,"Hitbox damage must be nonnegative and within its engine range");
        if (value.victim_only && value.exclude_grabbed)
            return error(d,n->source,"Choose either victim_only or exclude_grabbed for this hitbox");
        const uint32_t end_fields = 7u << (sizeof(fields) / sizeof(*fields) - 3);
        value.capsule = (seen & end_fields) != 0;
        if (value.capsule && (seen & end_fields) != end_fields)
            return error(d,n->source,"A capsule needs end_x, end_y and end_z");
        bank->hitbox_definitions[bank->hitbox_definition_count++]=value;
    }
    return true;
}
bool nativeScriptHitboxMetadata_Validate(const NativeScriptCompileBank* bank,
    const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    if(!bank||tree!=&bank->tree||at>=tree->count||bank->hitbox_definition_count>NATIVE_SCRIPT_HITBOX_DEFINITIONS)
        return error(d,(NativeScriptSourceSpan){0},"Hitbox metadata requires its exact prepared bank");
    for(uint32_t i=0;i<bank->hitbox_definition_count;++i){uint32_t root=bank->hitbox_definitions[i].node;
        if(at==root||(tree->nodes[at].parent==root&&nativeScriptHitboxMetadata_Owns(tree,at)))return true;}
    return error(d,tree->nodes[at].source,"Hitbox metadata is outside its prepared definition");
}
bool nativeScriptHitboxMetadata_CheckSymbols(const NativeScriptCompileBank* bank,NativeScriptDiagnostic* d)
{
    const NativeScriptSourceTree* tree=&bank->tree;
    for(uint32_t i=0;i<bank->hitbox_definition_count;++i){NativeScriptSourceSpan name=bank->hitbox_definitions[i].name;
        for(uint32_t node=1;node<tree->count;++node){const NativeScriptSourceNode* n=&tree->nodes[node];
            bool symbol=n->kind==NativeScriptSource_Function||n->kind==NativeScriptSource_State||n->kind==NativeScriptSource_Parameter||
                n->kind==NativeScriptSource_Attribute||n->kind==NativeScriptSource_Table||n->kind==NativeScriptSource_Article||
                n->kind==NativeScriptSource_Item||n->kind==NativeScriptSource_Effect;
            if(symbol&&equal(tree,name,n->name))return error(d,name,"Hitbox definition conflicts with another source symbol");
        }
        for(uint32_t p=0;p<bank->pose_symbol_count;++p)
            if(equal(tree,name,bank->pose_symbols[p].name))return error(d,name,"Hitbox definition conflicts with a pose graph");
        for(uint32_t b=1;b<NativeScriptBinding_Count;++b){const NativeScriptBindingDescriptor* spec=nativeScriptBindings_Descriptor(b);
            if(spec&&!spec->parent&&nativeScriptSource_SpanEquals(tree,name,spec->name))return error(d,name,"Hitbox definition conflicts with a native namespace");}
        NativeScriptIRNode n={.kind=NativeScriptIR_Name,.source=name,.text_offset=name.offset,.text_length=name.length};
        NativeScriptIRResolution result;int constant=nativeScriptConstants_TryResolve(NULL,&bank->expressions,&n,&result,d);
        if(constant<0)return false;if(constant)return error(d,name,"Hitbox definition conflicts with a shared constant");
        const char* reserved[]={"state","params","attrs","program","true","false","None","and","or","not"};
        for(unsigned r=0;r<sizeof(reserved)/sizeof(*reserved);++r)
            if(nativeScriptSource_SpanEquals(tree,name,reserved[r]))return error(d,name,"Hitbox definition uses a reserved source name");
    }
    return true;
}
int nativeScriptHitboxMetadata_TryResolve(const NativeScriptCompileBank* bank,uint32_t context,
    uint32_t owner,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{
    if(!bank||!arena||!node||!result||node->kind!=NativeScriptIR_Name)return 0;
    if(node->text_offset>arena->source_bytes||node->text_length>arena->source_bytes-node->text_offset||
       bank->hitbox_definition_count>NATIVE_SCRIPT_HITBOX_DEFINITIONS){error(d,node->source,"Invalid hitbox symbol span/table");return -1;}
    for(uint32_t i=0;i<bank->hitbox_definition_count;++i){NativeScriptSourceSpan name=bank->hitbox_definitions[i].name;
        if(name.length!=node->text_length||memcmp(bank->tree.source+name.offset,arena->source+node->text_offset,name.length))continue;
        if(owner||context!=NativeScriptBindings_Fighter){error(d,node->source,"Hitbox definition requires its root Fighter ownership");return -1;}
        *result=(NativeScriptIRResolution){.type=NativeScriptType_HitboxDefinition,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
            .flags=NativeScriptFlag_Literal,.literal_bits=i};return 1;
    }
    return 0;
}
