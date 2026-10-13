# Pending common Catch contracts

Proposal only; parent owns shared interfaces. Source paths below are under
src/melee/ft/kinds/ftCommon. All notifications must validate source generation,
kind and Definition before a cached fighter/victim continuation.

| Proposed handler | Type | Exact stock insertion/replacement |
| --- | --- | --- |
| catch_allowed | Bool | ftCo_Catch.c input functions: after existing pickup attempt, replace fn800D8E94/952C tether veto pair; preserve ordinary button checks. |
| catch_start | Void | ftCo800D8C54 after accelerationXYZ0 and before motion212/214 change; reset ordinary counter0; keep common capture callbacks. |
| catch_frame | Bool | exclusive fn800D8EC8/9228 replacement in Catch/CatchDash Anim; true returns, false preserves common finished-frame decision; no later Samus native timing after override. |
| catch_exit | Void | fn800D8E30 before common Fall; remove present chain, but absent chain is a defined no-op. |
| catch_wait_release | Void | CatchWait_IASA immediately after successful throw/cut; throw true short-circuits cut; absent chain clears the known accessory/death bindings. |
| catch_pull_anchor | Bool | fn800D9CE8 replaces kind/anchor switch after start-frame choice, before throwflags0 and motion change; true preserves authored anchor, false selects common bone. |
| catch_pull_ready | Bool | exclusive whole CatchPull_Anim readiness switch; absent chain OR typed completed flag -> common CatchWait transition. |
| catch_pull_follow | Bool | fn800D9CE8 after motion change, before accessory1 assignment and x221B_b7 clear; false disables follow, true selects substantive common follow body fn800DA054. |

Stock fn800DA190 is no-op for Link/Young/Samus but would follow for a clone kind.
Do not keep that hidden stock-kind decision in an authored package. CatchWait's
later generic follow remains. These two release events are distinct: it802A2B10(NULL) does nothing; it802A7AAC with no chain clears callback slots. Naming/representation are subject to parent review.

Capture preparation is already expressible: ChangeMode(5,true), which clears
attack in source order, then endpoint.bind_capture_anchor. Publish the Bool bind
result into creator ordinary state and return it from catch_pull_anchor. Resource
backend accepts old Catch212/214 states and tracks exact known borrowers; no raw
endpoint pointer API is needed. Absent original Hookshot at CatchPull entry is
NULL-dereference UB; false/common-anchor fallback is a disclosed safe extension.

Existing typed completion refs, immutable-author signals, stage/bone/sound and
saved-item operations suffice. Saved held/auxiliary slots cannot replace the five
independent article references. No additional native solver/geometry API found.

## Composed lifecycle next slice

Stock ftLk800EAF58 orders Boomerang -> Hookshot -> Arrow -> Bow -> Milk.
RemoveBoomerang1 only clears used/ref when present; initialization OnDeath always
resets it and visual groups0/1/2. Death2/death3/death1 dispatch is a separate outer
context from initialization reset. Milk cleanup calls the removal inline, not the
similarly named availability predicate.

Assemble accepted authored article bodies under one exact immutable Definition.
Use typed refs + articles.signal(handle,"remove",0) for retained original-creator
handles, because articles.owned checks current damage owner. Boomerang and Milk
already provide remove signals. Add Arrow remove signal: destroy_trails BEFORE
item.remove/destructor. Add Bow remove signal. Hookshot Dispose unbinds exact
creator callbacks before resource release and item destruction. Milk removes its
owner before destruction. Bow exported recipient cleanup keeps the recipient's
actual module layout/parameters; absent optional export still permits removal.

Then compare actual generated article callbacks and linked resource/joint teardown
across all presence masks, transferred/reflected ownership, live/stale generations,
capture borrowers and replacement bindings. Reuse accepted source fixtures but do
not infer full composition from isolated matrices. Shared common dispatch cases
must test continuation removal/reuse/Definition changes at their exact sites.
No default promotion until composed lifecycle and common hooks pass.
