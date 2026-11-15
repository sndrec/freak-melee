# Dependency notices

Prepared releases include every file in this directory. `sources.json` records
the source versions and SHA-256 hashes of the notices added during the public
source review. Update those notices when changing their corresponding dependency.
Aurora, cgltf and CORE-MATH notices are also copied from their source directories.

Portions of this software are copyright © 2026 The FreeType Project
(https://freetype.org). All rights reserved. This distribution uses the FreeType
License in `freetype.txt`; its license overview is in `freetype-licenses.txt`.
Zstandard uses the BSD terms in `zstd.txt`. Nod uses the MIT terms in `nod.txt`.

libjuice uses Mozilla Public License 2.0 (`libjuice.txt`). Its unmodified source
for the pinned build is available at:
https://github.com/paullouisageneau/libjuice/tree/3c40a3545b6b1b62c7adee7f8f2bd58aa290afd6
and as a source archive at:
https://github.com/paullouisageneau/libjuice/archive/3c40a3545b6b1b62c7adee7f8f2bd58aa290afd6.tar.gz
The build's configuration is in `melee/CMakeLists.txt` in the public Freak Melee
source repository. Future changes to MPL-covered files must remain available in
source form with that release.

The Dawn and Nod notices here cover those projects themselves. Their prebuilt
packages contain additional components. A complete notice/source inventory of
those binaries, and their bundled DirectX shader compiler, remains part of the
general binary-release review. These files do not claim to be a complete
transitive notice set for those prebuilt artifacts.
