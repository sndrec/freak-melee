# Intro storyboard

Updated: 2026-11-08. First cut: **71 seconds**, 4:3, captions with music.
The captions preserve the user's original wording, punctuation and order.
Keep that text verbatim. Adjust card breaks and timing for readability;
propose wording changes separately.
Card breaks and timing follow the existing How to Play movie. The supplied
mode-select music is a temporary timing track. This study establishes
composition and order before filming.

The [shot recipe](../assets/intro/storyboard.json) drives the contact sheet and
timed animatic. Its actors are rendered by the native client from the original
models, costumes and named animations. Pending footage is labelled on the boards.
The first renders live in `.local/intro-storyboard-6/`.

## Shot order

| Time | Captions | Picture and camera |
| --- | --- | --- |
| 0–19 | In Freak Melee, your characters belong to you!<br>Select one to create a clone of.<br>They'll start off like the original fighter...<br>but with the built-in workshop, you can edit them to your heart's content. | Mario turns toward camera. The familiar fighter becomes your first editable clone. |
| 19–22 | Tweak the hitboxes... | Match cut from the real Action Editor to the edited attack. Show a modest range change before the later overpowered gag. |
| 22–25 | tweak the animations... | Scrub an authored pose in Animation, then cut to that pose moving in-game. Keep the same view direction across the cut. |
| 25–28 | tweak the code! | Show a few readable lines in Scripting. Cut to a visibly altered special, followed by the fighter landing in the same composition. |
| 28–36 | When you play online, you can show off your one-of-a-kind fighters<br>to any other player, too. | Film the real tile-to-player delivery, then the received fighter appearing on the other player's CSS. |
| 36–44 | Be careful, though - there aren't any limits,<br>so when playing against strangers, expect anything... | A low-percent victim is launched by one absurdly strong hit. Smash cut to a taunting Ganondorf behind a deliberately excessive CENSORED bar. |
| 44–50 | Don't worry - you can disable custom fighter appearances in the settings. | Mid-taunt, a puff of smoke replaces the censored fighter with a neutral hurtbox-shaped wireframe. Jigglypuff sings; the wireframe falls asleep. |
| 50–66 | Of course, you don't have to make any tweaks.<br>But even if you just change your character's look, and not their moves...<br>other players will see that, too.<br>Swag up your fighters! | Four Falcos drop in one at a time, land on the beat, then taunt. Final film uses distinct outfits; stock colors establish spacing in this study. |
| 66–71 | Be prepared for anything in Freak Melee! | Hold the lineup; resolve to the Freak Melee logo. Fade into the live first-fighter carousel. |

The current boards use stock animations and stock Falco colors to establish
blocking. Gameplay shots need actual authored changes, recorded input and camera
passes. The outfit montage needs four distinct outfit packages. The wireframe
board marks its actor's location; its final appearance still needs designing.

## Camera and continuity

Use the original attract video's visual language: full-screen gameplay, clean
cuts, unusual camera positions and brief holds on readable poses. Keep the final
movie free of the Workshop's window chrome except when showing an edit.
Direct reference frames were decoded from the user's `MvHowto.mth` and viewed
at frames 0, 18, 100, 300, 800 and 1400. Its large white captions have dark
outlines and sit around the demonstrated action. Use that treatment and vary
placement with the action; the storyboard's stage remains a neutral blocking
surface until the gameplay camera passes are filmed.

For the edit-to-gameplay cuts, preserve facing, screen position and pose between
the editor and game. Start with a locked camera; add movement after the cut
reads clearly. Hide the match HUD for cinematic footage. Retain event particles
and hit sounds so the hits keep their weight.

The Falco shot uses four marks at 19%, 40%, 60% and 81% of frame width. Keep the
camera still, stagger the drops and save space above them for the final caption.
The current native captures establish silhouettes and outfit spacing; actual
drop, landing and taunt timing will come from the game.

## Recording gates

- Record the real Workshop controls and the changed fighter. Keep the example
  projects so every shown change can be opened and tried.
- Verify the full fighter-delivery path before recording its receiving shot.
- Implement and verify the appearance filter before presenting its Settings
  control in the movie. Until then, retain this beat as a planned board.
- Keep the CENSORED gag deliberately broad and covered throughout the shot.
- Choose actual Falco outfits before the final costume capture. Stock colors
  are temporary composition references.
- Choose final music and caption pacing together after reviewing this pass.

## Rebuilding the study

Build the native client, then run the staging tool with a **completed isolated
startup profile**. The tool copies that profile; it launches a hidden, muted
client and closes only that client. For example, from the repository root:

```powershell
python melee/tools/stage_native_intro.py `
  --exe melee/build/native/melee_native.exe `
  --iso 'Super Smash Bros. Melee (USA) (v1.02).iso' `
  --fonts melee/build/native/ui/fonts `
  --profile .local/startup-feedback-3-ui-review/resume/appdata `
  --output .local/intro-storyboard-6 `
  --music A:/randomfiles/mode_select.flac
```

Output includes the native transparent PNGs, seventeen caption cards,
`storyboard.png`, `intro-storyboard.mp4` and a receipt identifying the source
recipe, executable and captured poses. These local disc-derived images stay
outside Git. The shot recipe and staging tool are tracked.

The native authoring channel accepts
`ui-inspect menu-shot:id,kind,costume,animation,frame,yaw,pitch,zoom,panX,panY`.
It captures a named animation from the disc to `menu-shot-ID.png` in that
client's working directory. Requests run only on the home selection scene,
outside startup and portrait editing. Normal startup navigation is unchanged.
