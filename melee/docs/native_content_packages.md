# Native costume packages (foundation)

The local content store is per installation, under
`%LOCALAPPDATA%\Freak Melee\content`. Its content-addressed `cache` directory
holds complete packages, while `installed` contains separate collection
markers. Receiving a package only caches it. An explicit acceptance installs
the marker without transferring the DAT again. A verified SHA-256 package ID
covers the costume metadata and the original DAT bytes, and files are staged
and atomically moved into place. A bad cache file is rejected and can be
repaired by receiving the valid package again.

At the home CSS, select a fighter and base color, open that player's COSTUMES
picker, and choose **Import DAT**. The Windows file picker accepts an
original-format `.dat`; the client validates it for that fighter and color,
uses the filename as its collection name, installs it, and selects it for that
player. Import runs only in the menu, before match simulation. The developer
tool also supports scripted import and package inspection:

```powershell
build/native/native_content_tool.exe import-costume "C:\skins\PlMrNr.dat" Mario 0 "My Mario skin"
build/native/native_content_tool.exe list
build/native/native_content_tool.exe export <package-id> "C:\skins\mario.fmc"
build/native/native_content_tool.exe cache "C:\skins\mario.fmc"
build/native/native_content_tool.exe accept <package-id>
```

`import-costume` validates the selected stock character and base costume's
required joint/material symbols through the same typed DAT importer used by
the native fighter loader. It then caches and installs the package. `cache`
preflights stock DATs but leaves packages non-selectable. A custom-fighter
costume can be cached before its parent arrives; its graph is checked when
accepted or selected;
`accept` preflights a cached package and installs it. The optional `--root
DIRECTORY` flag isolates a store for testing. `extract-stock ISO Mario 0
OUTPUT.dat` extracts a known original costume for authoring or diagnosis.
Names with spaces must be quoted.

An installed costume can now be selected for a direct-start match with
`--p1-skin <package-id>` through `--p4-skin <package-id>`. `--content-root
DIRECTORY` can point a test run at an isolated collection. The loader imports
the selected model and material animation once at startup, then resolves them
per fighter slot during model creation. The original stock costume index stays
in the fighter for gameplay-specific color logic. The replay signature includes
the package ID. A package must be installed and match the selected character
and base costume; cached-only local selections, mismatched packages, and raw
UDP netplay selections are refused. For room-signaled direct matches, the
proposal carries the costume package ID. Each peer can load the exact cached
package for rendering without installing it into their collection. During
room match setup, an absent selected costume is requested from its owning
peer over the room connection, verified against its package ID, and cached
before that peer marks the match ready.

The home character-select screen now shows compatible installed costume
packages for each local fighter. Choose a stock character and base costume,
then use COSTUMES in the right rail before pressing Start for practice. The
selection follows that player into practice and is published with the room
selection. When the owner starts a room match, stage select carries the exact
package ID into the canonical match; peers receive missing packages during
match setup. The picker refreshes its installed collection after an in-client
import; external imports made while the client is running appear on the next
CSS entry.
Installed custom fighter bundles also appear on the original CSS; their
package identities follow room selection and are cached automatically by
other match peers. The Collection menu lists installed fighter and costume
packages and lets a player offer one to another connected room member. The
recipient can decline or accept and install it. If the package is already in
their verified cache, acceptance installs it without transferring the bytes
again. Otherwise the room connection transfers it, verifies the package ID,
then installs it. An offer alone does not create a cache or installed entry.

The current preflight
verifies the named DAT model/material graph; it does not yet prove a custom
skeleton is fully compatible with every fighter animation or that a costume
changes no gameplay assets. Ice Climbers and other multi-asset cases need
explicit dependency handling before full selectable support.

The room-selection and canonical match packets now have separate full SHA-256
package identities for a fighter definition and its costume, alongside their
stable character and base-costume IDs. Stock selections remain compact in room
presence; custom IDs use explicit optional fields. The signaling service source
allows a complete four-player selection with both IDs. The home CSS publishes
selected installed costume and custom fighter identities. The online client
requires the exact cached fighter bundle and a matching native behavior
definition before the game begins. A costume package also requires the exact
cached package. This prevents an apparently agreed custom selection from
silently appearing as stock on another PC. Custom-fighter costumes now carry
the exact parent fighter package ID. Their DAT is checked against that
fighter's costume symbols and bone count rather than a stock character table.
An installed parent can receive an extra costume through
`import-custom-costume DAT FIGHTER_ID TITLE`. Recipients may cache the costume
first, but explicit installation requires the parent bundle in cache. A
costume for another clone is rejected even when both clones share Mario's
skeleton. Direct matches and room match assembly load the matching extra
costume after both packages are present. The original CSS picker does not yet
expose extra costumes for custom fighters; its UI remains for the collaborative
pass.

