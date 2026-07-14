#!/usr/bin/env python3
"""Create a drop-in PKCalc data overlay from generated tracker artifacts."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from collections import OrderedDict
from datetime import datetime, timezone
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TRACKER_DIR = REPO_ROOT / "build/tracker_export"
DEFAULT_OVERLAY_DIR = DEFAULT_TRACKER_DIR / "pkcalc_overlay"

OVERLAY_FILES = OrderedDict(
    [
        ("partyOrder", "js/data/party_order.js"),
        ("sets", "js/data/sets.js"),
        ("locations", "js/data/dex/locations.js"),
    ]
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input-dir",
        default=DEFAULT_TRACKER_DIR,
        type=Path,
        help="Directory containing tracker_data.json and pkcalc adapter files.",
    )
    parser.add_argument(
        "--output-dir",
        default=DEFAULT_OVERLAY_DIR,
        type=Path,
        help="Directory to receive the PKCalc path-matching overlay.",
    )
    args = parser.parse_args()

    input_dir = resolve_from_root(args.input_dir)
    output_dir = resolve_from_root(args.output_dir)
    tracker_data = load_json(input_dir / "tracker_data.json")

    build_overlay(input_dir, output_dir, tracker_data)
    print(f"Wrote {relpath_or_abs(output_dir)}")
    for file_path in OVERLAY_FILES.values():
        print(f"Wrote {relpath_or_abs(output_dir / file_path)}")
    print(f"Wrote {relpath_or_abs(output_dir / 'manifest.json')}")
    print(f"Wrote {relpath_or_abs(output_dir / 'README.txt')}")


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def build_overlay(input_dir: Path, output_dir: Path, tracker_data: OrderedDict) -> None:
    if output_dir.exists():
        shutil.rmtree(output_dir)

    copy_adapter(input_dir / "pkcalc/party_order.js", output_dir / OVERLAY_FILES["partyOrder"])
    copy_adapter(input_dir / "pkcalc/sets.js", output_dir / OVERLAY_FILES["sets"])
    copy_adapter(input_dir / "pkcalc/locations.js", output_dir / OVERLAY_FILES["locations"])
    write_json(output_dir / "manifest.json", make_manifest(input_dir, tracker_data))
    write_readme(output_dir / "README.txt")


def copy_adapter(source: Path, destination: Path) -> None:
    if not source.exists():
        raise FileNotFoundError(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def make_manifest(input_dir: Path, tracker_data: OrderedDict) -> OrderedDict:
    commit = git_output(["rev-parse", "HEAD"]) or "unknown"
    dirty = bool(git_output(["status", "--porcelain"]))

    return OrderedDict(
        [
            ("schemaVersion", 1),
            ("artifact", "pkcalc-overlay"),
            ("generatedAt", datetime.now(timezone.utc).replace(microsecond=0).isoformat()),
            (
                "repo",
                OrderedDict(
                    [
                        ("commit", commit),
                        ("dirty", dirty),
                    ]
                ),
            ),
            (
                "inputs",
                OrderedDict(
                    [
                        ("trackerData", relpath_or_abs(input_dir / "tracker_data.json")),
                        (
                            "adapters",
                            OrderedDict(
                                [
                                    ("partyOrder", relpath_or_abs(input_dir / "pkcalc/party_order.js")),
                                    ("sets", relpath_or_abs(input_dir / "pkcalc/sets.js")),
                                    ("locations", relpath_or_abs(input_dir / "pkcalc/locations.js")),
                                ]
                            ),
                        ),
                    ]
                ),
            ),
            ("sourceFiles", tracker_data["sources"]),
            (
                "files",
                OrderedDict((name, file_path) for name, file_path in OVERLAY_FILES.items()),
            ),
            (
                "counts",
                OrderedDict(
                    [
                        ("trainers", len(tracker_data["trainers"])),
                        ("setdexSpecies", len(tracker_data["setdex"])),
                        ("partyOrder", len(tracker_data["partyOrder"])),
                        ("locations", len(tracker_data["locations"])),
                        (
                            "route101Encounters",
                            len(tracker_data["locations"]["route101"]["encounters"]),
                        ),
                    ]
                ),
            ),
            (
                "postMvpGaps",
                tracker_data.get(
                    "postMvpGaps",
                    [
                        "Full species, move, item, and ability Dex exports are deferred.",
                        "Damage-calculator correctness data is deferred.",
                        "Lua/save sync and emulator integration are deferred.",
                    ],
                ),
            ),
        ]
    )


def git_output(args: list[str]) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=REPO_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    if result.returncode != 0:
        return ""
    return result.stdout.strip()


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


def write_readme(path: Path) -> None:
    text = """PKCalc Tracker Overlay

Copy this directory's contents over a PKCalc build root to replace only the static tracker data files:

  js/data/party_order.js
  js/data/sets.js
  js/data/dex/locations.js

The manifest.json file records the repository commit, dirty-worktree flag, source file list, artifact paths, and generated data counts.

This overlay contains static tracker data only. It does not include full Dex data, damage-calculator correctness data, Lua/save sync, emulator integration, or hosted UI support.
"""
    path.write_text(text)


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
