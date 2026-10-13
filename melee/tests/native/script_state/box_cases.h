#include <math.h>
#include <melee/ft/ft_081B.h>
static unsigned box_comparisons;
static void seed_box(float facing,bool floor)
{
    setup();script_collision_owned=true;actor.facing_dir=facing;
    actor.coll_data.cur_pos=(Vec3){-13,19,23};
    actor.coll_data.last_pos=(Vec3){29,-31,37};
    actor.coll_data.env_flags=floor?Collide_FloorMask:0;
}
static void box_cases(void)
{
    const uint32_t ids[]={NativeScriptBinding_CollisionGroundBox,NativeScriptBinding_CollisionGroundBoxEdges,NativeScriptBinding_CollisionAirBox};
    const float facings[]={-1,1,0,-0.0f,-2};
    for(unsigned kind=0;kind<3;++kind)for(unsigned f=0;f<5;++f)
    for(unsigned floor=0;floor<2;++floor)for(unsigned shape=0;shape<2;++shape){
        ftCollisionBox box={12+(float)shape,-2,{ -6-(float)shape,4},{8,9+(float)shape}};
        seed_box(facings[f],floor!=0);
        bool expected=kind==0?ft_80082888(&object,&box):kind==1?ft_80082978(&object,&box):ft_800824A0(&object,&box);
        Fighter after=actor;ftCollisionBox mirrored=script_collision_box;unsigned mode=script_collision_mode;
        seed_box(facings[f],floor!=0);
        NativeScriptValue values[]={value_float(box.top),value_float(box.bottom),value_float(box.left.x),value_float(box.left.y),value_float(box.right.x),value_float(box.right.y)},out;
        CHECK(nativeScriptBindings_Invoke(&context,ids[kind],NULL,0,values,6,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&out.as.u64==(uint64_t)expected);
        CHECK(script_collision_calls==1&&script_collision_mode==mode&&mode==kind+1);
        CHECK(!memcmp(&after,&actor,sizeof(actor))&&!memcmp(&mirrored,&script_collision_box,sizeof(mirrored)));
        CHECK(actor.coll_data.last_pos.x==-13&&actor.coll_data.last_pos.y==19&&actor.cur_pos.x==7+(float)(kind+1)*0.125f);
        CHECK(script_collision_box.top==box.top&&script_collision_box.bottom==box.bottom&&script_collision_box.left.y==box.left.y&&script_collision_box.right.y==box.right.y);
        CHECK(script_collision_box.left.x==(facings[f]<0?-box.right.x:box.left.x)&&script_collision_box.right.x==(facings[f]<0?-box.left.x:box.right.x));
        ++box_comparisons;
    }
    for(unsigned kind=0;kind<3;++kind){
        for(unsigned coordinate=0;coordinate<6;++coordinate)for(unsigned bad=0;bad<3;++bad){
            seed_box(1,true);NativeScriptValue values[]={value_float(12),value_float(0),value_float(-6),value_float(6),value_float(6),value_float(6)};
            values[coordinate].as.f32=bad==0?NAN:bad==1?INFINITY:-INFINITY;
            Fighter before=actor;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindings_Invoke(&context,ids[kind],NULL,0,values,6,&out,&diagnostic));
            CHECK(script_collision_calls==0&&!memcmp(&actor,&before,sizeof(actor))&&out.type==NativeScriptType_U64&&out.as.u64==99);
        }
        for(unsigned bad=0;bad<6;++bad){
            seed_box(1,true);NativeScriptValue values[]={value_float(12),value_float(0),value_float(-6),value_float(6),value_float(6),value_float(6)};
            NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
            if(bad==0)++context.generation;
            if(bad==1)++context.scene_epoch;
            if(bad==2)object.classifier=HSD_GOBJ_CLASS_STAGE;
            if(bad==3)context.source_class=NativeCallbackSource_Item;
            unsigned count=bad==4?5:6;
            if(bad==5)values[0]=i32(12);
            Fighter before=actor;
            CHECK(!nativeScriptBindingState_Invoke(&context,ids[kind],values,count,&out,&diagnostic));
            CHECK(script_collision_calls==0&&!memcmp(&actor,&before,sizeof(actor))&&out.type==NativeScriptType_U64&&out.as.u64==99);
        }
        for(unsigned mode=1;mode<=7;++mode){
            seed_box(1,true);script_collision_retire=mode;
            NativeScriptValue values[]={value_float(12),value_float(0),value_float(-6),value_float(6),value_float(6),value_float(6)},out={.type=NativeScriptType_U64,.as.u64=99};
            bool accepted=nativeScriptBindings_Invoke(&context,ids[kind],NULL,0,values,6,&out,&diagnostic);
            if(accepted)fprintf(stderr,"Accepted retired box kind %u mode %u\n",kind,mode);
            CHECK(!accepted);
            CHECK(script_collision_calls==1&&out.type==NativeScriptType_U64&&out.as.u64==99);
        }
    }
}
static void box_frontend_vm(void)
{
    const char* text="func Box():\n    collision.ground_box_edges(12, -2, -6, 4, 8, 9)\n";
    NativeScriptSourceNode syntax[32];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRNode nodes[64];uint32_t args[32];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,64,args,0,32};
    NativeScriptFunctionDeclaration funcs[2];NativeScriptParameterDeclaration params[2];NativeScriptValueDeclaration fields[2];
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=funcs,.function_capacity=2,.parameters=params,.parameter_capacity=2,.values=fields,.value_capacity=2};CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptLocal locals[16];NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=16};
    NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    NativeScriptInstruction code[64];NativeScriptFunctionCode fn[1]={0};
    NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=64,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,funcs[0].source_node,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false};CHECK(nativeScriptBytecode_Prepare(&program,&diagnostic));NativeScriptValue registers[32];NativeScriptVMFrame frames[2];NativeScriptVM vm={.registers=registers,.register_capacity=32,.frames=frames,.frame_capacity=2};
    seed_box(-1,true);NativeScriptValue out;
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&out)==NativeScriptVM_Ok);
    CHECK(script_collision_calls==1&&script_collision_mode==2&&script_collision_box.left.x==-8&&script_collision_box.right.x==6);
    seed_box(1,true);script_collision_retire=1;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&out)==NativeScriptVM_SourceRetired);
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&vm.depth==0&&vm.register_top==0);
    const uint32_t invalid[]={NativeScriptBindings_Article,NativeScriptBindings_Effect};
    for(unsigned n=0;n<2;++n){binding=(NativeScriptBindingResolverContext){invalid[n]};CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));}
}
