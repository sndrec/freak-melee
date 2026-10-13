// Constant palette selections use the native float-to-index truncation rule.
// Malformed descriptors must leave the transition census output untouched.
static unsigned palette_census_cases, palette_comparisons, palette_refusals;

static void palette_census(void)
{
    unsigned char colors[8]={0};
    HSD_TlutDesc palette={.lut=colors,.fmt=GX_TL_RGB565,.n_entries=4};
    HSD_TlutDesc* palettes[256];
    for(unsigned n=0;n<256;++n)palettes[n]=&palette;
    uint8_t stream[6]={HSD_A_OP_CON,0,0,0,0,1};
    HSD_FObjDesc track={.length=sizeof(stream),.type=HSD_A_T_TCLT,.ad=stream};
    HSD_AObjDesc clock={.end_frame=60,.fobjdesc=&track};
    HSD_TexAnim animation={.id=GX_TEXMAP0,.aobjdesc=&clock,.tluttbl=palettes,.n_tluttbl=2};
    HSD_TObjDesc texture={.id=GX_TEXMAP0};
    HSD_MObjDesc material={.texdesc=&texture};
    HSD_DObjDesc display={.mobjdesc=&material};
    HSD_Joint joint={.u.dobjdesc=&display};
    HSD_MatAnim material_animation={.texanim=&animation};
    HSD_MatAnimJoint material_tree={.matanim=&material_animation};
    const float values[]={0,1,1.99f,-0.5f,-1,2,NAN,INFINITY,255};
    for(unsigned n=0;n<sizeof(values)/sizeof(values[0]);++n){
        memcpy(stream+1,&values[n],sizeof(float));
        NativeArticleAnimationDemand demand={0};
        CHECK(nativeArticleAnimation_Census(&joint,NULL,&material_tree,NULL,&demand));
        CHECK(demand.controllers==1&&demand.tracks==1);
        CHECK(demand.uncovered==(n<4?0:NativeArticleAnimation_Channel));
        ++palette_census_cases;
    }
    float value=254;memcpy(stream+1,&value,sizeof(value));
    animation.n_tluttbl=255;
    NativeArticleAnimationDemand demand={0};
    CHECK(nativeArticleAnimation_Census(&joint,NULL,&material_tree,NULL,&demand)&&!demand.uncovered);
    ++palette_census_cases;
    animation.n_tluttbl=256;
    CHECK(nativeArticleAnimation_Census(&joint,NULL,&material_tree,NULL,&demand));
    CHECK(demand.uncovered==(NativeArticleAnimation_PaletteTables|NativeArticleAnimation_Channel));
    ++palette_census_cases;
    animation.n_tluttbl=2;
    value=0;memcpy(stream+1,&value,sizeof(value));
    for(unsigned n=0;n<5;++n){
        HSD_TlutDesc saved=palette;
        HSD_TlutDesc** saved_table=animation.tluttbl;
        if(n==0)animation.tluttbl=NULL;
        if(n==1)palettes[1]=NULL;
        if(n==2)palette.lut=NULL;
        if(n==3)palette.n_entries=0;
        if(n==4)palette.fmt=(GXTlutFmt)3;
        NativeArticleAnimationDemand before={17,19,23};demand=before;
        CHECK(!nativeArticleAnimation_Census(&joint,NULL,&material_tree,NULL,&demand));
        CHECK(!memcmp(&before,&demand,sizeof(before)));
        animation.tluttbl=saved_table;palettes[1]=&palette;palette=saved;
        ++palette_census_cases;
    }
    stream[0]=HSD_A_OP_LIN;
    CHECK(nativeArticleAnimation_Census(&joint,NULL,&material_tree,NULL,&demand));
    CHECK(demand.uncovered==NativeArticleAnimation_Channel);
    ++palette_census_cases;
}

static void same_palette_clocks(HSD_AObj* actual,HSD_AObj* expected)
{
    CHECK(!!actual==!!expected);
    if(!actual)return;
    HSD_AObj a=*actual,b=*expected;a.fobj=b.fobj=NULL;a.hsd_obj=b.hsd_obj=NULL;
    CHECK(!memcmp(&a,&b,sizeof(a)));
    HSD_FObj* x=actual->fobj;HSD_FObj* y=expected->fobj;
    for(;x&&y;x=x->next,y=y->next){HSD_FObj a=*x,b=*y;a.next=b.next=NULL;CHECK(!memcmp(&a,&b,sizeof(a)));}
    CHECK(!x&&!y);
}

