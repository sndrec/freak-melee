#include "vm_runtime.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static EvalContext context;
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static EvalValue native_call(unsigned signature, uintptr_t callee, const EvalValue* args)
{
    CHECK(signature == 0);
    EvalValue result = { 0 };
    result.f = ((float (*)(float))callee)(args[0].f);
    return result;
}

static void numeric(void)
{
    // Exactly the mixed Float/Double expression used in the authored follower:
    // its unsuffixed coefficients are Double; assignment rounds to Float.
    static const EvalInstruction code[] = {
        {Eval_Convert,Eval_F64,2,0,0,0,Eval_F32},
        {Eval_Convert,Eval_F64,3,1,0,0,Eval_F32},
        {Eval_Constant,Eval_F64,4,0}, {Eval_Constant,Eval_F64,5,1},
        {Eval_Mul,Eval_F64,6,2,4}, {Eval_Mul,Eval_F64,7,3,5},
        {Eval_Add,Eval_F64,8,6,7},
        {Eval_Convert,Eval_F32,9,8,0,0,Eval_F64},
        {Eval_Return,Eval_F32,0,9}
    };
    static const EvalFunction functions[] = { {0,9,10,0,2,Eval_F32,"mixed"} };
    static const EvalValue constants[] = { {.d=0.95}, {.d=0.05} };
    const EvalProgram program = {code,functions,constants,NULL,9,2,1};
    uint32_t random = 0x984238;
    for (unsigned i = 0; i < 4096; ++i) {
        random = random * 1664525u + 1013904223u;
        float a = (int32_t)random / 4194304.0f;
        random = random * 1664525u + 1013904223u;
        float b = (int32_t)random / 4194304.0f;
        EvalValue args[] = {{.f=a},{.f=b}};
        float expected = (float)(0.95 * a + 0.05 * b);
        EvalValue actual = evalVm_Run(&context,&program,0,args,2);
        CHECK(memcmp(&actual.f,&expected,4) == 0 && context.depth == 0 && !context.errors);
    }
    static const EvalInstruction integer_code[] = {
        {Eval_Convert,Eval_I32,2,0,0,0,Eval_U32},
        {Eval_Shr,Eval_I32,3,2,1}, {Eval_Return,Eval_I32,0,3}
    };
    static const EvalFunction integers[] = {{0,3,4,0,2,Eval_I32,"signed-width"}};
    const EvalProgram integer_program = {integer_code,integers,NULL,NULL,3,0,1};
    for (unsigned bits = 1; bits < 32; ++bits) {
        EvalValue args[] = {{.u=0xFFFFFFFFu},{.u=bits}};
        EvalValue actual = evalVm_Run(&context,&integer_program,0,args,2);
        CHECK(actual.i == ((int32_t)0xFFFFFFFFu >> bits) && !context.errors);
    }
}

static void memory(void)
{
    struct { unsigned char low:3, high:5; } flags = {2,17};
    static const EvalInstruction code[] = {
        {Eval_StoreBits,Eval_U8,0,0,1,0,((uint64_t)3 << 32)},
        {Eval_LoadBits,Eval_U8,2,0,0,0,((uint64_t)5 << 32)|3},
        {Eval_Return,Eval_U8,0,2}
    };
    static const EvalFunction functions[] = {{0,3,3,0,2,Eval_U8,"bits"}};
    const EvalProgram program = {code,functions,NULL,NULL,3,0,1};
    for (unsigned i = 0; i < 256; ++i) {
        EvalValue args[] = {{.p=(uintptr_t)&flags},{.u=i}};
        EvalValue result = evalVm_Run(&context,&program,0,args,2);
        CHECK(flags.low == (i&7) && flags.high == 17 && result.u == 17 && !context.errors);
    }
    static const EvalInstruction locals[] = {
        {Eval_LocalAddress,Eval_Ptr,1,0,0,0,16},
        {Eval_Clear,Eval_Void,0,1,0,0,16},
        {Eval_Store,Eval_F32,0,1,0},
        {Eval_Load,Eval_F32,2,1},
        {Eval_Return,Eval_F32,0,2}
    };
    static const EvalFunction local_fn[] = {{0,5,3,32,1,Eval_F32,"local"}};
    const EvalProgram local_program = {locals,local_fn,NULL,NULL,5,0,1};
    EvalValue value = {.f=-13.25f};
    CHECK(evalVm_Run(&context,&local_program,0,&value,1).f == value.f);
}

static void calls_and_loop(void)
{
    static const EvalInstruction code[] = {
        {Eval_Call,Eval_U32,2,1,0,1}, {Eval_Return,Eval_U32,0,2},
        // Sum 1..argument. Every loop reads/stores actual instruction registers.
        {Eval_Const,Eval_U32,1,0,0,0,0}, {Eval_Const,Eval_U32,2,0,0,0,1},
        {Eval_Gt,Eval_U32,3,0,1}, {Eval_JumpFalse,Eval_Bool,0,9,3},
        {Eval_Add,Eval_U32,1,1,2}, {Eval_Add,Eval_U32,4,4,1},
        {Eval_Jump,Eval_Void,0,2}, {Eval_Return,Eval_U32,0,4}
    };
    // The accumulator is explicitly initialized by a third entry instruction,
    // not by relying on recycled VM-register memory.
    EvalInstruction patched[11];
    memcpy(patched,code,2*sizeof(*code));
    patched[2]=(EvalInstruction){Eval_Const,Eval_U32,4,0,0,0,0};
    memcpy(patched+3,code+2,8*sizeof(*code));
    patched[6].a=8; patched[9].a=3;
    static const EvalFunction functions[] = {
        {0,2,3,0,1,Eval_U32,"caller"}, {2,9,5,0,1,Eval_U32,"loop"}
    };
    const EvalProgram program = {patched,functions,NULL,NULL,11,0,2};
    for (unsigned n=0;n<128;++n) {
        EvalValue value={.u=n};
        EvalValue actual=evalVm_Run(&context,&program,0,&value,1);
        CHECK(actual.u == n*(n+1)/2 && context.depth == 0 && !context.errors);
    }
    static const EvalInstruction call_code[] = {
        {Eval_Constant,Eval_Ptr,1,0},
        {Eval_CallNative,Eval_F32,2,1,0,1,0}, {Eval_Return,Eval_F32,0,2}
    };
    static const EvalFunction call_fn[]={{0,3,3,0,1,Eval_F32,"native-sqrt"}};
    EvalValue constants[]={{.p=(uintptr_t)sqrtf}};
    const EvalProgram native_program={call_code,call_fn,constants,native_call,3,1,1};
    EvalValue value={.f=17};
    CHECK(evalVm_Run(&context,&native_program,0,&value,1).f==sqrtf(17));
}

int main(void)
{
    numeric(); memory(); calls_and_loop();
    CHECK(context.depth==0 && !context.errors && context.native_calls==1 && context.interpreted_calls==128);
    printf("{\"checks\":%u,\"entries\":%llu,\"errors\":%llu}\n",checks,
        (unsigned long long)context.entries,(unsigned long long)context.errors);
    return 0;
}
