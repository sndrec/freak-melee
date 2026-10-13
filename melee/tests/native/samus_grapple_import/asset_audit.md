# Samus grapple animation components: Gate 1 asset audit

The seven external components are genuinely empty under the retail load contract. They are not missing graphs supplied by a costume/common archive. Preserve them as explicit `None` channels in the owned recipe; retain strict external rejection in the generic direct/indexed graph importer.

This is a read-only asset/source audit, not an importer execution, rendering or gameplay acceptance. No production source, ISO bytes, default template or package was changed. The tracked JSON preserves the exact census and hashes; raw extracted `PlSs.dat` stays in the A-drive output directory.

## Exact recipe

Offsets are relative to the grapple attribute descriptor at DAT data offset **16912**. Every present channel is a pointer array, **index 0**, not a direct graph. Values in parentheses are that first graph's data offset; the outer array must be resolved and validated before reading its element.

| Named bundle | Joint channel | Material channel | Shape channel |
|---|---|---|---|
| ChainA | `array(116, 0)` (105440) | `array(120, 0)` (105696) | `None` |
| ChainB | `array(128, 0)` (104448) | `array(132, 0)` (104704) | `None` |
| ChainC | `array(140, 0)` (99816) | `array(144, 0)` (100072) | `None` |
| Top | `array(152, 0)` (88184) | `None` | `None` |
| Start | `array(164, 0)` (97228) | `None` | `None` |

The table describes component inputs, not a new parser syntax. Local joint model roots remain separate: attribute offsets 100/104/108/112 point to roots 105160/104168/97672/71496. Stock attaches these animation bundles to each loaded model's **first child**, then requests frame 0 on that child. Root-versus-subtree targeting must be preserved independently of component import.

## External records

Each external has two pointer slots. The word in the first slot identifies the second slot; the second word is `0xFFFFFFFF`, ending the relocation chain. Neither word is a graph pointer.

| Attribute byte offset | First pointer slot | Second pointer slot | Exact external name |
|---|---:|---:|---|
| `0x7C` / 124 | 17036 | 39404 | `ItmSamusGBeamChainA_TopN_shapeanim_joint` |
| `0x88` / 136 | 17048 | 39416 | `ItmSamusGBeamChainB_TopN_shapeanim_joint` |
| `0x94` / 148 | 17060 | 39428 | `ItmSamusGBeamChainC_TopN_shapeanim_joint` |
| `0x9C` / 156 | 17068 | 39436 | `ItmSamusGBeamTop_TopN_matanim_joint` |
| `0xA0` / 160 | 17072 | 39440 | `ItmSamusGBeamTop_TopN_shapeanim_joint` |
| `0xA8` / 168 | 17080 | 39448 | `ItmSamusGBeamStart_TopN_matanim_joint` |
| `0xAC` / 172 | 17084 | 39452 | `ItmSamusGBeamStart_TopN_shapeanim_joint` |

The census scans public and external symbol tables of **805 structurally bounded DAT archives** in the USA v1.02 disc. Those seven names occur only in `PlSs.dat`'s external table: **zero public definitions** under those names. Explicitly inspected owned inputs are `PlSs.dat`, `PlCo.dat`, all five costumes (`PlSsNr/Bk/Gr/La/Pi.dat`), and `PlSsDViWaitAJ.dat`; none contains matching public roots. This establishes symbol availability, not a search for anonymous byte sequences that could coincidentally resemble graphs.

## Actual loader and typed stock behavior

`src/melee/lb/lbarchive.c`, `lbArchive_InitializeDAT`, retail branch (lines 28–44) explicitly binds each external to NULL after parsing:

```c
while (true) {
    symbol = HSD_ArchiveGetExtern(archive, i++);
    if (symbol != NULL) {
        HSD_ArchiveLocateExtern(archive, symbol, NULL);
    }
    if (symbol == NULL) {
        return;
    }
}
```

`ftdata.c`'s fighter archive load uses `lbArchive_80017040`; its actual archive initialization reaches this operation. The repository contains no other call to `HSD_ArchiveLocateExtern` that subsequently supplies a graph. `src/sysdolphin/baselib/archive.c:96` walks external-chain offsets and overwrites each pointer slot with the supplied address. Its retail NULL binding therefore clears both slots in each listed chain.

