# Social hub, personal roster and fighter sharing

Design direction revised with user feedback on 2026-10-15. The first
deliverable is an interactive mockup of the connected screens. Native
implementation follows layout and flow review with the user.

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

## Social hub

Use the old StarCraft Battle.net interface as a reference for the arrangement:
navigation on the left, conversation or selected content in the center, and
players in the current chatroom on the right. Show that player list only with
global or room chat. Friends, Inbox, Roster and character select use the full
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
- My roster: installed fighters and access to character select and layout editing.

The hub replaces the default sidebars. Workshop and Settings continue to open
their dedicated, larger interfaces. Chat and room controls remain accessible
during gameplay; review that presentation separately from the hub layout.

Implement the new simple menus through the game renderer, with appropriate
fonts and graphics. Keep the existing 4:3 gameplay projection.

## Chat and letters

Global chat and room chat are separate shared conversations. Their player lists
show everyone in the relevant chatroom, including people outside the friends
list and spectators. Selecting a player gives access to writing a letter,
adding a friend or inviting an existing friend to a lobby.

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

Every screen and ordinary action must be usable with a controller. Use clear
selection feedback and consistent Confirm/Back actions. The CSS hand can serve
as a pointer where that suits the original menu. Keep the selected control
visible when changing screens or opening a dialog.

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

Selecting a fighter and entering practice marks the player ready. The host's
Start match command leads to stage select with the chosen fighters locked in,
then the match. Preserve the existing practice room and match-exit flow.

## Fighter deliveries

Fighter transfers require both players to be in the same lobby. Place the CSS
selection cookie first; with an empty hand, pick up a fighter tile and carry or
drag it onto another player's card. Show the fighter and recipient in a dedicated
Send fighter confirmation. The Roster's Send fighter button opens this same
confirmation and lists eligible lobby members, including spectators.

Letters carry correspondence. Fighter transfers carry a selected fighter package.
Received fighters appear as deliveries with an Accept fighter action. Accepting
installs the fighter and adds it to the roster. The received fighter can be opened
as an editable copy through the Workshop's draft workflow. The current mockup
places received-fighter acceptance in Roster; review its presentation with the
user before native implementation.

Associate a delivery with the exact package revision sent. The recipient keeps
their existing installed content and drafts until they choose an update or edit.
Downloading a fighter to participate in a match continues to populate the cache;
accepting a delivery adds it to the collection.

## Identity and offline delivery

Start with one signed-in social account per running client. Local guests can
still participate in matches. Friends, letters and inbox state belong to that
account.

The current room peer IDs are temporary, and the current global chat retains a
small shared history. Add persistent account identity, friend relationships,
letters, room chat, unread state and delivery records. Keep account identity
separate from a room connection and its local match seats.

Offline delivery stores letters, friend requests and invitations. Fighter
sharing uses the connected lobby's content transfer, validation, cache and
installation paths. Sign-in, recovery, storage limits and update presentation
need concrete design before service implementation.

## Quick Play

Quick Play has separate 1v1 and 2v2 queues. Doubles accepts solo players and
premade pairs. A pair can use two local seats on one PC or friends on separate
PCs. Keep premade teammates on the same team. Pair solo players automatically.

Both queues use four stocks, a six-minute timer and items off. Quick Play rules
are fixed. Both modes use all six stages: Battlefield, Final Destination,
Fountain of Dreams, Dream Land, Pokémon Stadium and Yoshi's Story.

Choose the first stage randomly. After each game, the losing side chooses the
next stage from those same six. Keep opponents together for rematches until
someone leaves. There is no fixed set length. The doubles stage-pick interface
must let the losing team agree on one choice; review its confirmation rule.

The service must allocate participants atomically, preserve pair membership,
enforce the fixed rules and stage-pick authority, and release queue entries on
cancellation or disconnect. Prevent duplicate searches and late matches after a
cancel. Check build and content compatibility before starting gameplay. Match
content downloads remain cached until explicitly accepted into the roster.

Local acceptance covers solo/pair queue assembly, cancellation, connection
failure, all six stages in both modes, first-stage selection, loser picks,
rematches, and leaving/requeueing with a premade teammate. Physical multi-PC
testing remains a later user pass. The mockup's search results use local sample
participants; a matchmaking service is still to be implemented.

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
