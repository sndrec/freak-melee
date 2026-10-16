# Social hub, personal roster and fighter sharing

Design direction agreed on 2026-10-15. The first deliverable is an interactive
mockup of the connected screens. Native implementation follows layout and flow
review with the user.

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
people on the right. Use Melee's visual language, typefaces, selection effects
and menu rendering.

Primary destinations:

- Channel: the shared conversation for everyone online.
- Rooms: create, join and return to multiplayer rooms.
- Friends: presence, direct conversations and fighter deliveries.
- Inbox: messages, friend requests and fighter deliveries received while offline.
- My roster: installed fighters and access to character select and layout editing.

The hub replaces the default sidebars. Workshop and Settings continue to open
their dedicated, larger interfaces. Chat and room controls remain accessible
during gameplay; review that presentation separately from the hub layout.

Implement the new simple menus through the game renderer, with appropriate
fonts and graphics. Keep the existing 4:3 gameplay projection.

## Controller navigation

Every screen and ordinary action must be usable with a controller. Use clear
selection feedback and consistent Confirm/Back actions. The CSS hand can serve
as a pointer where that suits the original menu. Keep the selected control
visible when changing screens or opening a dialog.

Portrait delivery supports picking up a tile, choosing a recipient and
confirming delivery with a controller. Mouse dragging remains an additional
way to perform the same action. Include offline friends and spectators in
the recipient chooser.

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

Use the multiplayer CSS as the proposed foundation, with smaller participant
cards where needed. Review its layout in the mockup before committing to that
choice over a modified single-player CSS.

Selecting a fighter and entering practice marks the player ready. The host's
Start match command leads to stage select with the chosen fighters locked in,
then the match. Preserve the existing practice room and match-exit flow.

## Fighter deliveries

Drag a fighter portrait onto a person or mailbox to compose a delivery. Show
the recipient, fighter and optional message before sending. Provide the same
action through a controller-accessible Send fighter command.

Mailboxes are available for friends and room members, including spectators.
Support sending to offline friends. Store the delivery and its fighter package
on the hosted service until the recipient retrieves it.

The inbox shows the sender, message and fighter. Accepting installs the fighter
and offers to add it to the roster in the same flow. The first mockup combines
these into Accept & add to roster. A received fighter can be opened as an
editable copy through the Workshop's draft workflow.

Associate a delivery with the exact package revision sent. The recipient keeps
their existing installed content and drafts until they choose an update or edit.
Downloading a fighter to participate in a match continues to populate the cache;
accepting a delivery adds it to the collection.

## Identity and offline delivery

Start with one signed-in social account per running client. Local guests can
still participate in matches. Friends, messages, inbox state and deliveries
belong to that account.

The current room peer IDs are temporary, and the current global chat retains a
small shared history. Add persistent account identity, friend relationships,
direct conversations, unread state and delivery records. Keep account identity
separate from a room connection and its local match seats.

Offline fighter delivery needs hosted package storage as well as inbox metadata.
Reuse the current package validation, cache and installation paths when a
delivery is downloaded. Sign-in, recovery, storage limits and update presentation
need concrete design before service implementation.

## Mockup and implementation order

1. Review first launch, starter cloning, hub, rooms, CSS and inbox as a connected
   interactive mockup. Portrait artwork is provisional.
2. Agree the in-game layout and controller navigation, including chat access
   during gameplay and the return path from the Workshop.
3. Add persistent accounts, friends, direct messages and offline delivery storage.
4. Build the hub and roster flow in the game renderer and connect existing room,
   practice, selection and content operations.
5. Connect portrait dragging, the Send fighter command, inbox acceptance and
   roster insertion. Recheck local shared-PC and spectator behavior.

The mockup uses local sample messages and deliveries. It provides design
feedback; it does not create accounts, rooms or hosted deliveries.
