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

The current atlases cover printable ASCII. Name and tile faces display lowercase
input with their uppercase forms. Non-ASCII names display a replacement glyph.
Original glyphs are retained where available; reconstructed letters and the
additional punctuation/numerals still require visual review. The supplied Folk
family is available for additional notices; it is not a universal replacement
for every Melee typeface.

This tool requires Pillow in the build's Python environment. Generated atlases
and PNG inspection sheets belong in the build output, not source control.

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
