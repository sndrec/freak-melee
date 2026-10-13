#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/robj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const HSD_Joint* joints[140];
static const HSD_Joint* parents[140];
static unsigned count;
static void visit(const HSD_Joint* joint,const HSD_Joint* parent)
{
    for(;joint;joint=joint->next) {
        if(count==140)abort();joints[count]=joint;parents[count++]=parent;
        if(!(joint->flags&JOBJ_INSTANCE))visit(joint->child,joint);
    }
}
int main(int argc,char** argv)
{
    if(argc!=2)return 2;
    FILE* file=fopen(argv[1],"rb");if(!file)return 3;
    fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);
    void* bytes=malloc((size_t)size);void* arena=calloc(1,FT_COSTUME_IMPORT_ARENA_SIZE);
    if(!bytes||!arena||fread(bytes,1,(size_t)size,file)!=(size_t)size)return 4;fclose(file);
    HSD_Dat dat;if(HSD_DatOpen(&dat,bytes,(size_t)size)!=HSD_DAT_OK)return 5;
    const char* root=NULL;
    for(unsigned i=0;i<dat.root_count;++i){const char* name=NULL;u32 offset;if(!HSD_DatSymbol(&dat,false,i,&offset,&name))return 5;if(name&&strstr(name,"_joint")){root=name;break;}}
    FtCostumeAssets assets;
    if(!root||!ftCostumeAssets_Import(bytes,(size_t)size,root,NULL,arena,FT_COSTUME_IMPORT_ARENA_SIZE,FT_COSTUME_IMPORT_RECORD_CAPACITY,&assets))return 6;
    visit(assets.joint,NULL);
    printf("{\"root\":\"%s\",\"joints\":%u,\"queries\":[",root,count);
    for(unsigned query=0;query<2;++query){unsigned index=query?47:29;printf("%s{\"part\":%u,\"ancestors\":[",query?",":"",index);
        if(index>=count)return 7;const HSD_Joint* joint=joints[index];unsigned depth=0;
        while(joint){unsigned i;for(i=0;i<count&&joints[i]!=joint;++i){}if(i==count)return 8;
            unsigned types[5]={0};for(const HSD_RObjDesc* r=joint->robjdesc;r;r=r->next){unsigned t=(r->flags&ROBJ_TYPE_MASK)>>28;if(t>4)return 9;++types[t];}
            printf("%s{\"index\":%u,\"flags\":%u,\"robj_types\":[%u,%u,%u,%u,%u]}",depth++?",":"",i,(unsigned)joint->flags,types[0],types[1],types[2],types[3],types[4]);joint=parents[i];
        }printf("]}");
    }puts("]}");free(arena);free(bytes);return 0;
}
