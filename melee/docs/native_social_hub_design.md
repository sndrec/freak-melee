# Social hub, personal roster and fighter sharing

Design direction revised with user feedback on 2026-10-15. The user approved
the connected-screen mockup and native integration on 2026-10-17. The approved
mockup remains the layout and flow reference during implementation.

The user clarified on 2026-10-17 that integration means adapting the game's
existing menus and assets. Use their authored geometry, textures, materials,
animations, scene structures and input behavior as the foundation. Reuse and
edit those elements to support the new flows. Drawing an imitation through GX
alone does not meet this requirement. Add new artwork or components where the
existing assets cannot express a feature. Detailed Workshop tools retain their
existing interface.

Every asset edit needs a specific base-game reference and a reason tied to
matching it. Compare the original asset, the proposed result and that reference
before accepting the edit. Preserve intentional shapes, colors, gradients,
border widths, proportions and decorative details. Adapt existing components
to the new functionality while retaining their visual language. An unusual
shape or a resemblance to a letter is not evidence of an artwork error.

The initial hub below uses custom procedural frames and controls. Its service
and flow checks remain valid, but do not establish this asset-integration
requirement. The stock hand, portraits and transition tracks are already reused.
Replacing the frame, navigation and page controls with stock menu components is
the current implementation priority.

Preserve the proportions of the left navigation ribbons' end caps while changing
their width. Stretch the middle section. Cut suitable baked artwork into
reusable corners, edges and centers. New lobby controls should match the stock
menus' colors, gradients and border widths; use source pieces where they suit
the component rather than forcing every control into a complete stock asset.
Retain intentional details in that artwork. In particular, the navigation
ribbon's interior shape resembles a P; preserve it as part of the stock border.
Discuss visual reinterpretations with the user before changing them.

The first adapted pass now uses the stock background, panel frame, camera,
lights and animated ribbons. Ribbon ends retain their proportions as the middle
changes width. A sliced stock panel border supplies hub panels, fields and
buttons. Shared border artwork now also supports CSS/practice controls,
fighter-delivery notices and the Quick Play countdown, independently of the
main-menu scene lifetime. Page captions use their recovered italic face through
the original caption joint, with its material opacity.
Hub controls and the hand now follow the visible stock frame's joint and camera,
including the original C-stick orbit. Mouse hit testing uses the same projected
surface, and page transitions compose with that view.
The [menu asset checkpoint](native_menu_assets_checkpoint.md) records
the source references, native checks and remaining typography/layout work.

## Native integration

The first implementation slice moves the hub pages into GX rendering and
connects the existing local collection, editable drafts, global chat, lobby
membership and fighter-delivery APIs. The detailed Workshop and binding editor
remain accessible from this hub. F1 or holding B returns to the hub from home
character select. Opening a fighter from this landing page returns directly
to it when the detailed editor closes.

First-run setup must preserve existing collections and saved CSS layouts.
When both are absent, save an empty CSS layout and offer a stock template.
The new clone flow prepares the copied behavior and parameters, installs the
result, and adds its portrait to an available CSS position. The independent
draft remains available for editing.

The hidden, muted `review_native_hub.py` run passed on 2026-10-17. Its isolated
installation exercised empty first launch, a playable Mario clone, CSS placement,
rename replacing the installed identity, an independent copy, opening and closing
the editor with CRLF source, profile saving and controller return from CSS.
It preserves the CRLF source file while displaying it with LF line endings.
The run and PNG screenshots are local under `.local/native-hub-acceptance-5`.
This check does not establish online matchmaking, offline inbox delivery or
the final controller feel on hardware.

The clone flow saves the proven hitbox-duration conversion before installation.
The shared script-template reader supplies owned behavior for fresh clones and
the editor. Directory publication retries brief Windows sharing/access failures
for at most 511 ms; existing destinations remain protected. Queued player
actions retain peer identities, and fighter delivery retains its package ID,
so changes to the displayed lists preserve the selected recipient and fighter.

