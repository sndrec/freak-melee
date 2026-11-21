#ifndef HSD_NATIVE_CONSTRAINT_H
#define HSD_NATIVE_CONSTRAINT_H

// HSD model expression instructions, in their on-disc order.
typedef enum HSD_ConstraintOp {
    Cns_Nop, Cns_Return, Cns_Argument, Cns_JumpIf, Cns_Jump, Cns_Drop,
    Cns_Literal, Cns_ToInt, Cns_ToFloat, Cns_NegFloat, Cns_NegInt,
    Cns_RandomBit, Cns_RandomFloat, Cns_Sin, Cns_Cos, Cns_Tan, Cns_Asin,
    Cns_Acos, Cns_Atan, Cns_Log, Cns_Exp, Cns_AbsFloat, Cns_Sqrt,
    Cns_AddFloat, Cns_SubFloat, Cns_MulFloat, Cns_DivFloat, Cns_ModFloat,
    Cns_AddInt, Cns_SubInt, Cns_MulInt, Cns_DivInt, Cns_ModInt, Cns_Pow,
    Cns_MinFloat, Cns_MaxFloat, Cns_MinInt, Cns_MaxInt, Cns_Atan2,
    Cns_RandomInt, Cns_AbsInt, Cns_LtInt, Cns_GtInt, Cns_LeInt,
    Cns_GeInt, Cns_EqInt, Cns_NeInt, Cns_And, Cns_Or, Cns_Not, Cns_Xor,
    Cns_LtFloat, Cns_GtFloat, Cns_LeFloat, Cns_GeFloat, Cns_EqFloat,
    Cns_NeFloat, Cns_BitAnd, Cns_BitOr, Cns_BitXor, Cns_Copy,
} HSD_ConstraintOp;

enum {
    HSD_CONSTRAINT_STACK = 64,
    HSD_CONSTRAINT_INSTRUCTIONS = 1024,
    HSD_CONSTRAINT_ARGUMENTS = 100,
    HSD_CONSTRAINT_EXPRESSIONS_PER_MODEL = 64,
};

#endif
