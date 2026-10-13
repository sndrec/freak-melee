# Effect hitlag ownership cohort

Actual source compilation, Fighter preparation/publication/Attach, Execution/VM,
receiver scopes, saved callback IDs and fixed GObj allocation/free are linked.
Actual efLib pause/resume/destroy loops and the common clear body operate on native
effect records in both effect lists. Same-owner records change; foreign-owner
records remain intact. Callback installation and replacement use real actor APIs.
Saved callback IDs restore only with the matching native callback slots restored
as peers. A retained foreign physical Fighter recipient can pause its effects;
unattached callback installation and a freed/reused recipient token reject.

The selected source package, native EF records, effect scheduling, graph walking
and model-effect cancellation callback are controlled. Cancellation failure
injection invokes actual authored helpers or retires the real source; it supplies
no effect operation result. Unrelated combat/native leaves abort or reject.
No renderer, native model-effect cancellation, full Climbers, or game proof.

The historical script_combat fixture retains its non-effects comparisons and now
uses real fixed MatchPool identity for its controlled physical Fighter. Its
unattached pause_during_hitlag case rejects; positive source callback ownership
is covered here with actual actors rather than the historical callback stub.