On 2026-09-23, the release build and all 357 serial CTests passed. A real
Mario costume DAT was extracted from the USA v1.02 ISO, imported into an
isolated host store, exported, cached in an isolated guest store (which listed
zero installed entries), then explicitly accepted there (which listed one).
The independent real-ISO test rejects the same DAT when marked as a Fox
costume. The runtime test now also loads that installed DAT for Mario, confirms
fighter-slot resolution, and rejects a wrong fighter, wrong base costume, and
cached-only local selection. A cached-only package resolves for online match
rendering without granting collection membership. A direct one-stock
Mario-versus-CPU Luigi match with the isolated package reached gameplay and drew
both fighters; its GX capture is
`screenshots/melee-1790138958-11156-0000.bmp`. Because the package contained
Mario's original DAT, this proves the load/draw path without a visible
before/after skin change. A second test joined two direct-start room clients,
with the package installed only in the host store and cached (not installed)
in the guest store. Both clients rendered the same Battlefield match through
the timed round and into sudden death; their GX captures are
`screenshots/melee-1790139449-71880-0000.bmp` and
`screenshots/melee-1790139449-19104-0000.bmp`. The clients reported thousands
of confirmed peer checksums with no reported mismatches. This confirms
room-signaled selection and cached rendering for an already present package.
Custom fighter selection was added after this costume-only checkpoint. A live
home-CSS test with an isolated installed stock Mario package exercised the
picker and entered practice with Mario and Sandbag; because its DAT is the
original Mario costume, this verifies the load path without a visible skin
difference. The release build and all 358 CTests pass. A second live test
launched two clients through the home CSS, room, practice, and stage-select
menus. The host selected the installed Mario package; the guest held the same
package in cache only and selected stock Luigi. Both rendered the Fourside
match with Mario and Luigi. The guest collection still listed zero installed
packages afterward. This verifies the menu-to-match package handoff with a
pre-cached peer; it does not verify automatic transfer or a visibly modified
costume.

A later two-client test started with that package installed only on the host
and an empty guest store. Both clients reached Fourside gameplay. The guest
received a byte-for-byte SHA-256 matching package in its cache and still had
zero installed packages. This verifies automatic peer transfer for a selected
costume, including the cache-only collection boundary. It does not yet test
visibly modified art, transfer through a TURN relay, or custom fighter code.
The in-client import command uses the same DAT validation and package/install
path as the developer tool; its real-DAT test rejects a wrong fighter and
installs the correct one. A live client test opened the Windows file picker
from an initially empty collection. Selecting Luigi's DAT for Mario displayed
an error and left the collection empty. Selecting Mario's original DAT next
installed and selected the package, then Start loaded Mario with that package
in the home practice room beside Sandbag. The installed collection listed one
entry. This proves the UI path with original art; it does not prove a visibly
modified costume.

During online match setup, the room sidebar shows progress while a required
costume is sent or received. A two-client test transferred a 4 MiB Mario test
costume from the host to a guest with an empty collection, showed the guest's
receiving percentage during stage select, and reached gameplay. The guest's
cache file matched the package ID's SHA-256 digest, while its installed
collection remained empty.

A two-process localhost gifting check offered the installed Rook fighter bundle
to a guest with an empty content root. Before acceptance, that root did not
exist. Accepting transferred a 1,801,640-byte cache file whose SHA-256 matched
package ID `4fc6fb3ec1bfda9ebd2d939142d3a7e26389794b883ae6ec2d0beabf31257cc3`
and created its separate installed marker. A second guest with only the cached
file accepted without a new download and gained an installed marker. A third
guest declined; its content root remained absent. A fresh run then declined
Rook, received a new offer for the same package, accepted it, and installed
the verified bundle. This checks the single-offer retry identity on the local
ICE path. Booting from that gifted guest root showed Rook in Add fighter;
placing its portrait, selecting it with the original cursor, and pressing
Start entered practice with Rook and Sandbag. A separate live CSS session
began with an empty content root, installed the same bundle externally while
CSS remained open, then showed Rook in Add fighter without leaving the screen.
The editor refreshes installed fighter portraits when opened.

A separate two-process localhost run offered a 473,598-byte Mario costume to
an empty guest store. Accepting created the cache and installed marker; the
cache SHA-256 matched package ID
`1d9fb26d02038326c192f8f74a21d2a3baff2fae786c26172bc368aa1c412bcd`.
A subsequent boot from that guest store selected Mario, offered the received
"Mario sample skin" in COSTUMES, selected it, and entered practice with Mario
and Sandbag. This package contains the original Mario art, so it proves the
gift-to-selection-to-load path without a visibly modified skin. The success
card now describes the costume picker, and that picker refreshes when a gift
installs during an open CSS session. Simultaneous offers and relay or
cross-network transfer remain untested. A fighter bundle may carry its
compiled native behavior module; the recipient validates it for this client
before adding the fighter to their collection.

On 2026-09-24, fighter-gift acceptance began validating the complete cached
bundle in detached storage before creating an installed marker. This includes
its embedded native behavior module when present; a matching built-in code key
alone is insufficient. In a rendered two-client room, the host offered a
compiled `mario_clone` bundle that the guest had cached but not installed.
Accepting installed it, and the guest collection listed one fighter. A second
run offered a package with the same metadata and a deliberately damaged DLL
header. The guest displayed "Could not install Clone Live" and still listed
zero installed entries. These were same-PC, pre-cached gift checks; the earlier
Rook transfer check covers the uncached transfer path. All 374 native tests pass.

The 2026-09-24 custom-costume backend pass kept the stock package encoding and
added a parent-bound costume type. A clone's separate original-format Mario
costume DAT was imported for installed compiled bundle
`0139e1db768d5be4967b5a4a9c6eb501c872ed1199809c96afc3f04ec08de4a0`
as costume `aa8cd554e14fa0fffbcfe403f4ea76725cb4073a51faf6a9133eceec59f3ec84`.
A direct practice launch rendered the clone and Sandbag with that package;
because the DAT contains Mario's original art, this proves load and draw, not
a visible custom appearance. Launching a second Mario clone with the first
clone's costume rejected the exact-parent mismatch before Aurora started.
In an isolated recipient store, the costume cached without its parent but
failed explicit acceptance. Caching and accepting the parent fighter first
then allowed costume acceptance. All 374 native tests pass. A room match with
automatic transfer of both custom packages and live CSS selection remain to
be exercised; online checks on another PC remain pending user testing.
