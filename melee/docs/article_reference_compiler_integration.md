# Read-only article reference compiler integration

The new `native_fighter_script_article_refs.c/.h` component collects every
declared article's kind and state layout before callback translation. It parses
`articles.ref(handle, It_Kind_Declared).exists` and
`articles.ref(handle, It_Kind_Declared).state.field[index]`. Scalar state reads
omit the index. Field descriptor indices are constants from the target article's
declaration order, independent of the referring article's state layout.

Compiler and shared backend integration are accepted: the full Release build4
passed, followed by all 66 focused checks in 18.07 seconds. These include the
catalog/parser fixture, full compiler fixture, both generated callback fixtures,
and the actual shared article-reference backend fixture. Evidence is
`.local/native-script-generic7-build4.log` and
`.local/native-script-generic7-checks.log`. This is source/backend acceptance;
it does not establish Ness PK Thunder gameplay parity.

## Implemented architecture

1. `NativeFighterItemProgram` owns one `NativeScriptArticleCatalog` and has
   `SCRIPT_ARTICLE_DECLARATIONS` (16) article declaration slots. `CatalogParse`
   runs before the item parser's translating pass. `nativeScriptFunctions_Parse`
   extracts helper source before item parsing without compiling expressions.
   The catalog therefore exists before helper **compilation**.
2. `ArticleDeclaration` borrows a const catalog pointer. At each header, the
   parser copies the catalog declaration's state table into `local_state`.
   The translating pass retains declaration-placement checks without adding
   the fields again. The fixed catalog and all borrowed
   pointers live until `GenerateC` finishes both fighter and article emission.
3. `ScriptCodeBlock` borrows a const catalog pointer propagated into
   every fighter block and helper-function compile context, including callbacks
   bound from helpers. This makes forward references work in fighter and article
   bodies without a process-global catalog.
4. Each expression scanner calls `Reference` before ordinary identifier scanning.
   Zero lets another parser proceed; negative is a compile error. Successful
   references recursively classify and translate `handle` and array `index`
   with the owning context. They require integer expressions, allowing
   explicit `int()`/`uint()` conversions through the typed expression parser.
   Handles cannot pass through float locals or parameters. Scalars emit index 0.
   `Emit` receives translated strings, and the scanner advances to `after`.
   `.exists` is bool; state reads use the resolved field's exact type.
5. Reference assignment targets are explicitly rejected. Host-check scans require
   the exact emitted `article_ref_*` member. The component emits direct calls with
   `gobj`, a `uint32_t` handle,
   the declared `It_Kind_*` symbol, constant field ordinal and `int32_t` index.
   Neither expression context performs an implicit kind remap. The Host validates
   the caller's immutable package definition, item kind and article logic.

The catalog shares the existing state declaration parser: at most 64 fields,
512 bytes and 128 elements per array. It also validates duplicate article kinds
and limits article declarations to 16. Article motion-state capacity remains 32.
The second pass still owns all callback/state placement and body validation;
catalog collection alone does not accept an invalid complete script.

Negative/out-of-range integer indices remain valid compiled expressions. The
runtime returns typed zero for missing/stale handles, foreign packages, wrong
kinds/logic, missing state or invalid bounds. Float handles/indices are rejected
by typed compiler integration, rather than silently rounded here.

This batch covers fighter callbacks/helpers and authored article callbacks/signals.
Plain `item Name` holdable callbacks are outside this reference context: they use
separate owner metadata rather than `native_article_author_kind`. No global
catalog or wrapper was added to extend them implicitly.

## Accepted verification

`tests/native/script_article_refs_test.c` exercises forward targets with different
field orders, all three read types, scalar/array shape errors, nested handle/index
expressions, full-bit hexadecimal handles, output capacity failures, 16/17 article
limits, 64/65 field limits, and the exact 512-byte state endpoint. Its compiled
`native_script_article_refs` check passed.

`script_article_refs_compiler_test.c` passed both fighter compiler forms and
authored article callbacks, including forward helper reads, float-handle/index
rejection, read-only assignment rejection, and field/array-shape rejection.
`script_article_refs_module_test.c` executed both actual generated modules:
high-bit `0xFEDCBA98` IDs, signed integer results, bool results, negative-zero
float bits, negative indices, different target field ordering, and each missing
Host requirement behaved as expected. These passed as
`native_script_article_refs_compiler`, `native_script_article_refs_action`, and
`native_script_article_refs_callback`.

`article_refs_test.c` executed the actual shared backend. It preserved creation
IDs above 2^24 and high-bit IDs, accepted same-package aliases independently of
damage ownership, and rejected stale/foreign/wrong-kind/wrong-logic references.
It also checked descriptor type/stride/index bounds, truncated or oversized state,
missing metadata, removed caller/target, null caller and absent live registry.
Its `native_article_refs` check passed. These fixtures exercise source contracts
and generated callbacks without launching a game; multi-article Ness gameplay
acceptance remains separate.
