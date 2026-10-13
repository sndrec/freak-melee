# Paired composition: bounded remaining generic contracts

This is a resumed proposal after the verified SDK66 scalar slice. Existing
solo.fms, fixture.py, test.c, run.py and README.md remain unchanged. New scripts
are unregistered candidate components. The listed paired observation/relation
contracts are now available in frozen SDK68; paired_README.md records the
bounded comparison and the separate unfinished primary Belay/Rope foundation.
No family role query is needed for two distinct authored programs: index0/1 is
ordinary policy tuning.

## Existing API reuse

Cross-fighter position, velocity, acceleration, ground velocity/acceleration,
facing, animation timer/rate and root rotation X can be exported by the owning
script using existing zero-argument typed exports. Recruitment/range/termination
are recipient-owned Bool exports. `action.category`, model.instance_scale_y,
raw bone world_position, animation.set_timer/set_rate, song immunity, armor,
physics transitions, normal gravity/drift, boxed probes and all common entries
already exist. Avoid adding a redundant target accessor for every scalar.

## Remaining source-backed requirements

| Contract | Proposed script spelling / exact storage | Source use |
|---|---|---|
| Existing root foundation | family.member_id(index), full32 Int, absent0 | Player_GetEntityAtIndex throughout all four special files |
| Existing root foundation | hitlag.link(id), unlink(), request_release() | source/peer link publication and conditional x221A_b1 request |
| Observe actual current link | hitlag.linked_id Int RO | primary/secondary cleanup reads current x1A5C, which capture code may replace; family ID or saved variable is not equivalent |
| Existing root foundation | collision.copy_from(id), exact mpCopyCollData(src,dst,2) | ft_800849EC before ground recruitment and secondary Belay launch |
| Native hitlag observation | hitlag.active Bool RO, x2219_b5 | secondary follows primary animation rate unless primary in hitlag |
| Native inactive observation | fighter.incapacitated Bool RO, x221F_b3 | secondary Belay availability and thrown-state availability |
| Raw ground/air publication | airborne Bool RW, direct GroundOrAir assignment | secondary Squall collision writes primary ground_or_air AFTER optional motion change and collision copy; do not add D5D4/D7FC side effects |
| Live target bone world vector | targets.read_bone_position(id,raw_part,x,y,z) Bool, typed local outputs | primary R4thNb -> secondary XRotN follow, and secondary L4thNb endpoint push at exact accessory callsite |
| Spawn separation | spawn.horizontal_offset Float RW, Fighter.x40 | Popo/Nana OnLoad attrs0/C4; reset, Entry accessory and Rebirth positioning consume x40*scale.x |

The bone operation should reuse the real joint-position engine operation after
resolving full generation, kind/Definition, current parts and bounded raw part.
The implemented operation returns false and leaves the three outputs unchanged
for a missing/retiring/replaced target, without a native pointer escape. Candidate
callers initialize their local vector and branch on the Bool result. One read
evaluates ID/part first, validates three distinct
Float outputs and copies one world vector; no three-query emulation. Source
uses **raw** FtPart indices29 (L4thNb),47 (R4thNb),2 (XRotN), not a guessed
semantic map. These raw indices must be owned named parameters.

## Initial follower policy: exact order

ftCo_800A101C first selects mode6 and spacing15 for Nana (normal mode argument
and spacing40 otherwise), then initializes CPU common fields. Its early
xF9_b2=true assignment is overwritten by the unconditional xF9_b2=false reset.
Do not retain true as a final follower flag. History pointers select records5
and0; all30 records have zero inputs and copy the live primary position/facing.
Only later, after height/extent math and ftCo_800B9704, does on_cpu_initialize
run. A late scalar hook cannot seed this safely or select the earlier mode.

Root owns family/follower foundations. A reusable initialization policy must
choose early controller mode/history source before those loops, then retain
ordinary CPU shared mechanics. No opaque stock Nana initializer or synthetic
press replay is accepted. This namespace does not implement follower behavior.

## x1A5C retirement proof

Fighter_UnkInitReset_80067C98 advances x8_spawnNum and clears its own x1A5C,
but does not scan surviving peers. Fighter_Unload_8006DABC has no inbound-link
cleanup. ftColl_8007B8E8 scans only thrownHitbox.owner, ftColl_800765AC clears
hit-victim data, and Player_80031FB0 clears the player entity slot.

Squall cleanup conditionally clears both ends only if its bound callback is
invoked. CaptureCut and Throw clear the two known participants only. Thus
source-linked pointers can survive raw retirement or same-address reset/reuse.
Shared hitlag consumers Fighter_UnkRecursiveFunc_8006D044 (setBit before
recursion), Fighter_8006D10C_Inline2 and Fighter_8006D10C dereference the peer
without a generation check. A new authored-only marker cannot protect their
stock assignment sites. Root's all-assignment stamp plus registry-before-deref
validation is the appropriate universal storage contract.

Cleanup healthy ordering remains root X=0, source request_release, peer
request_release, peer unlink, source unlink. A stale peer is unavailable; never
release or unlink a newly recycled fighter. Safety differences are tested
separately from defined healthy stock comparisons.
