# Outfit dependency acceptance

Checked on 2026-10-17. Explicitly accepting a custom outfit installs its exact
cached parent fighter as well as the outfit. Cache downloads alone leave both
packages outside the installed collection.

The content store checks that the parent is a fighter package. Accepting callers
validate its source behavior and the outfit's model before installation. A
failed outfit marker write removes a parent marker added by that attempt and
preserves a parent that was already installed. Repeated acceptance preserves the
existing markers.

## Evidence

The `native_content_store` CTest passes. It covers missing dependencies,
acceptance, repeated acceptance and failed marker publication with both new and
previously installed parents.

`tools/review_native_costume_content.py` also passes against the native content
tool. It clones the received source-owned Fox fixture, replaces a costume
texture, builds a standalone outfit and uses separate author and guest stores.
It rejects a missing parent and another same-rig Fox package, then accepts the
exact cached parent. The guest receives exactly the two installed identities.
It repeats the marker failure checks and duplicate acceptance using real source
and model validation. Receipt, input hashes and command logs are in
`.local/native-costume-content-2/receipt.json`.

These checks cover cached dependency retention. They do not establish automatic
dependency transfer, coordinated paired outfits, CSS outfit cycling or match
rendering. Those remain part of the current content integration task.

The later [coordinated outfit checkpoint](native_family_outfit_checkpoint.md)
adds paired custom outfits, X/Y selection and native practice checks.