Account sign-in, offline inbox storage and persistent friends remain separate
work. Quick Play queueing, assigned-room handoff, cancellable countdown and
fixed-rule matches now have local native checks. Result agreement and rematches
also work through the native six-stage page, with a pick and teammate confirmation
in doubles. Local mouse/controller checks cover shared-PC and separate-client
teams. User visual review and deployment remain open. Show their real
availability in the native menus. Browser sample players, messages and queue
results are examples, not native service evidence.

Global Chatroom now shows the live connected-player directory, including
players who have not posted. The service keeps names in WebSocket attachments
and broadcasts complete bounded snapshots on joins, leaves and renames. The
native menu supports 128 players with compact rows and paging. Local native
probes and the actual hidden, muted game verified this path; the service's
18 tests include hibernation and quiet-player name changes. Account identities
remain separate work; chat peer IDs identify a running client session.

Lobby chat uses the existing room connection and includes fighting, waiting and
spectating members. Its recent history survives server hibernation while the
room is occupied. Leaving the last connection clears that conversation. The
Lobbies page now pages through all 32 supported members, and lobby chat has its
own compact player list. See [chat](native_chat.md) for controls and evidence.

The native hub uses the actual CSS hand model, with its fingertip anchored
to the pointer position, and restores the scene camera after drawing it. The
practice scene now loads that hand independently from the CSS archive and
releases its own buffers on exit. Both the practice strip and full hub use the
same renderer; their hand survives the CSS-to-practice scene change.
The system mouse cursor hides only inside the active hub canvas. Starter clones use
portrait artwork extracted from the user's ISO during the build. The baked stock
name and border are removed; the native name bar uses the supplied angular glyph
set. Long names use two lines, while names such as Ganondorf remain on one line.
The lobby create/join page includes a one-to-four local-player selector.
The hidden, muted authoring acceptance run passed again with these changes;
screenshots are under `.local/native-hub-art-acceptance`.

CSS and practice now use native controls in the 4:3 game area. The CSS Ready
button follows the existing START path; practice offers lobby access, host start
and return to CSS. The hub also opens from paused practice with Z. Settings keeps
bindings, rules, costume selection and installed packages accessible as tools.
The [native play-menu checkpoint](native_play_menu_checkpoint.md) records the
source ownership and local acceptance. Quick Play queue controls and countdown
flow now have local native checks. The hub now uses the stock menu's authored
enter/exit animations, with forward/back, profile and scene-return checks in the
[play-menu checkpoint](native_play_menu_checkpoint.md). The GameCube adapter range/deadzone audit
below remains open for hardware verification.

The native CSS cookie path now separates portrait choice from cookie position.
A places a carried cookie anywhere in the viewport. Empty-space placement
retains its selected fighter. A near a placed cookie, or B, picks it up again.
Custom-fighter cookies draw through the stock cookie model, keep player/team
colors and remain at their placed positions. START requires every active slot
to have both a selected fighter and a placed cookie. Existing CPU controls,
costume controls and sliders retain their input paths.

Native CPU random selection samples the personal roster. This replaces the
retail loop that keeps looking for a stock portrait when no stock portraits
are present. CSS hands can reach the full viewport. The roster visibility mask
covers each portrait's whole hierarchy and restores its authored visibility
before menu animation updates, including for the six unlocked portrait groups.

The hidden, muted `review_native_css.py` run passed in
`.local/native-css-cookie-acceptance-3`: custom selection, free placement,
stable position, pickup, START rejection while carrying, opening a custom CPU
slot and actual practice entry. Reviewed PNGs show only the two installed
portraits, the free cookie and the resulting practice scene. The diagnostic
`ui-inspect css-selection` reports hands, cookies and current choices.

Online CSS cards now represent the lobby's active seats. Each client puts its
local seats first while retaining their physical controller ports and canonical
match ports. Waiting players and spectators stay out of the card row. The
presenter uses the stock panel backgrounds, emblems, doors and human/CPU badges.
Custom portraits also appear in offline player panels. Fighter choices publish
while players are on CSS; START still locks the choice and enters ready practice.

With the cookie placed, an empty hand can grab a fighter portrait. A drops it
onto a remote player card and opens delivery confirmation. X opens the recipient
chooser; B cancels. Mouse clicks and drags operate the same hand through the CSS
camera. Confirmation retains the fighter package and recipient identities.
Accepted fighters use the existing validated content transfer and roster install.
Receipt adds the saved CSS tile while the recipient is in ready practice too;
returning to CSS shows the new fighter. Repeated deliveries preserve one tile
and one installation. The local acceptance covers this complete path and role
changes that reconnect the active players.

