#!/usr/bin/env python3
"""Validate generated static tracker export artifacts."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
EXPORTER = REPO_ROOT / "tools/tracker_export/export_tracker_data.py"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Existing or generated export directory. Defaults to a temporary directory.",
    )
    args = parser.parse_args()

    if args.output_dir:
        output_dir = args.output_dir
        if not output_dir.is_absolute():
            output_dir = REPO_ROOT / output_dir
        run_exporter(output_dir)
        validate_output(output_dir)
    else:
        with tempfile.TemporaryDirectory(prefix="tracker-export-") as tmp:
            output_dir = Path(tmp)
            run_exporter(output_dir)
            validate_output(output_dir)


def run_exporter(output_dir: Path) -> None:
    subprocess.run(
        [sys.executable, str(EXPORTER), "--output-dir", str(output_dir)],
        cwd=REPO_ROOT,
        check=True,
    )


def validate_output(output_dir: Path) -> None:
    tracker_path = output_dir / "tracker_data.json"
    tracker_data = json.loads(tracker_path.read_text())

    setdex = load_js_constant(output_dir / "pkcalc/sets.js", "SETDEX_PK")
    party_order = load_js_constant(output_dir / "pkcalc/party_order.js", "PARTY_ORDER_PK")
    locations = load_js_constant(output_dir / "pkcalc/locations.js", "LOCATIONS")

    assert setdex == tracker_data["setdex"], "SETDEX_PK does not match tracker_data.json"
    assert party_order == tracker_data["partyOrder"], (
        "PARTY_ORDER_PK does not match tracker_data.json"
    )
    assert locations == tracker_data["locations"], "LOCATIONS does not match tracker_data.json"

    validate_sawyer(tracker_data)
    validate_route101(tracker_data)

    print(f"Validated {relative_or_abs(tracker_path)}")
    print("Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords")


def load_js_constant(path: Path, constant_name: str) -> object:
    text = path.read_text()
    match = re.fullmatch(rf"const {constant_name} = (.*);\n?", text, flags=re.DOTALL)
    if not match:
        raise AssertionError(f"{path} does not contain const {constant_name}")
    return json.loads(match.group(1))


def validate_sawyer(tracker_data: dict) -> None:
    trainer = tracker_data["trainers"]["TRAINER_SAWYER_1"]
    label = trainer["label"]
    assert tracker_data["partyOrder"][label] == ["Geodude"]

    geodude = tracker_data["setdex"]["Geodude"][label]
    assert geodude["level"] == 21
    assert geodude["item"] == "Berry Juice"
    assert geodude["ability"] == "Sturdy"
    assert geodude["nature"] == "Adamant"
    assert geodude["moves"] == [
        "Stealth Rock",
        "Rock Blast",
        "Earthquake",
        "Sucker Punch",
    ]


def validate_route101(tracker_data: dict) -> None:
    route101 = tracker_data["locations"]["route101"]
    assert [4, 10] in route101["coords"]
    grass = [enc for enc in route101["encounters"] if enc["method"] == "grass"]
    assert len(grass) == 12
    assert grass[0] == {
        "species": "wurmple",
        "chance": 20,
        "minLevel": 2,
        "maxLevel": 2,
        "method": "grass",
    }
    assert any(enc["species"] == "poochyena" for enc in grass)


def relative_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()

