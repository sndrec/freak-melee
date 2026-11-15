# Licensing and sources

Original software written for Freak Melee is available under [0BSD](LICENSE).
You may use, modify and redistribute that work for any purpose, including
commercial use, without an attribution requirement. The license includes a
warranty disclaimer. Its scope is the original contributions whose rights the
contributors can grant.

The repository also contains work from other sources:

| Work | Terms and source |
| --- | --- |
| Melee game and HSD code | Reconstructed from the original game by [doldecomp/melee](https://github.com/doldecomp/melee). The upstream project supplies no blanket software license for that game code. Our license grants no rights to Nintendo's code or assets. |
| Translated stock fighter scripts | Adapted from the corresponding stock game behavior. They have the same underlying provenance as the game code. |
| Aurora | MIT; see [aurora/LICENSE](aurora/LICENSE). Original changes written for Freak Melee use 0BSD; the upstream portions retain MIT. |
| cgltf | MIT; see [its license](melee/extern/cgltf/LICENSE). |
| CORE-MATH | MIT; see [its license](melee/extern/core-math/LICENSE). |
| Native runtime dependencies | Retained notices, FreeType credit, libjuice source link and version records are under [melee/licenses](melee/licenses) and included in prepared releases. That directory identifies the remaining prebuilt-component notice review. |
| Imported development tools | Their existing LICENSE and COPYING files remain beside their sources. The package locks record service tool dependencies. |
| Game textures, fonts, models and audio | Prepared from the player's ISO during the build. Each player supplies their own ISO. These assets are outside the original-code license. |
| Supplied startup music | The project owner confirmed distribution permission on 2026-11-14. Distributed through official builds; the audio remains a local build input outside Git. This permission does not relicense the track under 0BSD. |
| CSS name outlines | The project owner's hand-authored `angular_glyph_set.svg`; the original font design is preserved. |

[SOURCE_PROVENANCE.json](SOURCE_PROVENANCE.json) records the local upstream
base revisions. A public checkout supplies source access, rather than a grant
of rights over every work represented in that source.
