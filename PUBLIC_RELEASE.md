# Public source review

Reviewed 2026-11-14. The GitHub source repository and playtest downloads are
public. New checkouts need the private build inputs described in the README.

## Source and assets

- Original Freak Melee software contributions use 0BSD. Existing upstream work
  keeps its own rights and terms; see [licensing and sources](LICENSING.md).
- Native text atlases are generated from the player's ISO and the owner's
  hand-authored CSS glyph outlines. Desktop OTF files are excluded from Git.
- The supplied startup music is an ignored local build input. The owner
  confirmed distribution permission. Official updater builds can package it;
  source builds without it play the original ISO menu track.
- A fresh public clone confirms that published branch and tag history contains
  neither the former OTF inputs nor the startup HPS. Old pull-request references
  and cached views require GitHub action. A support request for the retained PR
  reference was submitted.
- Commit attribution, maintainer email addresses and historical local paths
  are retained at the owner's request. Their presence is intentional.

## Hosted services

Playtest download URLs are public distribution endpoints. The channel path
does not authenticate a downloader. The owner chose public downloads; changing
that path would not protect an installed client containing the same URL.
Publication uses the maintainer's Cloudflare credentials separately.

TURN credential requests require HTTPS outside localhost. The service limits
issuance per network address and across addresses within each Cloudflare
location before calling the paid provider. Guests can continue playing online.
These approximate limits reduce request abuse. They do not cap relay bandwidth
or total spending, and an issued credential can be shared during its lifetime.
See [the service guide](melee/services/signaling/README.md#nat-traversal).

Long-lived service credentials stay in Worker secrets. Local account keys,
recovery codes, development environment files and build output are ignored.
The published-history secret scan found two false positives and no confirmed
credentials. That result covers the scanned revisions, rather than future
commits or other people's clones.

## Distribution

Prepared packages include the original-code license, source-scope explanation,
and all files under `melee/licenses`, including dependency notices, version
records, the FreeType credit and a source link for the MPL-covered libjuice.
Aurora, cgltf and CORE-MATH notices are copied from their source directories.
Review the complete transitive notice set before a general binary release;
the prebuilt graphics and disc libraries and their bundled shader compiler
need their bundled-component notices carried forward as well.

The original game's code and assets have separate ownership. Publishing this
repository under a license for our original work grants no additional rights
over that upstream material. Users supply their own game ISO and DSP table.

This source cleanup does not publish a new updater build.
