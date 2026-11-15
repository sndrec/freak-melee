---
name: publish-build
description: Publish a coordinated Freak Melee build to GitHub Releases and the auto-update channel, with patch notes covering every change since the previous shipped build and an unpublished in-game News draft.
---

# Publish a Freak Melee build

Run this workflow when the user asks to publish a build. Defining, reviewing or
editing the workflow does not publish anything. A standalone friend ZIP is a
separate packaging task.

Read the [publishing procedure](../../../melee/docs/native_release.md#publish).
Use its stages in order, starting with the currently shipped build's exact
source revision. Inspect the complete commit range and final diff before writing
patch notes. The previous Git push is not the release baseline.

Write player-facing patch notes yourself from that inspection. Use the same
notes for the GitHub release and the initial News body. Avoid raw commit-log
output, implementation jargon, invented improvements and announcements of
changes that were already in the shipped build.

The outcome is:

1. A verified build, source tag and GitHub release with setup/update downloads,
   patch notes and a source/build record for the next release comparison.
2. One saved server-side News draft containing only the patch notes. Leave
   Published unchecked and publication time empty. The owner will add personal
   notes and publish it later. Do not create a game-wide announcement.
3. Verified R2 downloads and the new manifest live on the updater channel.

Use the publisher's `--stage-only` and `--activate-only` stages. Activation is
last, after the GitHub release and News draft are verified. Keep a local receipt
of completed stages so a retry can resume without duplicate articles or releases.
Preserve any personal edits made to an existing News draft.

Use the existing protected admin interface for News. Do not add an automation
login or broaden administrative access. Prefer a suitable connected GitHub tool;
otherwise use authenticated GitHub CLI or the browser. Do not extract browser
credentials. Missing authentication is a setup issue to resolve with the owner.

Build locally with the private ISO and optional music input. Package neither the
ISO nor user data. If an executable is locked, ask or wait for the owner to close
the game. Automated game launches are hidden and muted by default.

Report the release/download URLs, source revision, updater status, and the saved
News draft's title/link. Explicitly state that News remains unpublished. If a
stage fails, report exactly what succeeded and whether the live channel changed;
do not describe a partially published release as complete.
