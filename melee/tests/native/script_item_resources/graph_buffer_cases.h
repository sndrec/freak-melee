// Actual TObj/PObj constructors, frees and allocator snapshots. No draw/GPU.
static unsigned graph_buffer_checks;
static size_t graph_buffer_class_bytes;
static unsigned graph_buffer_reserved_checks;

static HSD_PObj* buffer_shape(unsigned count)
{
    HSD_ShapeSetDesc shape={.flags=SHAPESET_ADDITIVE,.nb_shape=(u16)count};
    HSD_PObjDesc polygon={.flags=POBJ_SHAPEANIM,.u.shape_set=&shape};
    HSD_PObj* result=HSD_PObjLoadDesc(&polygon);CHECK(result&&result->u.shape_set);
    CHECK(result->u.shape_set->nb_shape==count);
    for(unsigned i=0;i<count;++i)CHECK(result->u.shape_set->blend.bp[i]==0);
    return result;
}

static void graph_buffer_snapshots(void)
{
    unsigned start=checks;
    // Both mutable variable-size satellites must be captured by their graph's
    // ordinary class backing, including padding/free links and exact addresses.
    HSD_PObj* polygon=buffer_shape(7);
    for(unsigned i=0;i<7;++i)polygon->u.shape_set->blend.bp[i]=(float)i*.125f;
    HSD_PObj polygon_before=*polygon;HSD_ShapeSet shape_before=*polygon->u.shape_set;
    float weights_before[7];memcpy(weights_before,shape_before.blend.bp,sizeof(weights_before));
    unsigned char colors[2][8]={{1,2,3,4},{5,6,7,8}};
    HSD_TlutDesc palettes[2]={ {.lut=colors[0],.fmt=GX_TL_RGB565,.n_entries=4},
                             {.lut=colors[1],.fmt=GX_TL_RGB5A3,.n_entries=4} };
    HSD_TlutDesc* table[2]={&palettes[0],&palettes[1]};
    HSD_TObjDesc descriptor={.id=GX_TEXMAP0,.scale={1,1,1}};
    HSD_TObj* texture=HSD_TObjLoadDesc(&descriptor);CHECK(texture);
    HSD_TexAnim animation={.id=GX_TEXMAP0,.tluttbl=table,.n_tluttbl=2};
    HSD_TObjAddAnim(texture,&animation);CHECK(texture->tluttbl&&texture->tluttbl[0]&&texture->tluttbl[1]&&!texture->tluttbl[2]);
    HSD_TObj texture_before=*texture;HSD_Tlut* table_before[3];
    memcpy(table_before,texture->tluttbl,sizeof(table_before));
    HSD_Tlut palettes_before[2]={*table_before[0],*table_before[1]};
    size_t cb=nativeClass_SnapshotBytes(),cw=0,again=0;CHECK(cb);
    uint8_t* saved=malloc(cb),*after=malloc(cb);CHECK(saved&&after);
    CHECK(nativeClass_Save(saved,cb,&cw)&&cw==cb);
    OSHeapHandle heap=HSD_GetHeap();u32 hb=OSHeapSnapshotBytes(heap),hw=0;CHECK(hb);
    uint8_t* heap_saved=malloc(hb),*heap_after=malloc(hb);CHECK(heap_saved&&heap_after);
    CHECK(OSSaveHeapSnapshot(heap,heap_saved,hb,&hw)&&hw==hb);
    // Change payloads, remove their owners, and reuse class-piece addresses.
    polygon->u.shape_set->blend.bp[3]=-.875f;
    texture->tluttbl[0]->fmt=GX_TL_IA8;
    texture->tluttbl[0]=table_before[1];texture->tluttbl[1]=table_before[0];
    HSD_PObjRemove(polygon);HSD_TObjRemove(texture);
    HSD_PObj* replacement=buffer_shape(7);
    void* next_weights=replacement->u.shape_set->blend.bp;
    replacement->u.shape_set->blend.bp[4]=.75f;
    HSD_TObj* next_texture=HSD_TObjLoadDesc(&descriptor);CHECK(next_texture);
    HSD_TObjAddAnim(next_texture,&animation);void* next_table=next_texture->tluttbl;
    CHECK(nativeClass_ValidateSnapshot(saved,cw)&&OSValidateHeapSnapshot(heap,heap_saved,hw));
    CHECK(OSLoadHeapSnapshot(heap,heap_saved,hw));CHECK(nativeClass_Load(saved,cw));
    CHECK(!memcmp(polygon,&polygon_before,sizeof(polygon_before)));
    CHECK(!memcmp(polygon->u.shape_set,&shape_before,sizeof(shape_before)));
    CHECK(!memcmp(polygon->u.shape_set->blend.bp,weights_before,sizeof(weights_before)));
    CHECK(!memcmp(texture,&texture_before,sizeof(texture_before)));
    CHECK(!memcmp(texture->tluttbl,table_before,sizeof(table_before)));
    for(unsigned i=0;i<2;++i)CHECK(!memcmp(texture->tluttbl[i],&palettes_before[i],sizeof(HSD_Tlut)));
    CHECK(nativeClass_Save(after,cb,&again)&&again==cw&&!memcmp(saved,after,cw));
    u32 h_again=0;CHECK(OSSaveHeapSnapshot(heap,heap_after,hb,&h_again)&&h_again==hw&&!memcmp(heap_saved,heap_after,hw));
    // Repeat the same destruction and allocation; ordered free links repeat.
    texture->tluttbl[0]=table_before[1];texture->tluttbl[1]=table_before[0];
    HSD_PObjRemove(polygon);HSD_TObjRemove(texture);
    replacement=buffer_shape(7);CHECK(replacement->u.shape_set->blend.bp==next_weights);
    next_texture=HSD_TObjLoadDesc(&descriptor);CHECK(next_texture);
    HSD_TObjAddAnim(next_texture,&animation);CHECK(next_texture->tluttbl==next_table);
    // Replacement by a differently sized table and then no table exercises
    // each original native destruction path with the correct piece size.
    animation.n_tluttbl=1;HSD_TObjAddAnim(next_texture,&animation);
    CHECK(next_texture->tluttbl[0]&&!next_texture->tluttbl[1]);
    animation.n_tluttbl=0;HSD_TObjAddAnim(next_texture,&animation);CHECK(!next_texture->tluttbl);
    HSD_TObjRemove(next_texture);HSD_PObjRemove(replacement);
    free(heap_after);free(heap_saved);free(after);free(saved);
    HSD_PObj* empty=buffer_shape(0);CHECK(!empty->u.shape_set->blend.bp);HSD_PObjRemove(empty);
    graph_buffer_class_bytes=cb;graph_buffer_checks=checks-start;++cases;
}

