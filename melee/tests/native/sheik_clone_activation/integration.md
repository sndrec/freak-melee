# Root-owned integration inventory (not applied)

The new base lives at the actual stock directory `ftSeak`, with requested readable
filenames `ftsheik_clone.c/.h` and symbol `ftSheikClone_Definition`. Existing
CMake's `MELEE_GAME_SOURCES` recursive glob discovers the source automatically;
no manual source entry or shim is required.

For later normal clone/import preparation, root must add these exact data rows:

- `native_fighter_runtime.c`: include
  `<melee/ft/kinds/ftSeak/ftsheik_clone.h>` and add
  `{ Ft_Kind_Seak, &ftSheikClone_Definition }` to `stock_clone_bases`. The existing
  FindDefinition/FindStockDefinition loops then resolve `sheik_clone` naturally.
- `native_fighter_project.c` StockBaseName: `case Ft_Kind_Seak: return "Sheik";`.
- `native_fighter_article_bindings.c`: four actual import catalog rows:

```c
SLOT(Ft_Kind_Seak, It_Kind_Seak_NeedleThrow, 0, -1),
SLOT(Ft_Kind_Seak, It_Kind_Seak_NeedleHeld, 1, -1),
SLOT(Ft_Kind_Seak, It_Kind_Seak_Vanish, 2, -1),
SLOT(Ft_Kind_Seak, It_Kind_Seak_Chain, 3, -1),
```

The generic typed importer already knows the immutable Chain format and separate
joint slots4/5. The base owns that recipe, four article bindings, 52 semantic
parts and pose declarations. No new runtime DAT-offset primitive is needed.
The stock-name parser/CLI help should be reviewed against the accepted spelling
`Sheik`; its full stock selection names currently already include Sheik, while
the supported clone help omits it.

Do **not** wire the partial composition as a Workshop default or declare
independence merely after these rows exist. Paired metadata/import/launch and
generic transfer plus destination hooks from `paired_contract.md` must close
first. Owned Zelda remains a separate absent catalog base. The eventual actual
full source becomes the Sheik default only after lifecycle/CPU/Transform source
comparisons, complete generated module guard and root's gameplay acceptance.

Current disjoint source proof requires no CTest registration: run `audit.py` and
`probe.py` as documented. A later registered-base metadata/import test can use
the census values and same complete five-costume proof. Existing accepted Chain
and Needle/Vanish source matrices remain unchanged dependencies.