static void same_material_animation(HSD_JObj* actual,HSD_JObj* expected,unsigned depth)
{
    CHECK(depth<64);
    for(;actual&&expected;actual=actual->next,expected=expected->next){
        HSD_DObj* x=actual->u.dobj;HSD_DObj* y=expected->u.dobj;
        for(;x&&y;x=x->next,y=y->next){
            HSD_MObj* a=x->mobj;HSD_MObj* b=y->mobj;CHECK(!!a==!!b);
            if(!a)continue;
            same_palette_clocks(a->aobj,b->aobj);
            CHECK(!memcmp(a->mat,b->mat,sizeof(*a->mat)));
            HSD_TObj* ta=a->tobj;HSD_TObj* tb=b->tobj;
            for(;ta&&tb;ta=ta->next,tb=tb->next){
                same_palette_clocks(ta->aobj,tb->aobj);
                CHECK(ta->tlut_no==tb->tlut_no&&ta->imagetbl==tb->imagetbl&&ta->imagedesc==tb->imagedesc);
                CHECK(!memcmp(&ta->rotate,&tb->rotate,sizeof(ta->rotate)));
                CHECK(!memcmp(&ta->translate,&tb->translate,sizeof(ta->translate)));
                CHECK(!memcmp(&ta->scale,&tb->scale,sizeof(ta->scale)));
                CHECK(ta->blending==tb->blending&&!!ta->tluttbl==!!tb->tluttbl);
                if(ta->tluttbl){
                    unsigned n=0;
                    for(;n<255&&ta->tluttbl[n]&&tb->tluttbl[n];++n){
                        HSD_Tlut* pa=ta->tluttbl[n];HSD_Tlut* pb=tb->tluttbl[n];
                        CHECK(pa->lut==pb->lut&&pa->fmt==pb->fmt&&pa->n_entries==pb->n_entries);
                    }
                    CHECK(!ta->tluttbl[n]&&!tb->tluttbl[n]);++palette_comparisons;
                }
            }
            CHECK(!ta&&!tb);
        }
        CHECK(!x&&!y);
        if(actual->child&&!(actual->flags&JOBJ_INSTANCE))same_material_animation(actual->child,expected->child,depth+1);
    }
    CHECK(!actual&&!expected);
}

static HSD_TObj* first_palette(HSD_JObj* model)
{
    for(;model;model=model->next){
        for(HSD_DObj* display=model->u.dobj;display;display=display->next)
            if(display->mobj)for(HSD_TObj* texture=display->mobj->tobj;texture;texture=texture->next)
                if(texture->tluttbl)return texture;
        if(model->child&&!(model->flags&JOBJ_INSTANCE)){
            HSD_TObj* texture=first_palette(model->child);if(texture)return texture;
        }
    }
    return NULL;
}

static void refuse_palette_mutations(Item_GObj* owner,unsigned index,HSD_JObj* model)
{
    HSD_TObj* texture=first_palette(model);if(!texture)return;
    const uint8_t child[]={0};
    HSD_Tlut** table=texture->tluttbl;
    HSD_Tlut saved=*table[0];unsigned count=0;
    while(count<255&&table[count])++count;
    CHECK(count>0&&count<255);
    for(unsigned n=0;n<6;++n){
        uint8_t saved_selection=texture->tlut_no;
        if(n==0)table[0]->lut=NULL;
        if(n==1)table[0]->fmt=(GXTlutFmt)3;
        if(n==2)table[0]->n_entries=0;
        if(n==3)texture->tluttbl=NULL;
        if(n==4)texture->tlut_no=(uint8_t)count;
        if(n==5)table[count]=table[0];
        CHECK(!nativeArticleResource_RequestAnimation(owner,index,child,1,0));
        texture->tluttbl=table;*table[0]=saved;texture->tlut_no=saved_selection;table[count]=NULL;
        ++palette_refusals;
    }
    CHECK(nativeArticleResource_RequestAnimation(owner,index,child,1,0));
}
