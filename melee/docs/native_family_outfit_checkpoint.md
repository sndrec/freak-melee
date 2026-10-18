# Coordinated custom outfits

Checked on 2026-10-17. A custom outfit now contains a model for every member
of its exact parent fighter family. The parent keys identify those models.
Zelda/Sheik and Popo/Nana each use one outfit selection. Single fighters use
the same format with one member.

The loader imports all member models before publishing the selection. Each
actor resolves its own model and the outfit's shared cosmetic variant. Failure
to import the second model releases the first and leaves the port without a
partially loaded outfit. Wrong parent identities, member keys, counts and
skeleton sizes are rejected at preparation.

## Selection and import

X/Y cycles a custom fighter's installed outfits and its included default model
on the actual CSS. The outfit name is drawn on the player card through GX with
the menu notice typeface. Picking up and replacing the cookie on the same
fighter preserves its outfit. Local cards now publish the selected outfit ID
alongside the fighter ID.

Costumes lists custom fighters and filters outfits by their exact parent.
The existing import button asks for each member's DAT in order, with a member
caption on the file picker. The developer importer accepts a directory of
`MEMBER_KEY.dat` files. Collection metadata is loaded into a fixed catalog at
CSS entry or collection changes; cycling does not scan the installed directory.

Custom outfit packets now have a member directory. Earlier development custom
outfits must be rebuilt from their model files. Stock costume packets and
fighter packets keep their existing encoding.

## Evidence

- The native build passes. `native_content_store` and the real-ISO
  `native_costume_package` CTests pass. The codec checks both payload spans,
  every truncation, unchanged outputs on parse failure, duplicate keys, invalid
  counts and member length overflow.
- `tools/review_native_family_outfits.py` passes for the received Zelda/Sheik,
  Popo/Nana and Fox source fixtures. Each member receives an edited texture.
  The script imports, installs and verifies the exact packaged DATs. It rejects
  a missing second model. The actual runtime test rejects swapped member keys,
  wrong parents and a malformed second model, verifies no partial publication,
  resolves each member's DAT and variant, and loads cache-only outfits without
  installing them. Receipt and hashes:
  `.local/native-family-outfits-1/receipt.json`.
- `tools/review_native_css_outfits.py` passes in an isolated profile with the
  actual hidden, muted client. For all three families it exercises X/Y in both
  directions, wraparound, card labels, cookie pickup/replacement, entry into
  practice and return. Reviewed PNGs show the labels within the stock cards
  and rendered practice actors, including both Climbers. Receipt and hashes:
  `.local/native-css-outfits-1/receipt.json`.
- The native CLI dependency acceptance batch passes again with the new outfit
  format: `.local/native-costume-content-3/receipt.json`. Missing and different
  parents remain uninstalled. Failed marker publication rolls back a newly
  added parent and preserves an existing parent; repeated acceptance preserves
  the two installed identities.

These checks cover local imports, model resolution, CSS controls and practice.
The Windows file-picker sequence itself still needs user evaluation. Missing
parent transfer during gifting and a fresh online match with these coordinated
outfits remain open. These runs do not establish relay or other-PC behavior.