The recipient chooser pages through the whole lobby, including waiting players
and spectators. Fighter delivery uses its own content connection. Accepting an
uncached fighter starts that connection; cached fighters install immediately.
The receiver can remain in practice while receiving the package. A local
five-client test verifies that content transfers preserve the four-player
gameplay connections. The two-client game test covers an uncached delivery to
a spectator in practice and the new roster tile after returning to CSS.

Selected remote fighters and costumes now download automatically while the
lobby is open. Their portraits refresh after the package is verified and cached,
before readying. Receiving a preview keeps the personal roster unchanged.
See the [lobby-cache checkpoint](native_lobby_cache_checkpoint.md) for the
transfer boundaries and native evidence.

The [native CSS card checkpoint](native_css_cards_checkpoint.md) records source
ownership, local tests and remaining transport and interface work.

## Personal roster

A new installation starts with an empty character-select screen. The first
prompt offers vanilla fighter templates. Choosing one creates an independent,
playable custom fighter and adds its portrait to the roster. The player can
rename it immediately or keep its original name. Editing its moves is optional.

Keep existing installations' rosters and installed characters. The empty roster
is the starting point for new installations. Players can add more fighters by
cloning templates or accepting deliveries. Keep the existing in-place CSS
layout editor for moving, resizing, adding and removing portraits.

Template choices use the existing supported cloning set. Kirby remains deferred.
Preserve paired-fighter editing and packaging for Zelda/Sheik and Ice Climbers.

An outfit package dresses the whole fighter family: Zelda and Sheik together,
or Popo and Nana together. Players cycle installed outfits with X/Y on CSS,
and the player card shows the outfit name. Costumes handles importing and
managing outfits. Accepting an outfit also installs its exact parent fighter.
Caching either package leaves the personal collection unchanged.

## Social hub

Use the old StarCraft Battle.net interface as a reference for the arrangement:
navigation on the left, conversation or selected content in the center, and
players in the current chatroom on the right. Show that player list only with
global or room chat. Friends, Inbox, Workshop and character select use the full
remaining area for their own content.

Use Melee's regular main, VS and Options menus as direct visual references,
alongside character select. Reproduce rounded colored outer frames, gold menu
ribbons, yellow selection highlights, translucent inner panels and contextual
typefaces. The mockup establishes the intended final appearance before native
implementation. Use compact game-menu controls throughout.

Visual references inspected for the revision:

- The running native home CSS, captured under
  `C:/FreakMelee-review/2026-10-15-hub-reference/screenshots`.
