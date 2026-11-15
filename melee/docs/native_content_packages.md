# Outfit packages

The collection belongs to this installation. Choose a fighter on character
select, then use X/Y to cycle its installed outfits. The player card shows the
chosen outfit's name. The default choice uses the model included with the
fighter.

Open Settings > Costumes to import or choose outfits for local players. Select
a player, then Import DAT. A custom outfit belongs to that exact fighter
package. For paired custom fighters, the picker asks for each family member's
model in order. Zelda/Sheik and Popo/Nana each use one coordinated outfit.
Every model must match its member's skeleton. The import finishes after all
models pass validation. The first filename supplies the outfit name.

Stock character costumes use their original character and base color. Custom
outfits have a separate model variant value that scripts can read for cosmetic
choices. Selecting an outfit preserves the fighter's moves and parameters.

## Receiving and accepting

Packages are stored in `%LOCALAPPDATA%\Freak Melee\content`. Downloaded
packages enter `cache`. Explicit acceptance adds them to the installed
collection. A custom outfit requires its exact parent fighter package;
accepting it installs that cached parent too. Another clone with the same
skeleton does not satisfy this dependency.

The package ID covers the title, member mapping, model variant and complete
model data. Corrupt cached files are rejected. Repeated acceptance keeps one
installation. Failed outfit installation preserves a parent that was already
installed and removes a parent marker newly added by that failed attempt.

A room selection carries the fighter and outfit package IDs. Remote match
setup can load cached outfits without adding them to the local collection.
Accepting an outfit delivery fetches its missing parent fighter first, then the
outfit. Both are validated before installation. A newly received parent is
added to the roster. The sender reports delivery after installation is
confirmed. See the [delivery checkpoint](native_outfit_gift_checkpoint.md).
Current [room-match checks](native_outfit_match_checkpoint.md) cover cached
remote outfits, form changes, four shared-PC fighter slots, rollback and Results.

## Developer commands

Run `native_content_tool.exe` from the native build directory. Use `--root
DIRECTORY` before the command to select an isolated content store.

```powershell
.\native_content_tool.exe import-costume "C:\skins\PlMrNr.dat" Mario 0 "My Mario outfit"
.\native_content_tool.exe import-custom-costume "C:\skins\custom.dat" FIGHTER_ID "My outfit" 3
.\native_content_tool.exe import-custom-costume "C:\skins\paired-outfit" FIGHTER_ID "Coordinated outfit" 7
.\native_content_tool.exe list
.\native_content_tool.exe export PACKAGE_ID "C:\skins\outfit.fmc"
.\native_content_tool.exe cache "C:\skins\outfit.fmc"
.\native_content_tool.exe accept PACKAGE_ID
```

For a single-member custom fighter, import a DAT or a directory. Paired imports
use a directory containing `MEMBER_KEY.dat` for every member, such as `main.dat`
and `partner.dat`. `list` prints the parent fighter's member keys. The optional
last number is the model variant, from 0 to 255.

Import validates model symbols, skeleton size and material data. Cache verifies
the package structure and identity. Acceptance validates every model against the
cached parent before adding both packages to the collection. Imports and model
loading run at menu or scene transitions.

Custom outfits now carry a member directory. Rebuild earlier development custom
outfit packets from their DATs. The stock costume packet encoding is unchanged.
Fighter packages are unchanged.

The [coordinated outfit checkpoint](native_family_outfit_checkpoint.md) records
current local import, runtime and CSS-to-practice checks.

## Earlier checkpoints

The records below describe the builds and fixtures tested on their stated dates.

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
