# Private menu-font inputs

`FolkPro-Medium.otf` and `FolkPro-Heavy.otf` were supplied by the user from
`FolkPro _Family.zip` on 2026-10-13. They are font assets, not project-authored
or open-source code. Do not infer redistribution rights from the repository's
code licenses.

`tools/prepare_menu_fonts.py` builds separate GX atlases for notices, the CSS
heading, CSS tile labels, results names and the results winner banner. The last
two reuse glyphs from the verified original disc. Headings use the supplied
Folk Heavy font; notices use Folk Medium. CSS tile labels use the user-supplied
`assets/ui/fonts/angular_glyph_set.svg`.
It contains outlined capital letters, digits, punctuation and additional symbols.
The generator preserves the shared baseline and holes within letters, and
rasterizes at a higher resolution before reducing to the atlas size. The earlier
PNG alphabet remains a visual reference.

The atlases cover printable ASCII, accented Latin letters and common punctuation,
including curly quotes, dashes and ellipses. Name and tile faces use uppercase
forms. Notices and headings use Folk's own accented glyphs where available.
Missing accents are built over that context's existing letters. Disc-derived
faces also use their own letters as the bases for accents and ligatures.
Remaining non-decomposable forms use the supplied Folk family at the matching
cap height. Original ASCII artwork and spacing are preserved.

The renderer measures and draws UTF-8 characters. Common precomposed and
combining forms, such as `é` and `e` followed by an acute accent, select the same
glyph. An unsupported character displays one question mark. Coverage is bounded;
full international shaping and right-to-left layout remain future work.
Reconstructed glyphs and their typography still need the user's visual review.

This tool requires Pillow in the build's Python environment. Generated atlases
and PNG inspection sheets belong in the build output, not source control.
`prepare_menu_unicode.py` supplies the accent composition and additional glyphs.
The packed atlases store code points, glyph placement/spacing, vertical offsets
and composition pairs. The native loader validates this data once when the
fonts are acquired. Drawing uses fixed lookup tables and resident texture data.

`menu-option` is a separate outlined face for the main-menu ribbons and hub
buttons. `prepare_menu_lettering.py` extracts its letters from the English
`MnMaAll` option-name textures. Letters absent from those names are reconstructed
using the captured stroke width and proportions. Additional punctuation and
digits use the supplied Folk family. The generated IA4 atlas keeps the dark
outline and white fill in one texture; the other faces use I4 masks.

`menu-caption` extracts the italic page-heading face from the main panel's
caption textures. The generator removes the shared shear before separating
letters, and the renderer reapplies it across each glyph. The resulting strings
share a baseline. The hub places these captions through the original model
joint and its material opacity. Reconstructed letters and punctuation remain
part of the user's visual review.