static void graph_buffer_reserved(void)
{
    unsigned start=checks;
    static NativeHsdReservation owner;
    static unsigned char backing[32768] ATTRIBUTE_ALIGN(64);
    NativeHsdDemand demand[5]={0};unsigned count=0;
    const uint32_t pieces[]={sizeof(HSD_PObj),sizeof(HSD_ShapeSet),7*sizeof(float),
                            sizeof(HSD_TObj),3*sizeof(HSD_Tlut*),sizeof(HSD_Tlut),sizeof(HSD_Tlut)};
    for(unsigned i=0;i<sizeof(pieces)/sizeof(pieces[0]);++i){
        uint32_t size=(pieces[i]+31)&~31u;unsigned n=0;
        for(;n<count&&demand[n].size!=size;++n){}
        if(n==count){CHECK(count<5);demand[count++]=(NativeHsdDemand){.size=size};}
        ++demand[n].count;
    }
    CHECK(nativeHsdReservation_Bind(&owner,backing,sizeof(backing),demand,count,0x4752415048));
    CHECK(nativeHsdReservation_Begin(&owner,demand,count));
    HSD_PObj* polygon=buffer_shape(7);
    HSD_TObjDesc descriptor={.id=GX_TEXMAP0,.scale={1,1,1}};
    HSD_TObj* texture=HSD_TObjLoadDesc(&descriptor);CHECK(texture);
    unsigned char colors[8]={1,2,3,4};
    HSD_TlutDesc palette={.lut=colors,.fmt=GX_TL_RGB565,.n_entries=4};
    HSD_TlutDesc* table[2]={&palette,&palette};
    HSD_TexAnim animation={.id=GX_TEXMAP0,.n_tluttbl=2,.tluttbl=table};
    HSD_TObjAddAnim(texture,&animation);CHECK(nativeHsdReservation_End(&owner));
    polygon->u.shape_set->blend.bp[5]=.625f;
    void* weights=polygon->u.shape_set->blend.bp;void* palette_table=texture->tluttbl;
    size_t rb=nativeHsdReservation_SnapshotBytes(),rw=0,cb=nativeClass_SnapshotBytes(),cw=0,again=0;
    CHECK(rb&&cb);uint8_t* records=malloc(rb),*after=malloc(rb),*classes=malloc(cb),*class_after=malloc(cb);
    CHECK(records&&after&&classes&&class_after);
    CHECK(nativeHsdReservation_Save(records,rb,&rw)&&rw==rb);
    CHECK(nativeClass_Save(classes,cb,&cw)&&cw==cb);
    HSD_PObjRemove(polygon);HSD_TObjRemove(texture);
    CHECK(nativeHsdReservation_Begin(&owner,demand,count));
    HSD_PObj* future=buffer_shape(7);HSD_TObj* future_texture=HSD_TObjLoadDesc(&descriptor);CHECK(future_texture);
    HSD_TObjAddAnim(future_texture,&animation);CHECK(nativeHsdReservation_End(&owner));
    CHECK(nativeClass_ValidateSnapshot(classes,cw)&&nativeHsdReservation_ValidateSnapshot(records,rw));
    CHECK(nativeClass_Load(classes,cw)&&nativeHsdReservation_Load(records,rw));
    CHECK(polygon->u.shape_set->blend.bp==weights&&polygon->u.shape_set->blend.bp[5]==.625f);
    CHECK(texture->tluttbl==palette_table&&texture->tluttbl[0]->lut==colors&&!texture->tluttbl[2]);
    CHECK(nativeHsdReservation_Save(after,rb,&again)&&again==rw&&!memcmp(records,after,rw));
    CHECK(nativeClass_Save(class_after,cb,&again)&&again==cw&&!memcmp(classes,class_after,cw));
    HSD_PObjRemove(polygon);HSD_TObjRemove(texture);CHECK(nativeHsdReservation_Release(&owner));
    free(class_after);free(classes);free(after);free(records);
    graph_buffer_reserved_checks=checks-start;++cases;
}
