# Interpreted reaction observations and collision groups

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10. Ordinary fighter scripts execute through the typed
interpreter. Generated C remains test-reference infrastructure only.

## Delivered

- `damage.reaction_suppressed` calls the original common zero-knockback/
  reaction-threshold predicate. `damage.knockback_level` uses the original
  scale and three threshold comparisons, returning 0–3. Both observations
  require the current owned fighter/program/storage and common parameters;
  they do not consume damage, mutate the fighter or implement fighter policy.
- `articles.new_collision_group()` calls the native match-wide item counter
  allocator. It preserves all 32 bits in an Int word, including negative word
  representations beyond `INT32_MAX`, and retains native wraparound. It creates
  no article, allocates no memory and draws no RNG.
- Bowser's two obsolete `items.new_hit_group()` calls now use that single
  generic allocator, already named in Ness and Pikachu/Pichu sources. Removed
  the obsolete exporter branch and duplicate unused reference-host slot;
  rebuilt the reference executable and DLL with the current layout. No alias
  or compatibility entry remains.
- Updated user documentation with observation semantics, group allocation and
  an explicit distinction between these implemented reads and pending shared
  damage-entry operations.

## Evidence

- Hidden owned-capture interpreter cohort: **24,366 checks / 4,475 cases**.
  New cases compare the actual common reaction bodies across 512 scale,
  flag, stored-threshold and knockback combinations, including zero/negative
  values and threshold boundaries. Production reads and compiled helpers
  preserve the entire Fighter payload.
- Counter comparisons cover zero/current IDs, signed-word boundaries and
  wraparound, with exact next-counter state. Alternating helpers on two owned
  fighters share one counter. Invalid source identities, scenes, classes,
  missing common parameters, argument count and stock/unowned fighters reject
  without publishing results or advancing the counter.
- Existing combat cohort: **37,048 checks / 12,240 RNG pairs / 30 native body
  pairs**. Existing stage cohort: **22,000 checks / 1,970 cases / 1,758 native
  pairs / 750 VM calls / 114 negatives**. Their native-body extraction now
  supplies the newly referenced common predicates and allocator as well.
- Windows client, content tool, source family/package runtime, capture enclosure,
  reference script executable and DLL targets built. Reference script CTest passed.
- Six roster sources still prepare: Captain Falcon, Ganondorf, Marth, Roy, Luigi
  and Yoshi. DK advances from the damage observations to `damage.enter` at line
  439; Bowser advances from group allocation to `articles.launch_at_position`
  at line 315. Fox/Falco retain their metadata/owned-asset gates. Source checking
  changed no installed assets and opened no game window.

## Receipts and limits

- `A:/FreakMelee-test-archives/script-reaction-groups-clean-oct10-final/receipt.json`
  SHA256 `96df9281881e4dd1eaf6c51b12fc36e926f8e13d463456f9103a495f7cbc9a33`; 1,751 pins.
- `A:/FreakMelee-test-archives/script-reaction-groups-combat-clean-oct10-final/receipt.json`
  SHA256 `1a62e3b86b0f457ec811c71de82b45f76347bc22e8684b5bfe667d59a8989663`; 1,680 pins.
- `A:/FreakMelee-test-archives/script-reaction-groups-stage-clean-oct10-final/receipt.json`
  SHA256 `05ffc40a18ef864729efa780b27cd8f7fa46abe84c2bf8d54838c87e3daba2dc`; 1,758 pins.
- `A:/FreakMelee-test-archives/source-reaction-groups-clean-oct10-final/receipt.json`
  SHA256 `9a15d9d144e364950a2a33f2294424ce015a018a2c99f51645986868165be8fa`; 27 pins.

All pins were independently rehashed unchanged in
`A:/FreakMelee-test-archives/reaction-groups-independent-pins-oct10.json`.
The fixtures use actual common bodies with controlled actor assets and engine
leaves. This establishes observations/allocation and interpreter integration,
not full hit scheduling, reaction entry, fire-breath construction, rendered
fighter gameplay acceptance or compact rollback performance.
