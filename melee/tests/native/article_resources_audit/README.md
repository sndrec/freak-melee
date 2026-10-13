# Owned Chain graph audit probe

```powershell
python tests/native/article_resources_audit/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso' --output 'A:/FreakMelee-test-archives/article-resource-owned-import-new'
```

Requires already-built native libraries and the existing `native_source_assets_test.exe` link dependencies in `build/native/build.ninja`. Builds only this isolated probe. Compile/probe processes use `CREATE_NO_WINDOW`, 120-second bounds and fresh private A: output; receipt records exact commands and verifies input hashes remain unchanged. No global build/game.

Actual acceptance: `A:/FreakMelee-test-archives/article-resource-owned-import-review3/receipt.json`, PASS12. Retail Chain has20 links, special-attribute source0x3C1C, Link100/Tip104 both target0x11EC8. Actual typed import already owns their shared descriptor graph; named-root preparation preserves the same cached graph pointers. The named declarations are controlled probe input, not acceptance of source metadata/compiler/resource construction. No mutable JObj instances, collision, render or gameplay are exercised.

The first probe incorrectly asserted distinct Link/Tip descriptor pointers; that failed archive is retained. Distinct logical names are allowed to alias one immutable graph. See `docs/native_source_article_resources_audit.md` for the connected implementation requirements.
