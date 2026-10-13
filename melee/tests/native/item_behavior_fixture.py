"""Extract actual constructor policy entry points; allocation is a fixture boundary."""
from pathlib import Path
import sys
from callback_engine_fixture import function

root, output = map(Path, sys.argv[1:])
source = (root / "src/melee/it/item.c").read_text(encoding="utf-8")
names = ["Item_80268B18", "Item_CreateEngine", "Item_CreateAuthored",
         "Item_80268B5C", "Item_80268B9C"]
output.write_text("\n\n".join(function(source, name) for name in names) + "\n",
                  encoding="utf-8")
