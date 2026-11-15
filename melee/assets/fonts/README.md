# Native menu fonts

`tools/prepare_menu_fonts.py` creates the seven GX font atlases from a verified
USA v1.02 Melee ISO and the supplied CSS tile outlines. Build inputs are the
player's ISO, Python and Pillow. No installed desktop font is required.

| Face | Source |
| --- | --- |
| Notice and body text | The executable's original HSD glyph textures and proportional bearings |
| CSS heading | Capitals from the same HSD text face |
| Menu buttons | English `MnMaAll` option-name textures |
| Italic page headings | `MnMaAll` panel-caption textures, with their shear removed during extraction |
| Results names and winner banner | Separate `GmRst` lettering textures |
| CSS tile names | The user-authored `assets/ui/fonts/angular_glyph_set.svg` |

The native renderer reapplies the page-heading shear. Menu buttons preserve
their outlined face. Each recovered alphabet keeps its own letters, baseline
and proportions. Missing letters, punctuation and accented forms are built from
that face's strokes; the original HSD face supplies additional ASCII symbols.
The CSS outline sheet contains capital letters, digits and punctuation. Its
shared baseline and counters are preserved while rasterizing.

The atlases support printable ASCII, accented Latin letters and common
punctuation. Name and tile faces use uppercase forms. Common precomposed and
combining forms, such as accented letters and their decomposed forms, select
the same glyph. Unsupported characters display a question mark. Full
international shaping and right-to-left layout remain future work.

Generated `.bin` atlases and PNG inspection sheets belong in the build output.
The packed data contains code points, glyph placement, spacing, vertical
offsets and composition pairs. The native loader validates it once when fonts
are acquired. Drawing uses fixed lookup tables and resident texture data.

The intro-film tools draw these same prepared atlases with `menu_font_image.py`.
Pass the build's `ui/fonts` directory using `--fonts` when staging or assembling
the film. Their source text and edit recipes remain editable.
