# Model-less Article import probe

Reads original `PlZd.dat` directly from the ISO and links the isolated probe against existing native libraries. It checks actual raw NULL relocation semantics and the complete owned character import's two Zelda Articles. It does not invoke Ninja, a game, Item constructors or render callbacks.

```powershell
python tests/native/model_less_article_audit/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso' --output 'A:/FreakMelee-test-archives/model-less-article-import-new'
```

Requires current `build/native/build.ninja`, the existing `native_source_assets_test.exe` link dependency block and already-built native libraries. The output must be a fresh private A: directory. Compile/probe processes use `CREATE_NO_WINDOW` and 120-second bounds. The receipt records exact commands and input hashes and verifies libraries/headers were unchanged.

Acceptance: `A:/FreakMelee-test-archives/model-less-article-import-review/receipt.json`, PASS18. Both slots have NULL imported roots, zero bone count, NULL dynamics and NULL animation/material/shape graphs, with 2/1 actual command scripts. See `docs/native_source_model_less_article_audit.md` for source lifetime evidence, limitations and remaining acceptance.
