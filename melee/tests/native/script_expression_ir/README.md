# Direct semantic expression frontend

`native_script_expression.c` reads FMS expressions directly into the nodes in
`native_script_ir.h`. It does not call the old C emitter, parse generated C,
consume native layouts, or construct callable machine addresses. The caller
provides immutable source and bounded node/argument storage at content load.
Parsing and tokenization allocate no heap memory.

`nativeScriptExpression_Parse` requires a semantic resolver for names,
members, indexes and calls. The resolver receives the proposed node and already
resolved children; it supplies type, semantic ID and ownership/access/effect
flags. It must validate callable signatures, context, selectors and typed
output destinations. Unknown names do not default to Float or execute a native
fallback. Call argument IDs occupy a contiguous range after nested arguments
have been appended.

Chained selectors remain structured: `item.resource(node).position.x`, joint
paths, arrays and recipient calls retain every Name/Member/Call/Index node.
Names and raw string contents refer to immutable source spans. Source spans
also retain the originating line/column for diagnostics. Source-invalidation
effects propagate through enclosing nodes and call arguments for cheap effect
analysis. Execution guards still belong at the actual reached calls; a flagged
short-circuit expression must not execute or guard an unreached branch.

Numeric rules are explicit:

- Declared/public Float properties and Float helper results are F32.
- Unsuffixed floating literals are finite F64 values. F32 arithmetic retains
  F32 intermediates; mixing F32 with F64 explicitly promotes to F64.
- Decimal integers select I32 then I64; hexadecimal/octal masks can select U32
  or U64. Integer literals outside their representable range fail.
- Bool arithmetic promotes to I32. Integral operands use C-compatible common
  signed/unsigned types; shifts promote each operand independently.
- Comparison and `not` return Bool. `and`/`or` retain short-circuit nodes and
  their left-to-right evaluation structure.
- Float assignment/return/arguments can request conversion from other numeric
  types. Bool assignment is strict. Implicit Float-to-Int assignment fails;
  an explicit `int()`/`uint()` binding can request a deliberate conversion.

The shared `nativeScriptExpression_Tokenize` API supports structural inventories
without pretending unknown engine bindings passed semantic validation. It
recognizes assignment/colon tokens for statement readers and ignores comments
outside strings. Lexical success is separate from grammar/type acceptance.

The console fixture exercises actual Nana interpolation expressions, Float
physics arithmetic, unsigned handles/masks, nested resource selectors, article
arrays, typed recipient returns, writable distinct position outputs and callback
references. It rejects wrong types, unknown names, malformed syntax, invalid
numbers and exhausted bounded storage. Failure restores both arena counts.

```powershell
python tests/native/script_expression_ir/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-expression-ir-example
```

Output and executables stay on `A:`. This fixture validates the direct frontend;
it does not claim a complete engine binding registry or bytecode/game parity.
