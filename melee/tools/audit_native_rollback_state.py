#!/usr/bin/env python3
"""Inventory writable PE contributions that may belong in rollback state.

This is a diagnostic for the Windows native build. It deliberately reports
linker ownership rather than deciding that every contribution from the game
library is safe to restore: native audio, replay, control, and diagnostics
state currently live in that library too.
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import pathlib
import re
import shutil
import subprocess
import sys


MODULE_RE = re.compile(r"^\s*Mod\s+(\d+)\s+\|\s+`([^`]*)`:")
OBJECT_RE = re.compile(r"^\s*Obj:\s+`([^`]*)`:")
CONTRIBUTION_RE = re.compile(
    r"^\s*SC\[([^]]+)\]\s+\|\s+mod\s+=\s+(\d+),\s+"
    r"(\d+):(\d+),\s+size\s+=\s+(\d+)"
)


@dataclasses.dataclass(frozen=True)
class Module:
    index: int
    name: str
    owner: str


@dataclasses.dataclass(frozen=True)
class Contribution:
    section_name: str
    section: int
    offset: int
    size: int
    module: int


def run_pdbutil(pdbutil: str, pdb: pathlib.Path, argument: str) -> str:
    command = [pdbutil, "dump", argument, str(pdb)]
    result = subprocess.run(
        command, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, encoding="utf-8", errors="replace"
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"{' '.join(command)} failed: {detail}")
    return result.stdout


def parse_modules(text: str) -> dict[int, Module]:
    modules: dict[int, Module] = {}
    pending: tuple[int, str] | None = None
    for line in text.splitlines():
        match = MODULE_RE.match(line)
        if match:
            pending = (int(match.group(1)), match.group(2))
            continue
        match = OBJECT_RE.match(line)
        if match and pending is not None:
            index, name = pending
            modules[index] = Module(index, name, match.group(1))
            pending = None
    return modules


def parse_contributions(text: str) -> list[Contribution]:
    result: list[Contribution] = []
    for line in text.splitlines():
        match = CONTRIBUTION_RE.match(line)
        if not match:
            continue
        size = int(match.group(5))
        if size == 0:
            continue
        result.append(
            Contribution(
                match.group(1),
                int(match.group(3)),
                int(match.group(4)),
                size,
                int(match.group(2)),
            )
        )
    return result


def short_owner(owner: str) -> str:
    if not owner:
        return "<direct>"
    return pathlib.PureWindowsPath(owner).name


def human_size(size: int) -> str:
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.2f} MiB"
    if size >= 1024:
        return f"{size / 1024:.1f} KiB"
    return f"{size} B"


def page_set(contributions: list[Contribution], page_size: int) -> set[tuple[int, int]]:
    pages: set[tuple[int, int]] = set()
    for contribution in contributions:
        first = contribution.offset // page_size
        last = (contribution.offset + contribution.size - 1) // page_size
        for page in range(first, last + 1):
            pages.add((contribution.section, page))
    return pages


def build_report(
    pdb: pathlib.Path,
    modules: dict[int, Module],
    contributions: list[Contribution],
    writable_sections: set[str],
    page_size: int,
) -> str:
    writable = [
        contribution for contribution in contributions
        if contribution.section_name in writable_sections
    ]
    unknown_modules = sorted({c.module for c in writable if c.module not in modules})
    if unknown_modules:
        raise RuntimeError(f"section contributions reference unknown modules: {unknown_modules}")

    by_owner: dict[str, list[Contribution]] = collections.defaultdict(list)
    by_module: dict[int, list[Contribution]] = collections.defaultdict(list)
    page_owners: dict[tuple[int, int], set[str]] = collections.defaultdict(set)
    for contribution in writable:
        module = modules[contribution.module]
        owner = short_owner(module.owner)
        by_owner[owner].append(contribution)
        by_module[contribution.module].append(contribution)
        first = contribution.offset // page_size
        last = (contribution.offset + contribution.size - 1) // page_size
        for page in range(first, last + 1):
            page_owners[(contribution.section, page)].add(owner)

    lines = [
        "# Native rollback writable-state inventory",
        "",
        f"Generated from `{pdb.name}` with `llvm-pdbutil`. This report is a diagnostic,",
        "not a snapshot manifest. A module's writable contribution may contain gameplay",
        "state, immutable initialized data, or native host handles that must remain live",
        "across rollback.",
        "",
        "## Summary by PE section",
        "",
        "| Section | Contribution bytes | 4 KiB pages touched | Modules |",
        "|---|---:|---:|---:|",
    ]
    section_rows = []
    for section_name in writable_sections:
        selected = [c for c in writable if c.section_name == section_name]
        if not selected:
            continue
        section_rows.append((sum(c.size for c in selected), section_name,
                             len(page_set(selected, page_size)),
                             len({c.module for c in selected})))
    for byte_count, section_name, pages, module_count in sorted(
        section_rows, reverse=True
    ):
        lines.append(
            f"| `{section_name}` | {human_size(byte_count)} | {pages} | {module_count} |"
        )
    lines.extend([
        "",
        "## Summary by linked owner",
        "",
        "| Owner | Contribution bytes | 4 KiB pages touched | Modules |",
        "|---|---:|---:|---:|",
    ])
    owner_rows = []
    for owner, owned in by_owner.items():
        module_count = len({c.module for c in owned})
        owner_rows.append(
            (sum(c.size for c in owned), owner, len(page_set(owned, page_size)), module_count)
        )
    for byte_count, owner, pages, module_count in sorted(owner_rows, reverse=True):
        lines.append(f"| `{owner}` | {human_size(byte_count)} | {pages} | {module_count} |")

    total_bytes = sum(c.size for c in writable)
    shared_pages = sum(1 for owners in page_owners.values() if len(owners) > 1)
    lines.extend([
        "",
        f"The selected writable sections contain {human_size(total_bytes)} across "
        f"{len(page_set(writable, page_size))} pages. {shared_pages} pages contain "
        "contributions from more than one linked owner. This is why page-aligning",
        "arbitrary executable globals is unsafe: restoring one owner can overwrite",
        "another owner that happens to share its page.",
        "",
        "## melee_runtime.lib modules",
        "",
        "| Object module | Contribution bytes | 4 KiB pages touched | Ranges |",
        "|---|---:|---:|---:|",
    ])
    rows = []
    for index, owned in by_module.items():
        module = modules[index]
        if short_owner(module.owner).lower() != "melee_runtime.lib":
            continue
        rows.append((sum(c.size for c in owned), module.name, len(page_set(owned, page_size)), len(owned)))
    for byte_count, name, pages, ranges in sorted(rows, reverse=True):
        lines.append(f"| `{name}` | {human_size(byte_count)} | {pages} | {ranges} |")

    lines.extend([
        "",
        "## Interpretation",
        "",
        "`.mstate` is the explicit candidate boundary for mutable game/HSD globals.",
        "`.maudio` contains original synth/driver and native mixer state that advances",
        "with the live audio timeline and is deliberately not restored. The remaining",
        "`melee_runtime.lib` contributions in `.data` belong to sources excluded because",
        "they own native audio output, replay files, debug control, screenshots,",
        "asynchronous I/O, logging callbacks, or diagnostic state. This",
        "report should be reviewed whenever a new source gains a host resource or is",
        "added to the exclusion list.",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdb", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--pdbutil", default=shutil.which("llvm-pdbutil"))
    parser.add_argument("--page-size", type=int, default=4096)
    parser.add_argument(
        "--section", action="append", dest="sections", default=None,
        help=("writable section name to include; defaults to .data, .bss, "
              ".mstate, and .maudio"),
    )
    args = parser.parse_args()
    if args.pdbutil is None:
        parser.error("llvm-pdbutil was not found on PATH; pass --pdbutil")
    if not args.pdb.is_file():
        parser.error(f"PDB does not exist: {args.pdb}")
    if args.page_size <= 0 or args.page_size & (args.page_size - 1):
        parser.error("--page-size must be a positive power of two")

    modules = parse_modules(run_pdbutil(args.pdbutil, args.pdb, "--modules"))
    contributions = parse_contributions(
        run_pdbutil(args.pdbutil, args.pdb, "--section-contribs")
    )
    report = build_report(
        args.pdb, modules, contributions,
        set(args.sections or [".data", ".bss", ".mstate", ".maudio"]),
        args.page_size,
    )
    if args.output is None:
        print(report)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
