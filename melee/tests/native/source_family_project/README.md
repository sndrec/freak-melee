# Actual source family authoring

This test links the actual runtime library and opens a real ISO. It takes an
already prepared source draft with a portrait, leaving that input unchanged.
The first member retains its complete source and assets; the second has a
separate minimal independent script. All three family modes prepare, build,
parse, reopen and rebuild identical package bytes/content identity. Paired
drafts own separate DAT allocations and preserve member keys and source/resource
payloads. Invalid second-member source preserves all builder outputs and does
not publish either program.

The fixture also constructs raw stock Zelda/Sheik and Popo/Nana family drafts
from disc and verifies both independent members. It does not translate those
specials, activate their scripts, publish families, create actors, draw or play
the game. Those raw drafts deliberately have no built behavior source yet.

The companion CLI runner packages/verifies all three modes, checks both member
paths, diagnoses an invalid second-member package, and checks that an invalid
draft build adds no cache entry. It writes only new private outputs on A: and
launches console children without visible windows. No installed user content is
changed.

```powershell
python tests/native/source_family_project/run.py --exe build/native/native_source_family_project_test.exe --tool build/native/native_content_tool.exe --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --draft A:/FreakMelee-test-archives/source-rook-private-draft-root/draft --output A:/FreakMelee-test-archives/source-family-project-review
```

Current receipt: `A:/FreakMelee-test-archives/source-family-project-cli-root/receipt.json`.
The native fixture passes 106 checks, all three modes and two stock asset pairs;
the CLI checks pass. Input and executable hashes are recorded. The test is not a
claim that complete Zelda/Sheik or Popo/Nana source behavior runs yet.
