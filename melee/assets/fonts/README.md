# Private menu-font inputs

`FolkPro-Medium.otf` and `FolkPro-Heavy.otf` were supplied by the user from
`FolkPro _Family.zip` on 2026-10-13. They are font assets, not project-authored
or open-source code. Do not infer redistribution rights from the repository's
code licenses.

`tools/prepare_menu_fonts.py` builds separate GX atlases for notices, the CSS
heading, CSS tile labels, results names and the results winner banner. The last
two reuse glyphs from the verified original disc. The heading uses the Windows
Impact font. CSS tile labels use the user-supplied `assets/ui/fonts/angular_glyph_set.svg`.
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