`src/melee/ft/ftcharacter_assets.c:898`, `import_graph_pointer_slot`, preserves this for typed stock imports:

```c
*output = NULL;
if (kind == HSD_DAT_REF_NULL) {
    return true;
}
if (kind == HSD_DAT_REF_EXTERNAL) {
    import->unresolved_external_count++;
    import->external_symbol = external;
    return true;
}
```

For a local outer pointer, that function allocates one native pointer element and calls `import_optional_graph_slot` on the referenced array's first slot. That helper also allows an external inner component to remain NULL while recording diagnostics. `import_samus_grapple_attributes` at line 933 maps all fifteen fields from `0x74` through `0xAC` to their actual joint/material/shape types. It requires the original attribute span to be `0xB0`, copies scalar bytes separately and imports the four model roots separately.

`HSD_DatResolve` classifies a slot as external by traversing external references **before** interpreting its raw word as a local/null pointer. General `HSD_DatImportGraph` reference handling allows unresolved externals only when explicitly enabled. The authored `itArticle_ImportModelAnimations` currently rejects external components, including at indexed array resolution; that strict generic behavior need not change for this asset. Explicit omission of these seven channels agrees with both existing retail and typed stock behavior.

Stock `itsamusgrapple.c` explicitly guards the relevant outer pointers in `it_802B743C` (ChainA/B/C), `samus_grapple_setup_tail` (Top), and the Start construction block near line 504. For example:

```c
HSD_JObjAddAnimAll(tail_jobj->child,
                  (attrs2->x98 != NULL) ? *attrs2->x98 : NULL,
                  (attrs2->x9C != NULL) ? *attrs2->x9C : NULL,
                  (attrs2->xA0 != NULL) ? *attrs2->xA0 : NULL);
HSD_JObjReqAnimAll(tail_jobj->child, 0.0f);
```

The conclusion rests on loader behavior, typed import behavior, raw symbol records and actual null guards together. It does not infer emptiness from a symbol name or fabricated address.

## Resolution contract

1. Resolve the declared component slot as NULL/local/external/invalid using actual DAT metadata. Never use the unrelocated external-chain word as a graph offset.
2. For a declared array component, require a bounded local array before selecting its declared index, then resolve/import that element with the declared graph type.
3. Keep generic external/invalid graph rejection. For these audited retail-empty channels, the owned constructor declares `None` explicitly; no unresolved graph or runtime donor lookup is needed.
4. Preserve all eight local graph imports, the five bundles and first-child attachment. Do not replace whole bundles with empty graphs merely because one component is external.

## Provenance and reproduction

Input inventory: `.local/samus-tether-assets52/inventory.json`, SHA256 `30d3e474864e21f941d1b1bd1d482e9cf87e72c1955d6d8ecc9f911d46f67779`; its frozen52 manifest SHA256 is `e012438a7e998183c642c01485b99fa4fc7758ac29689f194fa7f68a45bf5ec1`. This historical inventory is independently checked against newly extracted retail pointer slots and all eight local first elements.

Retail `PlSs.dat` SHA256: `8fc0047354d551c8854e9834a3aad08e377f9855ab2572f47d0b41adbf347c22`. Disc header (first `0x440` bytes) SHA256: `d6aa48b44849cbfb5cd20eff0d7bab1cc1db9d17dc3972e27373ae0ee2fbb3aa`; FST SHA256: `ef099ee0edf64189544d36fe8e4be2293282d595eef27715df8aacb95fc6d4d7`. `asset_audit.json` records all owned-input and audited-source hashes. Sources are the live read-only audit inputs, not a claim that the current importer is frozen52.

```powershell
python tests/native/samus_grapple_import/audit.py `
  --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso' `
  --inventory .local/samus-tether-assets52/inventory.json `
  --output A:/FreakMelee-test-archives/samus-grapple-asset-audit-root
```

Use a fresh output directory. The script reuses the committed `samus_tether_composition/audit.py` DAT classifier; it scans raw tables and writes only isolated audit evidence. Worker result: **805 archives, seven matching external records, seven external and eight local components**, output `A:/FreakMelee-test-archives/samus-grapple-asset-audit-final2/audit.json`. No C compilation, importer execution or game was run.
