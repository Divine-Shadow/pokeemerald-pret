#!/usr/bin/env python3
"""Validate a generated PKCalc tracker overlay."""

from __future__ import annotations

import argparse
import json
import re
from collections import OrderedDict
from datetime import datetime
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TRACKER_DIR = REPO_ROOT / "build/tracker_export"
DEFAULT_OVERLAY_DIR = DEFAULT_TRACKER_DIR / "pkcalc_overlay"

EXPECTED_FILES = OrderedDict(
    [
        ("partyOrder", "js/data/party_order.js"),
        ("sets", "js/data/sets.js"),
        ("locations", "js/data/dex/locations.js"),
    ]
)
EXPECTED_CONSTANTS = {
    "partyOrder": "PARTY_ORDER_PK",
    "sets": "SETDEX_PK",
    "locations": "LOCATIONS",
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--tracker-dir",
        default=DEFAULT_TRACKER_DIR,
        type=Path,
        help="Directory containing tracker_data.json.",
    )
    parser.add_argument(
        "--overlay-dir",
        default=DEFAULT_OVERLAY_DIR,
        type=Path,
        help="Generated overlay directory to validate.",
    )
    args = parser.parse_args()

    tracker_dir = resolve_from_root(args.tracker_dir)
    overlay_dir = resolve_from_root(args.overlay_dir)
    tracker_data = load_json(tracker_dir / "tracker_data.json")
    validate_overlay(tracker_data, overlay_dir)

    print(f"Validated {relpath_or_abs(overlay_dir / 'manifest.json')}")
    print("Overlay checks passed: file paths, constants, manifest, counts, sources")


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def validate_overlay(tracker_data: OrderedDict, overlay_dir: Path) -> None:
    manifest = load_json(overlay_dir / "manifest.json")
    readme = overlay_dir / "README.txt"
    assert readme.exists(), "README.txt is missing"

    adapters = {
        name: load_js_constant(overlay_dir / file_path, EXPECTED_CONSTANTS[name])
        for name, file_path in EXPECTED_FILES.items()
    }

    assert adapters["partyOrder"] == tracker_data["partyOrder"], "party_order.js payload mismatch"
    assert adapters["sets"] == tracker_data["setdex"], "sets.js payload mismatch"
    assert adapters["locations"] == tracker_data["locations"], "locations.js payload mismatch"

    validate_manifest(manifest, tracker_data)
    readme_text = readme.read_text()
    for file_path in EXPECTED_FILES.values():
        assert file_path in readme_text, f"README.txt does not mention {file_path}"


def load_js_constant(path: Path, constant_name: str) -> object:
    text = path.read_text()
    match = re.fullmatch(rf"const {constant_name} = (.*);\n?", text, flags=re.DOTALL)
    if not match:
        raise AssertionError(f"{path} does not contain const {constant_name}")
    return json.loads(match.group(1), object_pairs_hook=OrderedDict)


def validate_manifest(manifest: OrderedDict, tracker_data: OrderedDict) -> None:
    assert manifest["schemaVersion"] == 1
    assert manifest["artifact"] == "pkcalc-overlay"
    datetime.fromisoformat(manifest["generatedAt"])
    assert isinstance(manifest["repo"]["commit"], str) and manifest["repo"]["commit"]
    assert isinstance(manifest["repo"]["dirty"], bool)
    assert manifest["sourceFiles"] == tracker_data["sources"]
    assert manifest["files"] == EXPECTED_FILES

    expected_counts = OrderedDict(
        [
            ("trainers", len(tracker_data["trainers"])),
            ("setdexSpecies", len(tracker_data["setdex"])),
            ("partyOrder", len(tracker_data["partyOrder"])),
            ("locations", len(tracker_data["locations"])),
            ("route101Encounters", len(tracker_data["locations"]["route101"]["encounters"])),
        ]
    )
    assert manifest["counts"] == expected_counts

    assert "trackerData" in manifest["inputs"]
    assert "adapters" in manifest["inputs"]
    for name in EXPECTED_FILES:
        assert name in manifest["inputs"]["adapters"]


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