- [Main menu screenshot](https://davidvkimball.com/posts/super-smash-bros-melee-twitch-stream-overlay-tour/melee-main-menu.webp):
  gold ribbons, yellow selection, rounded frame and floating inner panel.
- [Upscaled VS menu reference](https://images.gamebanana.com/img/ss/mods/57c3416b73985.webp):
  red outer frame and contextual description strip.
- [Original language menu](https://ssb.wiki.gallery/images/4/44/Language_NTSC_SSBM.png):
  rounded gold frame and selected/unselected panels.

The local mockup embeds the supplied Folk fonts, the CSS tile glyphs generated
from `angular_glyph_set.svg`, and screenshot-derived portraits. Review each text
context against the original menus; the native font guide describes the faces.
Roster tiles are compact. Tile lettering fills the name bar vertically and
compresses horizontally to fit. Ganondorf stays on one line. Reserve taller,
multiline bars for genuinely long names such as Mr. Game & Watch.

Primary destinations:

- Channel: the shared conversation for everyone online.
- Quick Play: find random opponents for 1v1 or 2v2.
- Lobbies: create, join and return to multiplayer lobbies.
- Friends: presence, room invitations and access to writing letters.
- Inbox: letters, friend requests and lobby invitations, including those
  received while offline.
- Workshop: installed fighters, cloning, importing, and a prominent Edit fighter
  action. Keep editing visible as a central part of the client.
- Settings: a full-size main-menu destination for controls, audio, video and profile.

The hub replaces the default sidebars. The Workshop landing page presents the
personal roster and authoring commands. Its Edit fighter action opens the
existing detailed editor. Simple Settings pages use the game's menu presentation;
advanced authoring workspaces keep their dedicated interfaces. Chat and room
controls remain accessible during gameplay; the user explicitly deferred
match-time chat to a collaborative design pass on 2026-10-19. Keep the current
CSS/practice flow until that presentation is agreed.

Implement the new simple menus through the game renderer, with appropriate
fonts and graphics. Keep the existing 4:3 gameplay projection.

## Chat and letters

Global chat and room chat are separate shared conversations. Their player lists
show everyone in the relevant chatroom, including people outside the friends
list and spectators. Keep player rows compact; the revised mockup uses 29-pixel
rows at its desktop size. Selecting a player opens a full profile page with a
player-written bio, presence, writing a letter, adding a friend and inviting an
existing friend to a lobby. The signed-in player can edit their own bio.

Change pages with the rotating transition used by Melee's menus: outgoing
controls move off one side while incoming controls enter from the opposite
side. The original implementation in `mnmain.c` plays separate enter/exit joint
animations. Its four transition ranges are frames 320–339, 340–359, 360–379 and
380–399. The browser study approximates the motion over a third of a second;
native implementation now uses those authored joint tracks through HSD and GX.
Windows' client-area animation preference supplies the native default, with a
per-process override for testing. The mockup honors reduced-motion preferences.

Private correspondence uses email-style letters in the inbox. Each letter has
a sender, recipient, subject and body. Fighter transfers have their own flow
within a lobby.
Reply composes a new letter. Received and Sent views keep correspondence easy
to find. The supported chat types are global and room chat.

Players can address a letter to any player by name or friend code. Friendship,
current room membership and online presence do not restrict inbox recipients.
Friends have a room-invite command; invitations arrive in the recipient's
inbox with a Join room action. If the sender has no room, offer Create & invite.

The user's name is Twilight. Example participants and correspondence are
sample data; the old Discord excerpt's chess!! is a different participant.

## Controller navigation

Use the Melee hand as a pointer throughout the simple hub menus. The stick moves
the hand continuously; A activates the control underneath it and B goes back.
Keep hover feedback visible. The mouse can use the same hand cursor, with an
option in Settings. Text fields retain ordinary text entry. Detailed Workshop
editors keep their mouse and keyboard editing controls.

The hand's analog movement should use the original CSS response, with one input
normalization step. Account for elapsed time in the browser prototype. The native
input range/deadzone issue is recorded in the input audit below.

Portrait delivery supports picking up a tile, choosing a recipient and
confirming delivery with a controller. Mouse dragging performs the same action.
The recipient chooser includes other online members of the current lobby,
including spectators.

Use the keyboard for names, chat messages, friend codes and other text entry.
An on-screen typing keyboard is outside this scope. Controls around text fields
remain controller-accessible.

## Room and character select

Keep the existing choice to fight, wait or spectate. The host may override a
choice, and the player may change it again before the match starts.

Character-select cards represent the active match slots. Exclude waiting
players and spectators from fighter cards; keep them visible in the room's
people list. Preserve configured CPU slots and up to four local players.

Put local players first in the CSS presentation, beginning at P1. This ordering
applies only to character select. Keep the agreed match slots, labels and colors
consistent across clients during gameplay. Bind each card to its participant
identity and local seat, so changing its display order preserves input ownership.

Use the multiplayer CSS as the proposed foundation. The mockup includes an
original-layout reference and a lobby sharing version. Keep the portrait grid
and card arrangement as the baseline. A 1v1 lobby has two participant cards;
2v2 has four. Local practice shows its participating local seats.

The selection cookie can be placed anywhere in the viewport. Placing it over a
fighter chooses that fighter. Placing it elsewhere preserves the selected
fighter, if any. An empty hand can pick the cookie back up or carry a fighter
tile for sharing. Readying requires a chosen fighter.

Selecting a fighter and entering practice marks the player ready. The host's
Start match command leads to stage select with the chosen fighters locked in,
then the match. Preserve the existing practice room and match-exit flow.

## Fighter deliveries

Fighter transfers require both players to be in the same lobby. Place the CSS
selection cookie first; with an empty hand, pick up a fighter tile and carry or
drag it onto another player's card. Show the fighter and recipient in a dedicated
Send fighter confirmation. The Workshop landing page's Send fighter button opens this same
confirmation and lists eligible lobby members, including spectators.

Letters carry correspondence. Fighter transfers carry a selected fighter package.
Received fighters appear as deliveries with an Accept fighter action. Accepting
installs the fighter and adds it to the roster. The received fighter can be opened
as an editable copy through the Workshop's draft workflow. The current mockup
places received-fighter acceptance in Workshop; review its presentation with the
user before native implementation.

Associate a delivery with the exact package revision sent. The recipient keeps
their existing installed content and drafts until they choose an update or edit.
Downloading a fighter to participate in a match continues to populate the cache;
accepting a delivery adds it to the collection.

## Identity and offline delivery

Start with one signed-in social account per running client. Local guests can
still participate in matches. Friends, letters and inbox state belong to that
account.

Room peer IDs identify temporary connections. A verified connection also carries
the signed-in account's permanent ID and account name. Global chat retains a
small shared history with that public identity attached to each message.
Account identity remains separate from the room connection and local match seats.

Offline delivery stores letters, friend requests and invitations. Fighter
sharing uses the connected lobby's content transfer, validation, cache and
installation paths. Persistent accounts use generated Ed25519 key pairs, automatic
local sign-in, portable account-key files and recovery codes. Native account
screens, HTTPS handling and protected local storage are implemented. The service
stores public keys and hashes of random session tokens and recovery codes.
The private key stays on the client and can be exported to another PC. Windows
security-key prompts and user-chosen passwords are outside this flow.

Persistent friends and the email-style Inbox/Sent pages use those account
sessions. Global/lobby player lists open verified account profiles. Friends can
send expiring lobby invitations through Inbox; joining rechecks the sender's
membership and friendship. Signing out leaves the current lobby. The account-key
service is deployed and has native two-client checks. The user confirmed that
in-game account creation works on their PC. Standard file-picker interaction
and multi-PC acceptance remain separate checks. See the
[account guide](native_account_auth.md) and [friends and letters guide](native_social_accounts.md).

## Lobby browser

Lobbies lists named player-created lobbies for the running client build.
Creating a lobby includes its name, local player count and a Listed/Unlisted
choice. A listed lobby can be joined directly from its row. An unlisted lobby
can be joined using its code. The host can change listing visibility later.
Quick Play parties and assigned matches remain outside this directory.

Each row shows fighting seats, waiting seats, spectators and whether a match
is in progress. The service reads those values from the live room, including
hibernated connections. Closing the last connection unlists the lobby.
The directory displays eight rows per page, refreshes while open, and keeps
its network connection separate from gameplay signaling and global chat.
The initial private service bounds the directory to 512 listed rooms.

## Quick Play

Quick Play has separate 1v1 and 2v2 queues. Doubles accepts solo players and
premade pairs. A pair can use two local seats on one PC or friends on separate
PCs. Keep premade teammates on the same team. Pair solo players automatically.

The Quick Play landing page shows just the 1v1 and 2v2 choices. Choosing one
opens character select immediately. Its bottom cards show the local player and
one other slot for singles, or three other slots for doubles. Open slots have a
distinct waiting/searching appearance. Occupied cards show the human player,
connection status and readiness.

Queue admission and readiness are independent. Enter queue begins searching;
cancel stops the search. A player can choose a fighter and press START to ready
before joining the queue. Readying enters the practice room while waiting. Keep
queue controls and connection/readiness status accessible from practice.

When all required players have connected and are ready, begin a three-second
countdown. B unreadies the local player and cancels that countdown. A disconnect,
roster change, unready or loss of required content also cancels it. Ordinary B
outside the countdown remains the fighter's Special button in practice; retain
the normal pause/exit flow back to CSS. Start the match flow only after the
countdown completes with every condition still satisfied.

Both queues use four stocks, a six-minute timer and items off. Enforce these
rules without filling the landing page with rule text. Both modes use all six
stages: Battlefield, Final Destination,
Fountain of Dreams, Dream Land, Pokémon Stadium and Yoshi's Story.

Choose the first stage randomly. After each game, the losing side chooses the
next stage from those same six. Keep opponents together for rematches until
someone leaves. There is no fixed set length. In doubles, either losing teammate
selects a stage and the other confirms it. Changing the selection clears the
confirmation. Start the rematch once both teammates have agreed on that stage.

The service must allocate participants atomically, preserve pair membership,
enforce the fixed rules and stage-pick authority, and release queue entries on
cancellation or disconnect. Prevent duplicate searches and late matches after a
cancel. Check build and content compatibility before starting gameplay. Match
content downloads remain cached until explicitly accepted into the roster.

Local acceptance covers solo/pair queue assembly, cancellation, connection
failure, all six stages in both modes, first-stage selection, loser picks,
readiness before/after queueing, countdown cancellation, rematches, and
leaving/requeueing with a premade teammate. Physical multi-PC
testing remains a later user pass. The mockup's search results use local sample
participants. The local service now has party-preserving queue allocation,
private match admission and cancellation/recovery checks. Native queue controls,
ready countdown, fixed-rule matches and rematches now have local runtime evidence.
Separate-client premade teammates and four independent solo clients also finish
a four-stock game and rematch in the same assigned room in native local checks.
The native stage-confirmation page now passes local mouse/controller checks;
the user will review its presentation. Native cancellation checks cover the deadline race, opponent
exit and original-party recovery. Signaling-loss checks finish a viable P2P
match and requeue into a second match. See the
[Quick Play checkpoint](native_quick_play_checkpoint.md) and
[return checkpoint](native_quick_return_checkpoint.md).

## Native stick input audit

The user reported imprecise hand movement in the actual native game using a
GameCube controller and USB adapter. This is separate from the browser mockup.
The audit found the following source-level mismatches:

- Aurora's default controller deadzone and the default native profile both use
  8000 in SDL's 32767-unit range, removing roughly the first quarter of each axis.
- `aurora/lib/dolphin/pad/pad.cpp` divides SDL stick values by 256, producing a
  nominal 127-unit range. Remapped axes in `native_profile_input.c` do the same.
  Melee's `gmmain.c` configures an 80-unit clamp and scale, so this path can
  saturate well before full stick travel.
- [The SDL 3.4.10 GameCube driver](https://github.com/libsdl-org/SDL/blob/release-3.4.10/src/joystick/hidapi/SDL_hidapi_gamecube.c)
  maps an initial raw range of 128 ± 88 to signed 16-bit values and expands its
  observed limits. Conversion must account for that driver behavior.
- The original CSS uses a radial threshold of squared magnitude 200, followed
  by a squared-magnitude response and a 0.0002 position multiplier. Preserve
  that menu response after correcting the input conversion.

The native correction remains to be implemented and checked with the actual
controller. Capture center, small deflections, cardinal extremes and diagonals;
then normalize once, remove unintended extra filtering, and apply the same
conversion to default and remapped axes. Preserve explicit user calibration and
the original gameplay's thresholds. The browser response change does not fix
the native game.

## Mockup and implementation order

1. Review first launch, starter cloning, hub, lobbies, Quick Play, CSS and inbox
   as a connected interactive mockup. Compare the vanilla CSS and sharing views.
   Portrait and stage artwork is provisional.
2. Agree the in-game layout and controller navigation, including chat access
   during gameplay and the return path from the Workshop.
3. Add persistent accounts, friends, letters, room chat, room invitations and
   offline letter and invitation storage.
4. Build the hub and roster flow in the game renderer and connect existing room,
   practice, selection and content operations.
5. Connect portrait dragging, the Send fighter command, delivery acceptance and
   roster insertion. Recheck local shared-PC and spectator behavior.
6. Implement Quick Play queue admission, pair assembly, fixed rules, stage picks
   and rematches, with local simulated-peer coverage before multi-PC testing.

The mockup uses local sample messages and deliveries. It provides design
feedback; it does not create accounts, rooms or hosted deliveries.
