# Resource recipe metadata

The hidden console cohort uses the actual source parser and exact owner recipe
preparer. It covers absent metadata, every capacity 2..90, Article/Item owner
isolation, extra part 139 across rig counts 1..140, hex/whitespace parsing,
omitted-part insertions, checked final-count limits/overflow, signs,
trailing/duplicate fields and declarations, nested/block recipes,
invalid spans and revalidation after the final rig changes. Failures preserve
the recipe output. There are no native callbacks, collection allocations or
successful runtime substitutes in this pure preparation fixture.

`reserve_part` names an extra native slot: it must be at least the final owned
part count (`joints + omission_count`) and below 140. The final count is checked
before addition; a rig with 140 final parts has no available external part.
Missing recipe is `{capacity=0,reserved_part=-1}`.
Capacity-only metadata does not require rig facts.

```powershell
python tests/native/script_resource_metadata/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-resource-metadata-final
```

This proves source recipe preparation, not public body bindings, complete Item
program publication, collection lifecycle, actual DAT rig import or gameplay.
