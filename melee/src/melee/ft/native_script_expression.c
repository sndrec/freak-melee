#include "native_script_expression.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ExpressionLexer {
    const char* source;
    uint32_t cursor, end, line, column;
    NativeScriptDiagnostic* diagnostic;
    bool failed;
} ExpressionLexer;

typedef struct ExpressionParser {
    ExpressionLexer lexer;
    NativeScriptExpressionToken token;
    NativeScriptIRArena* arena;
    NativeScriptIRResolver resolve;
    void* context;
    unsigned depth;
} ExpressionParser;

static bool fail(NativeScriptDiagnostic* diagnostic,
                 NativeScriptSourceSpan source, const char* message)
{
    if (diagnostic) {
        diagnostic->source = source;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    }
    return false;
}

static bool ascii_name_start(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool ascii_digit(unsigned char c) { return c >= '0' && c <= '9'; }
static bool ascii_name(unsigned char c)
{
    return ascii_name_start(c) || ascii_digit(c);
}

static int digit_value(unsigned char c)
{
    if (ascii_digit(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void advance(ExpressionLexer* lexer)
{
    if (lexer->source[lexer->cursor++] == '\n') {
        ++lexer->line;
        lexer->column = 1;
    } else ++lexer->column;
}

static char peek(const ExpressionLexer* lexer, unsigned distance)
{
    return distance < lexer->end - lexer->cursor
               ? lexer->source[lexer->cursor + distance] : '\0';
}

static bool lexical_fail(ExpressionLexer* lexer,
                         NativeScriptExpressionToken* token, const char* message)
{
    lexer->failed = true;
    token->source.length = lexer->cursor - token->source.offset;
    return fail(lexer->diagnostic, token->source, message);
}

static bool token_word(ExpressionLexer* lexer,
                       NativeScriptExpressionToken* token, const char* word)
{
    size_t length = strlen(word);
    return token->text_length == length &&
           !memcmp(lexer->source + token->text_offset, word, length);
}

static bool number_token(ExpressionLexer* lexer,
                         NativeScriptExpressionToken* token)
{
    uint32_t start = lexer->cursor;
    bool hex = peek(lexer, 0) == '0' &&
               (peek(lexer, 1) == 'x' || peek(lexer, 1) == 'X');
    bool floating = false;
    unsigned digits = 0;
    if (hex) { advance(lexer); advance(lexer); }
    while (hex ? digit_value((unsigned char)peek(lexer, 0)) >= 0
               : ascii_digit((unsigned char)peek(lexer, 0))) {
        ++digits; advance(lexer);
    }
    if (peek(lexer, 0) == '.') {
        floating = true; advance(lexer);
        while (hex ? digit_value((unsigned char)peek(lexer, 0)) >= 0
                   : ascii_digit((unsigned char)peek(lexer, 0))) {
            ++digits; advance(lexer);
        }
    }
    char exponent = peek(lexer, 0);
    bool has_exponent = hex ? exponent == 'p' || exponent == 'P'
                            : exponent == 'e' || exponent == 'E';
    if (has_exponent) {
        floating = true; advance(lexer);
        if (peek(lexer, 0) == '+' || peek(lexer, 0) == '-') advance(lexer);
        unsigned exponent_digits = 0;
        while (ascii_digit((unsigned char)peek(lexer, 0))) {
            ++exponent_digits; advance(lexer);
        }
        if (!exponent_digits) return lexical_fail(lexer, token, "Missing numeric exponent digits.");
    }
    if (!digits || (hex && floating && !has_exponent))
        return lexical_fail(lexer, token, "Invalid numeric literal.");
    if (ascii_name((unsigned char)peek(lexer, 0)) || peek(lexer, 0) == '.')
        return lexical_fail(lexer, token, "Invalid numeric suffix.");
    token->text_length = lexer->cursor - start;
    token->source.length = token->text_length;
    if (floating) {
        char text[128];
        if (token->text_length >= sizeof(text))
            return lexical_fail(lexer, token, "Numeric literal is too long.");
        memcpy(text, lexer->source + start, token->text_length);
        text[token->text_length] = '\0';
        char* after;
        double value = strtod(text, &after);
        if (*after || !isfinite(value))
            return lexical_fail(lexer, token, "Numeric literal must be finite and representable.");
        token->kind = NativeScriptToken_Floating;
        token->type = NativeScriptType_F64;
        memcpy(&token->bits, &value, sizeof(value));
    } else {
        unsigned base = hex ? 16 : token->text_length > 1 && lexer->source[start] == '0' ? 8 : 10;
        uint64_t value = 0;
        for (uint32_t at = start + (hex ? 2 : 0); at < lexer->cursor; ++at) {
            unsigned digit = (unsigned)digit_value((unsigned char)lexer->source[at]);
            if (digit >= base || value > (UINT64_MAX - digit) / base)
                return lexical_fail(lexer, token, "Integer literal is invalid or outside 64 bits.");
            value = value * base + digit;
        }
        token->kind = NativeScriptToken_Integer;
        token->bits = value;
        token->type = value <= INT32_MAX ? NativeScriptType_I32 :
            base != 10 && value <= UINT32_MAX ? NativeScriptType_U32 :
            value <= INT64_MAX ? NativeScriptType_I64 : NativeScriptType_U64;
        if (base == 10 && value > INT64_MAX)
            return lexical_fail(lexer, token, "Decimal integer literal is outside the signed 64-bit range.");
    }
    return true;
}

static bool lex(ExpressionLexer* lexer, NativeScriptExpressionToken* token)
{
    memset(token, 0, sizeof(*token));
    if (lexer->failed) return false;
    for (;;) {
        char c = peek(lexer, 0);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { advance(lexer); continue; }
        if (c == '#') {
            while (lexer->cursor < lexer->end && peek(lexer, 0) != '\n') advance(lexer);
            continue;
        }
        break;
    }
    token->source = (NativeScriptSourceSpan) {
        lexer->cursor, 0, lexer->line, lexer->column,
    };
    token->text_offset = lexer->cursor;
    char c = peek(lexer, 0);
    if (lexer->cursor == lexer->end) { token->kind = NativeScriptToken_End; return true; }
    if (c == '\0') return lexical_fail(lexer, token, "Embedded zero byte in expression.");
    if (ascii_digit((unsigned char)c) || (c == '.' && ascii_digit((unsigned char)peek(lexer, 1))))
        return number_token(lexer, token);
    if (ascii_name_start((unsigned char)c)) {
        do { advance(lexer); } while (ascii_name((unsigned char)peek(lexer, 0)));
        token->text_length = lexer->cursor - token->text_offset;
        token->kind = NativeScriptToken_Name;
        if (token_word(lexer, token, "and")) {
            token->kind = NativeScriptToken_Operator; token->operation = NativeScriptOp_And;
        } else if (token_word(lexer, token, "or")) {
            token->kind = NativeScriptToken_Operator; token->operation = NativeScriptOp_Or;
        } else if (token_word(lexer, token, "not")) {
            token->kind = NativeScriptToken_Operator; token->operation = NativeScriptOp_Not;
        }
    } else if (c == '"') {
        token->kind = NativeScriptToken_String; advance(lexer);
        token->text_offset = lexer->cursor;
        bool closed = false;
        while (lexer->cursor < lexer->end) {
            c = peek(lexer, 0);
            if (c == '"') { token->text_length = lexer->cursor - token->text_offset; advance(lexer); closed = true; break; }
            if ((unsigned char)c < 32) return lexical_fail(lexer, token, "Control byte or newline inside string literal.");
            if (c == '\\') {
                advance(lexer);
                c = peek(lexer, 0);
                if (!c || !strchr("\\\"nrtbfva0", c))
                    return lexical_fail(lexer, token, "Unsupported string escape.");
            }
            advance(lexer);
        }
        if (!closed) return lexical_fail(lexer, token, "Unterminated string literal.");
        token->type = NativeScriptType_Text;
    } else {
        advance(lexer);
        switch (c) {
        case '.': token->kind = NativeScriptToken_Dot; break;
        case '(': token->kind = NativeScriptToken_OpenParen; break;
        case ')': token->kind = NativeScriptToken_CloseParen; break;
        case '[': token->kind = NativeScriptToken_OpenIndex; break;
        case ']': token->kind = NativeScriptToken_CloseIndex; break;
        case ',': token->kind = NativeScriptToken_Comma; break;
        case ':': token->kind = NativeScriptToken_Colon; break;
        default: {
            token->kind = NativeScriptToken_Operator;
            char next = peek(lexer, 0);
            switch (c) {
            case '+': token->operation = NativeScriptOp_Add; break;
            case '-': token->operation = NativeScriptOp_Subtract; break;
            case '*': token->operation = NativeScriptOp_Multiply; break;
            case '/': token->operation = NativeScriptOp_Divide; break;
            case '%': token->operation = NativeScriptOp_Remainder; break;
            case '~': token->operation = NativeScriptOp_Invert; break;
            case '^': token->operation = NativeScriptOp_BitXor; break;
            case '&': token->operation = next == '&' ? NativeScriptOp_And : NativeScriptOp_BitAnd; if (next == '&') advance(lexer); break;
            case '|': token->operation = next == '|' ? NativeScriptOp_Or : NativeScriptOp_BitOr; if (next == '|') advance(lexer); break;
            case '!': token->operation = next == '=' ? NativeScriptOp_NotEqual : NativeScriptOp_Not; if (next == '=') advance(lexer); break;
            case '=': if (next == '=') { token->operation = NativeScriptOp_Equal; advance(lexer); } else token->kind = NativeScriptToken_Assign; break;
            case '<': token->operation = next == '=' ? NativeScriptOp_LessEqual : next == '<' ? NativeScriptOp_ShiftLeft : NativeScriptOp_Less; if (next == '=' || next == '<') advance(lexer); break;
            case '>': token->operation = next == '=' ? NativeScriptOp_GreaterEqual : next == '>' ? NativeScriptOp_ShiftRight : NativeScriptOp_Greater; if (next == '=' || next == '>') advance(lexer); break;
            default: return lexical_fail(lexer, token, "Unsupported character in expression.");
            }
        } break;
        }
        token->text_length = lexer->cursor - token->text_offset;
    }
    token->source.length = lexer->cursor - token->source.offset;
    return true;
}

static bool source_valid(const char* source, uint32_t source_bytes,
                         NativeScriptSourceSpan span,
                         NativeScriptDiagnostic* diagnostic)
{
    if (!source || span.offset > source_bytes || span.length > source_bytes - span.offset)
        return fail(diagnostic, span, "Expression source range is invalid.");
    return true;
}

bool nativeScriptExpression_Tokenize(const char* source, uint32_t source_bytes,
    NativeScriptSourceSpan span, NativeScriptExpressionToken* tokens,
    uint32_t capacity, uint32_t* count, NativeScriptDiagnostic* diagnostic)
{
    if (count) *count = 0;
    if (diagnostic) memset(diagnostic, 0, sizeof(*diagnostic));
    if (!count || !tokens || !source_valid(source, source_bytes, span, diagnostic)) return false;
    ExpressionLexer lexer = { source, span.offset, span.offset + span.length,
        span.line, span.column, diagnostic, false };
    uint32_t used = 0;
    do {
        if (used == capacity) return fail(diagnostic, span, "Expression token capacity exhausted.");
        if (!lex(&lexer, &tokens[used])) return false;
    } while (tokens[used++].kind != NativeScriptToken_End);
    *count = used;
    return true;
}

bool nativeScriptExpression_IsIntegral(NativeScriptIRType type)
{
    return type >= NativeScriptType_Bool && type <= NativeScriptType_U64;
}

bool nativeScriptExpression_IsNumeric(NativeScriptIRType type)
{
    return type >= NativeScriptType_Bool && type <= NativeScriptType_F64;
}

const char* nativeScriptExpression_TypeName(NativeScriptIRType type)
{
    static const char* names[] = { "invalid", "void", "bool", "int", "uint",
        "int64", "uint64", "float", "double", "text", "None", "namespace",
        "object", "callable", "pose destination", "pose graph" };
    return (unsigned)type < sizeof(names) / sizeof(*names) ? names[type] : "invalid";
}

bool nativeScriptExpression_Assignable(NativeScriptIRType target,
                                      NativeScriptIRType value)
{
    if (target == NativeScriptType_Invalid || value == NativeScriptType_Invalid ||
        target == NativeScriptType_Void || target == NativeScriptType_Namespace) return false;
    if (target == value) return true;
    if (target == NativeScriptType_Bool) return false;
    if (nativeScriptExpression_IsIntegral(target)) return nativeScriptExpression_IsIntegral(value);
    if (target == NativeScriptType_F32 || target == NativeScriptType_F64)
        return nativeScriptExpression_IsNumeric(value);
    return value == NativeScriptType_Null &&
           (target == NativeScriptType_Object || target == NativeScriptType_Callable);
}

static bool append(NativeScriptIRArena* arena, const NativeScriptIRNode* node,
                   uint32_t* result, NativeScriptDiagnostic* diagnostic)
{
    if (!arena->nodes || arena->node_count >= arena->node_capacity)
        return fail(diagnostic, node->source, "Expression node capacity exhausted.");
    *result = arena->node_count;
    arena->nodes[arena->node_count++] = *node;
    return true;
}

bool nativeScriptExpression_Convert(NativeScriptIRArena* arena, uint32_t expression,
    NativeScriptIRType target, bool explicit_conversion, uint32_t* converted,
    NativeScriptDiagnostic* diagnostic)
{
    if (converted) *converted = NATIVE_SCRIPT_NODE_NONE;
    if (!arena || !converted || !arena->nodes || expression >= arena->node_count)
        return fail(diagnostic, (NativeScriptSourceSpan){0}, "Invalid conversion expression.");
    NativeScriptIRNode value = arena->nodes[expression];
    bool allowed = explicit_conversion ?
        nativeScriptExpression_IsNumeric(target) && nativeScriptExpression_IsNumeric(value.type) :
        nativeScriptExpression_Assignable(target, value.type);
    if (!allowed) return fail(diagnostic, value.source, "Expression conversion is not permitted.");
    if (target == value.type) { *converted = expression; return true; }
    NativeScriptIRNode node = { .source = value.source, .a = expression,
        .b = NATIVE_SCRIPT_NODE_NONE, .semantic = NATIVE_SCRIPT_SEMANTIC_NONE,
        .flags = NativeScriptFlag_Readable |
                 (value.flags & NativeScriptFlag_MayInvalidateSource),
        .kind = NativeScriptIR_Convert,
        .type = (uint8_t)target };
    return append(arena, &node, converted, diagnostic);
}

static bool next(ExpressionParser* parser) { return lex(&parser->lexer, &parser->token); }
static bool parse_expression(ExpressionParser*, unsigned, uint32_t*);

static NativeScriptSourceSpan joined(NativeScriptSourceSpan left,
                                    NativeScriptSourceSpan right)
{
    left.length = right.offset + right.length - left.offset;
    return left;
}

static NativeScriptIRNode token_node(const NativeScriptExpressionToken* token,
                                     NativeScriptIRKind kind)
{
    NativeScriptIRNode node = { .source = token->source,
        .text_offset = token->text_offset, .text_length = token->text_length,
        .a = NATIVE_SCRIPT_NODE_NONE, .b = NATIVE_SCRIPT_NODE_NONE,
        .semantic = NATIVE_SCRIPT_SEMANTIC_NONE, .bits = token->bits,
        .flags = NativeScriptFlag_Readable, .kind = (uint8_t)kind,
        .type = token->type, .operation = token->operation };
    return node;
}

static bool resolved(ExpressionParser* parser, NativeScriptIRNode* node,
                     uint32_t* result)
{
    NativeScriptIRResolution value = { NativeScriptType_Invalid,
        NATIVE_SCRIPT_SEMANTIC_NONE, 0, 0 };
    if (!parser->resolve || !parser->resolve(parser->context, parser->arena,
            node, &value, parser->lexer.diagnostic)) {
        if (!parser->lexer.diagnostic || !parser->lexer.diagnostic->message[0])
            fail(parser->lexer.diagnostic, node->source, "Unresolved semantic name, selector or call.");
        return false;
    }
    if (value.type <= NativeScriptType_Invalid || value.type > NativeScriptType_HitboxDefinition)
        return fail(parser->lexer.diagnostic, node->source, "Semantic resolver supplied an invalid type.");
    bool selector=nativeScriptType_SelectorLimit(value.type)!=0;
    if(selector && !(value.flags&NativeScriptFlag_Literal))
        return fail(parser->lexer.diagnostic,node->source,"Immutable selectors require a closed load-time literal.");
    if(value.flags&NativeScriptFlag_Literal) {
        if((node->kind!=NativeScriptIR_Name && node->kind!=NativeScriptIR_Member) ||
            (!nativeScriptExpression_IsNumeric(value.type)&&!selector) ||
            (value.flags&~NativeScriptFlag_Literal) || value.semantic!=NATIVE_SCRIPT_SEMANTIC_NONE ||
            (node->kind==NativeScriptIR_Member && (node->a>=parser->arena->node_count ||
                parser->arena->nodes[node->a].type!=NativeScriptType_Namespace)))
            return fail(parser->lexer.diagnostic,node->source,"Invalid load-time scalar literal resolution.");
        node->kind=selector?NativeScriptIR_Selector:value.type==NativeScriptType_Bool?NativeScriptIR_Boolean:
            value.type==NativeScriptType_F32||value.type==NativeScriptType_F64?NativeScriptIR_Floating:NativeScriptIR_Integer;
        node->type=(uint8_t)value.type;node->semantic=NATIVE_SCRIPT_SEMANTIC_NONE;
        node->flags=NativeScriptFlag_Pure|NativeScriptFlag_Readable;
        node->bits=value.literal_bits;
        if(selector && node->bits>=nativeScriptType_SelectorLimit(value.type))
            return fail(parser->lexer.diagnostic,node->source,"Immutable selector literal exceeds its closed domain.");
        if(value.type==NativeScriptType_Bool && node->bits>1)
            return fail(parser->lexer.diagnostic,node->source,"Boolean literal resolution must be zero or one.");
        if(value.type==NativeScriptType_I32) {
            uint32_t bits=(uint32_t)node->bits;
            int64_t signed_bits=bits<=INT32_MAX?(int64_t)bits:-1-(int64_t)(UINT32_MAX-bits);
            node->bits=(uint64_t)signed_bits;
        } else if(value.type==NativeScriptType_U32 || value.type==NativeScriptType_F32) {
            if(node->bits>UINT32_MAX)return fail(parser->lexer.diagnostic,node->source,"32-bit literal resolution exceeds its declared width.");
        }
        node->a=node->b=NATIVE_SCRIPT_NODE_NONE;node->argument_count=0;
        return append(parser->arena,node,result,parser->lexer.diagnostic);
    }
    node->type = (uint8_t)value.type; node->semantic = value.semantic;
    node->flags = value.flags;
    if (node->a != NATIVE_SCRIPT_NODE_NONE)
        node->flags |= parser->arena->nodes[node->a].flags & NativeScriptFlag_MayInvalidateSource;
    if (node->b != NATIVE_SCRIPT_NODE_NONE)
        node->flags |= parser->arena->nodes[node->b].flags & NativeScriptFlag_MayInvalidateSource;
    if (node->kind == NativeScriptIR_Call) {
        for (uint32_t i = 0; i < node->argument_count; ++i) {
            uint32_t argument = parser->arena->arguments[node->argument_first + i];
            node->flags |= parser->arena->nodes[argument].flags & NativeScriptFlag_MayInvalidateSource;
        }
    }
    return append(parser->arena, node, result, parser->lexer.diagnostic);
}

static bool postfix(ExpressionParser* parser, uint32_t* result)
{
    for (;;) {
        NativeScriptExpressionToken token = parser->token;
        NativeScriptIRNode base = parser->arena->nodes[*result];
        if (token.kind == NativeScriptToken_Dot) {
            if (!next(parser)) return false;
            token = parser->token;
            if (token.kind != NativeScriptToken_Name)
                return fail(parser->lexer.diagnostic, token.source, "Member selector requires a name.");
            NativeScriptIRNode node = token_node(&token, NativeScriptIR_Member);
            node.source = joined(base.source, token.source); node.a = *result;
            if (!resolved(parser, &node, result) || !next(parser)) return false;
        } else if (token.kind == NativeScriptToken_OpenIndex) {
            if (!next(parser)) return false;
            uint32_t index;
            if (!parse_expression(parser, 1, &index)) return false;
            if (parser->token.kind != NativeScriptToken_CloseIndex)
                return fail(parser->lexer.diagnostic, parser->token.source, "Index requires a closing bracket.");
            if (!nativeScriptExpression_IsIntegral(parser->arena->nodes[index].type))
                return fail(parser->lexer.diagnostic, parser->arena->nodes[index].source, "Index must be integral.");
            NativeScriptIRNode node = token_node(&token, NativeScriptIR_Index);
            node.source = joined(base.source, parser->token.source);
            node.a = *result; node.b = index;
            if (!resolved(parser, &node, result) || !next(parser)) return false;
        } else if (token.kind == NativeScriptToken_OpenParen) {
            uint32_t arguments[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS], count = 0;
            if (!next(parser)) return false;
            if (parser->token.kind != NativeScriptToken_CloseParen) {
                for (;;) {
                    if (count == NATIVE_SCRIPT_EXPRESSION_ARGUMENTS)
                        return fail(parser->lexer.diagnostic, token.source, "Expression argument limit exhausted.");
                    if (!parse_expression(parser, 1, &arguments[count++])) return false;
                    if (parser->token.kind != NativeScriptToken_Comma) break;
                    if (!next(parser)) return false;
                }
            }
            if (parser->token.kind != NativeScriptToken_CloseParen)
                return fail(parser->lexer.diagnostic, parser->token.source, "Call requires a closing parenthesis.");
            NativeScriptIRArena* arena = parser->arena;
            if (count > arena->argument_capacity - arena->argument_count || (count && !arena->arguments))
                return fail(parser->lexer.diagnostic, token.source, "Expression argument storage exhausted.");
            NativeScriptIRNode node = token_node(&token, NativeScriptIR_Call);
            node.source = joined(base.source, parser->token.source); node.a = *result;
            node.argument_first = arena->argument_count; node.argument_count = count;
            if (count) memcpy(arena->arguments + arena->argument_count, arguments, count * sizeof(*arguments));
            arena->argument_count += count;
            if (!resolved(parser, &node, result) || !next(parser)) return false;
        } else return true;
    }
}

static bool primary(ExpressionParser* parser, uint32_t* result)
{
    NativeScriptExpressionToken token = parser->token;
    NativeScriptIRNode node;
    if (token.kind == NativeScriptToken_OpenParen) {
        if (!next(parser) || !parse_expression(parser, 1, result)) return false;
        if (parser->token.kind != NativeScriptToken_CloseParen)
            return fail(parser->lexer.diagnostic, parser->token.source, "Expression requires a closing parenthesis.");
        parser->arena->nodes[*result].source = joined(token.source, parser->token.source);
        if (!next(parser)) return false;
        return postfix(parser, result);
    }
    if (token.kind == NativeScriptToken_Integer) node = token_node(&token, NativeScriptIR_Integer);
    else if (token.kind == NativeScriptToken_Floating) node = token_node(&token, NativeScriptIR_Floating);
    else if (token.kind == NativeScriptToken_String) node = token_node(&token, NativeScriptIR_String);
    else if (token.kind == NativeScriptToken_Name) {
        if (token_word(&parser->lexer, &token, "true") || token_word(&parser->lexer, &token, "false")) {
            node = token_node(&token, NativeScriptIR_Boolean); node.type = NativeScriptType_Bool;
            node.bits = token_word(&parser->lexer, &token, "true");
        } else if (token_word(&parser->lexer, &token, "None")) {
            node = token_node(&token, NativeScriptIR_Null); node.type = NativeScriptType_Null;
        } else {
            node = token_node(&token, NativeScriptIR_Name);
            if (!resolved(parser, &node, result) || !next(parser)) return false;
            return postfix(parser, result);
        }
    } else return fail(parser->lexer.diagnostic, token.source, "Expected an expression value.");
    if (!append(parser->arena, &node, result, parser->lexer.diagnostic) || !next(parser)) return false;
    return postfix(parser, result);
}

static NativeScriptIRType integral_promotion(NativeScriptIRType type)
{
    return type == NativeScriptType_Bool ? NativeScriptType_I32 : type;
}

static NativeScriptIRType numeric_common(NativeScriptIRType left,
                                        NativeScriptIRType right)
{
    if (left == NativeScriptType_F64 || right == NativeScriptType_F64) return NativeScriptType_F64;
    if (left == NativeScriptType_F32 || right == NativeScriptType_F32) return NativeScriptType_F32;
    left = integral_promotion(left); right = integral_promotion(right);
    if (left == NativeScriptType_U64 || right == NativeScriptType_U64) return NativeScriptType_U64;
    if (left == NativeScriptType_I64 || right == NativeScriptType_I64) return NativeScriptType_I64;
    if (left == NativeScriptType_U32 || right == NativeScriptType_U32) return NativeScriptType_U32;
    return NativeScriptType_I32;
}

static bool unary(ExpressionParser* parser, uint32_t* result)
{
    NativeScriptExpressionToken token = parser->token;
    NativeScriptIROperation op = token.operation;
    bool is_unary = token.kind == NativeScriptToken_Operator &&
        (op == NativeScriptOp_Add || op == NativeScriptOp_Subtract ||
         op == NativeScriptOp_Not || op == NativeScriptOp_Invert);
    if (!is_unary) return primary(parser, result);
    if (++parser->depth > NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return fail(parser->lexer.diagnostic, token.source, "Expression nesting limit exhausted.");
    uint32_t value;
    bool ok = next(parser) && unary(parser, &value);
    --parser->depth;
    if (!ok) return false;
    NativeScriptIRNode operand = parser->arena->nodes[value];
    if (!nativeScriptExpression_IsNumeric(operand.type) ||
        (op == NativeScriptOp_Invert && !nativeScriptExpression_IsIntegral(operand.type)))
        return fail(parser->lexer.diagnostic, operand.source, "Unary operator requires a compatible numeric value.");
    NativeScriptIRType type = op == NativeScriptOp_Not ? NativeScriptType_Bool : integral_promotion(operand.type);
    if (op != NativeScriptOp_Not &&
        !nativeScriptExpression_Convert(parser->arena, value, type, true, &value, parser->lexer.diagnostic)) return false;
    NativeScriptIRNode node = token_node(&token, NativeScriptIR_Unary);
    node.source = joined(token.source, operand.source); node.a = value; node.type = (uint8_t)type;
    node.flags |= operand.flags & NativeScriptFlag_MayInvalidateSource;
    node.operation = op == NativeScriptOp_Add ? NativeScriptOp_Positive :
                     op == NativeScriptOp_Subtract ? NativeScriptOp_Negative : op;
    return append(parser->arena, &node, result, parser->lexer.diagnostic);
}

static unsigned precedence(NativeScriptIROperation op)
{
    switch (op) {
    case NativeScriptOp_Or: return 1;
    case NativeScriptOp_And: return 2;
    case NativeScriptOp_BitOr: return 3;
    case NativeScriptOp_BitXor: return 4;
    case NativeScriptOp_BitAnd: return 5;
    case NativeScriptOp_Equal: case NativeScriptOp_NotEqual: return 6;
    case NativeScriptOp_Less: case NativeScriptOp_LessEqual:
    case NativeScriptOp_Greater: case NativeScriptOp_GreaterEqual: return 7;
    case NativeScriptOp_ShiftLeft: case NativeScriptOp_ShiftRight: return 8;
    case NativeScriptOp_Add: case NativeScriptOp_Subtract: return 9;
    case NativeScriptOp_Multiply: case NativeScriptOp_Divide:
    case NativeScriptOp_Remainder: return 10;
    default: return 0;
    }
}

static bool binary(ExpressionParser* parser, uint32_t left, uint32_t right,
                    NativeScriptIROperation op, uint32_t* result)
{
    NativeScriptIRNode lhs = parser->arena->nodes[left], rhs = parser->arena->nodes[right];
    bool logical = op == NativeScriptOp_And || op == NativeScriptOp_Or;
    bool comparison = op >= NativeScriptOp_Equal && op <= NativeScriptOp_GreaterEqual;
    bool integral = op == NativeScriptOp_Remainder ||
        (op >= NativeScriptOp_BitAnd && op <= NativeScriptOp_ShiftRight);
    bool numeric = nativeScriptExpression_IsNumeric(lhs.type) && nativeScriptExpression_IsNumeric(rhs.type);
    bool identity = (op == NativeScriptOp_Equal || op == NativeScriptOp_NotEqual) &&
        ((lhs.type == rhs.type && (lhs.type == NativeScriptType_Object || lhs.type == NativeScriptType_Callable || lhs.type == NativeScriptType_Null)) ||
         (lhs.type == NativeScriptType_Null && (rhs.type == NativeScriptType_Object || rhs.type == NativeScriptType_Callable)) ||
         (rhs.type == NativeScriptType_Null && (lhs.type == NativeScriptType_Object || lhs.type == NativeScriptType_Callable)));
    if ((!numeric && !identity) ||
        (integral && (!nativeScriptExpression_IsIntegral(lhs.type) || !nativeScriptExpression_IsIntegral(rhs.type))))
        return fail(parser->lexer.diagnostic, joined(lhs.source, rhs.source), "Binary operator requires compatible operand types.");
    NativeScriptIRType type;
    if (logical || identity) type = NativeScriptType_Bool;
    else {
        bool shift = op == NativeScriptOp_ShiftLeft || op == NativeScriptOp_ShiftRight;
        NativeScriptIRType common = shift ? integral_promotion(lhs.type) : numeric_common(lhs.type, rhs.type);
        NativeScriptIRType right_type = shift ? integral_promotion(rhs.type) : common;
        if (!nativeScriptExpression_Convert(parser->arena, left, common, true, &left, parser->lexer.diagnostic) ||
            !nativeScriptExpression_Convert(parser->arena, right, right_type, true, &right, parser->lexer.diagnostic)) return false;
        type = comparison ? NativeScriptType_Bool : common;
    }
    NativeScriptIRNode node = { .source = joined(lhs.source, rhs.source),
        .a = left, .b = right, .semantic = NATIVE_SCRIPT_SEMANTIC_NONE,
        .flags = NativeScriptFlag_Readable | (logical ? NativeScriptFlag_ShortCircuit : 0) |
                 ((lhs.flags | rhs.flags) & NativeScriptFlag_MayInvalidateSource),
        .kind = NativeScriptIR_Binary, .type = (uint8_t)type, .operation = (uint8_t)op };
    return append(parser->arena, &node, result, parser->lexer.diagnostic);
}

static bool parse_expression(ExpressionParser* parser, unsigned minimum,
                              uint32_t* result)
{
    if (++parser->depth > NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return fail(parser->lexer.diagnostic, parser->token.source, "Expression nesting limit exhausted.");
    uint32_t left;
    if (!unary(parser, &left)) { --parser->depth; return false; }
    for (;;) {
        NativeScriptIROperation op = parser->token.operation;
        unsigned rank = parser->token.kind == NativeScriptToken_Operator ? precedence(op) : 0;
        if (rank < minimum) break;
        uint32_t right;
        if (!next(parser) || !parse_expression(parser, rank + 1, &right) || !binary(parser, left, right, op, &left)) {
            --parser->depth; return false;
        }
    }
    --parser->depth; *result = left;
    return true;
}

bool nativeScriptExpression_Parse(NativeScriptIRArena* arena,
    NativeScriptSourceSpan span, NativeScriptIRResolver resolver, void* context,
    uint32_t* expression, NativeScriptDiagnostic* diagnostic)
{
    if (expression) *expression = NATIVE_SCRIPT_NODE_NONE;
    if (diagnostic) memset(diagnostic, 0, sizeof(*diagnostic));
    if (!arena || !expression) return fail(diagnostic, span, "Invalid expression arena or output.");
    if (!source_valid(arena->source, arena->source_bytes, span, diagnostic)) return false;
    if (arena->node_count > arena->node_capacity || arena->argument_count > arena->argument_capacity)
        return fail(diagnostic, span, "Expression arena counts exceed capacity.");
    uint32_t nodes = arena->node_count, arguments = arena->argument_count;
    ExpressionParser parser = { .lexer = { arena->source, span.offset,
        span.offset + span.length, span.line, span.column, diagnostic, false },
        .arena = arena, .resolve = resolver, .context = context };
    uint32_t result;
    if (next(&parser) && parse_expression(&parser, 1, &result) && parser.token.kind == NativeScriptToken_End) {
        *expression = result; return true;
    }
    if (!diagnostic || !diagnostic->message[0]) fail(diagnostic, parser.token.source, "Unexpected token after expression.");
    arena->node_count = nodes; arena->argument_count = arguments;
    return false;
}
