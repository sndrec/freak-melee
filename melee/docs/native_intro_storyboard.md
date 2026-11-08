# Intro storyboard

Updated: 2026-11-08. First cut: **62 seconds**, 4:3, captions with music.
The user chose captions like the existing How to Play movie. The supplied
mode-select music is a temporary timing track. This study establishes
composition and order before filming.

The [shot recipe](../assets/intro/storyboard.json) drives the contact sheet and
timed animatic. Its actors are rendered by the native client from the original
models, costumes and named animations. Pending footage is labelled on the boards.
The first renders live in `.local/intro-storyboard-5/`.

## Shot order

| Time | Caption | Picture and camera |
| --- | --- | --- |
| 0–7 | In Freak Melee, your characters belong to you! | Mario stands in a quiet pool of light. Begin with a three-quarter full-body view; ease around toward his face. The carousel settles on Mario, then confirms the clone. |
| 7–13 | Tweak the hitboxes… | Match the Action Editor camera to a side view of Mario's forward tilt. Move the outer hitbox a little farther from the foot. Cut to the real attack reaching a sandbag it previously missed. |
| 13–19 | …the animations… | Hold Mario's forward-air pose. Show selected joints and the shared curve graph, change the pose, then match-cut to the altered move playing in the practice room. |
| 19–26 | …the code! | Show a short readable script change to a projectile's speed or launch angle. Cut to the result from a low three-quarter camera. Keep the example small enough to understand in one glance. |
| 26–33 | Show off your one-of-a-kind fighters online. | Film the actual CSS tile being dragged onto another player. Cut to their received fighter, then both characters facing each other. Use local test accounts for capture. |
| 33–40 | There aren't any limits. Against strangers, expect anything… | A ridiculously strong hit launches a low-percent opponent. Hard cut to a clothed Ganondorf taunting behind an oversized CENSORED bar. The joke works through the edit and bar; no nude asset is required. |
| 40–48 | You can disable custom fighter appearances in Settings. | Select the actual setting, return mid-taunt, then smoke replaces the censored fighter with a neutral hurtbox-shaped wireframe. Jigglypuff sings and the wireframe falls asleep. |
| 48–57 | Keep the moves. Change the look. Swag up your fighters! | Four different Falco outfits drop into a fixed wide shot on successive beats. Each lands and taunts. Hold all four together long enough to compare them. |
| 57–62 | Be prepared for anything in Freak Melee! | Hold the lineup, then resolve to the logo. Fade into the live first-fighter carousel, where the player takes control. |

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
  --profile .local/startup-feedback-3-ui-review/resume/appdata `
  --output .local/intro-storyboard-5 `
  --music A:/randomfiles/mode_select.flac
```

Output includes the native transparent PNGs, nine composed boards,
`storyboard.png`, `intro-storyboard.mp4` and a receipt identifying the source
recipe, executable and captured poses. These local disc-derived images stay
outside Git. The shot recipe and staging tool are tracked.

The native authoring channel accepts
`ui-inspect menu-shot:id,kind,costume,animation,frame,yaw,pitch,zoom,panX,panY`.
It captures a named animation from the disc to `menu-shot-ID.png` in that
client's working directory. Requests run only on the home selection scene,
outside startup and portrait editing. Normal startup navigation is unchanged.
