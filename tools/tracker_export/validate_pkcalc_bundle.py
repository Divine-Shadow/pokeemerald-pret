#!/usr/bin/env python3
"""Validate an auditable PKCalc tracker release bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import tarfile
from collections import OrderedDict
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TRACKER_DIR = REPO_ROOT / "build/tracker_export"
DEFAULT_BUNDLE = DEFAULT_TRACKER_DIR / "pkcalc_tracker_release_bundle.tar.gz"
DEFAULT_WORK_DIR = DEFAULT_TRACKER_DIR / "bundle_check"
BUNDLE_ROOT = "pkcalc_tracker_release_bundle"

REQUIRED_FILES = [
    "pkcalc_overlay/js/data/party_order.js",
    "pkcalc_overlay/js/data/sets.js",
    "pkcalc_overlay/js/data/dex/locations.js",
    "pkcalc_overlay/manifest.json",
    "pkcalc_overlay/README.txt",
    "coverage_report.json",
    "reference_report.json",
    "live_reference_report.json",
    "pkcalc_compat_contract.json",
    "bundle_manifest.json",
    "VERIFY.txt",
]
REQUIRED_VERIFY_COMMANDS = [
    "make tracker-export-check",
    "make tracker-export-coverage-check",
    "make tracker-export-reference-check",
    "make tracker-export-live-reference-check",
    "make tracker-export-overlay-check",
    "make tracker-export-compat-check",
    "make tracker-export-bundle-check",
]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--bundle",
        default=DEFAULT_BUNDLE,
        type=Path,
        help="Release bundle archive to validate.",
    )
    parser.add_argument(
        "--work-dir",
        default=DEFAULT_WORK_DIR,
        type=Path,
        help="Directory for unpacking and validating the bundle.",
    )
    args = parser.parse_args()

    bundle = resolve_from_root(args.bundle)
    work_dir = resolve_from_root(args.work_dir)
    root = unpack_bundle(bundle, work_dir)
    validate_bundle(root)

    print(f"Validated {relpath_or_abs(bundle)}")
    print(f"Unpacked {relpath_or_abs(root)}")
    print(
        "Bundle checks passed: overlay paths, manifest, coverage, references, live references, contract, checksums, VERIFY.txt"
    )


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def unpack_bundle(bundle: Path, work_dir: Path) -> Path:
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)

    with tarfile.open(bundle, "r:gz") as tar:
        safe_extract(tar, work_dir)

    entries = sorted(path for path in work_dir.iterdir())
    expected_root = work_dir / BUNDLE_ROOT
    if entries != [expected_root] or not expected_root.is_dir():
        raise AssertionError(f"bundle must unpack to exactly one {BUNDLE_ROOT}/ directory")
    return expected_root


def safe_extract(tar: tarfile.TarFile, destination: Path) -> None:
    destination_resolved = destination.resolve()
    for member in tar.getmembers():
        member_path = Path(member.name)
        if member_path.is_absolute():
            raise AssertionError(f"unsafe absolute path in bundle: {member.name}")
        if member.issym() or member.islnk() or not (member.isfile() or member.isdir()):
            raise AssertionError(f"unsupported tar member type: {member.name}")
        target = (destination / member.name).resolve()
        if not target.is_relative_to(destination_resolved):
            raise AssertionError(f"unsafe path traversal in bundle: {member.name}")
    tar.extractall(destination, filter="data")


def validate_bundle(root: Path) -> None:
    for rel_path in REQUIRED_FILES:
        path = root / rel_path
        assert path.exists(), f"missing bundle file: {rel_path}"

    manifest = load_json(root / "bundle_manifest.json")
    overlay_manifest = load_json(root / "pkcalc_overlay/manifest.json")
    coverage = load_json(root / "coverage_report.json")
    references = load_json(root / "reference_report.json")
    live_references = load_json(root / "live_reference_report.json")
    contract = load_json(root / "pkcalc_compat_contract.json")
    verify_text = (root / "VERIFY.txt").read_text()

    assert manifest["schemaVersion"] == 1
    assert manifest["artifact"] == "pkcalc-tracker-release-bundle"
    assert manifest["bundleRoot"] == BUNDLE_ROOT
    assert manifest["checksumAlgorithm"] == "sha256"
    assert overlay_manifest["schemaVersion"] == 1
    assert overlay_manifest["artifact"] == "pkcalc-overlay"
    assert coverage["schemaVersion"] == 1
    assert coverage["failures"] == []
    assert references["schemaVersion"] == 1
    assert references["failures"] == []
    assert live_references["schemaVersion"] == 1
    assert live_references["status"] == "ok"
    assert live_references["failures"] == []
    assert contract["schemaVersion"] == 1

    validate_manifest_files(root, manifest)
    validate_checksums(root, manifest)
    validate_counts(manifest, overlay_manifest, coverage, references, live_references)
    validate_verify_notes(verify_text)


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def validate_manifest_files(root: Path, manifest: OrderedDict) -> None:
    files = manifest["files"]
    assert files["overlayRoot"] == "pkcalc_overlay"
    assert files["overlayManifest"] == "pkcalc_overlay/manifest.json"
    assert files["overlayReadme"] == "pkcalc_overlay/README.txt"
    assert files["coverageReport"] == "coverage_report.json"
    assert files["referenceReport"] == "reference_report.json"
    assert files["liveReferenceReport"] == "live_reference_report.json"
    assert files["compatContract"] == "pkcalc_compat_contract.json"
    assert files["verifyNotes"] == "VERIFY.txt"

    for name, rel_path in files["overlayFiles"].items():
        assert name in {"partyOrder", "sets", "locations"}
        assert (root / "pkcalc_overlay" / rel_path).exists(), (
            f"missing overlay file from manifest: {rel_path}"
        )


def validate_checksums(root: Path, manifest: OrderedDict) -> None:
    checksums = manifest["checksums"]
    actual_files = sorted(
        relpath_posix(path, root)
        for path in root.rglob("*")
        if path.is_file() and relpath_posix(path, root) != "bundle_manifest.json"
    )
    expected_files = sorted(checksums.keys())
    assert actual_files == expected_files, "bundle checksum file list does not match payload"

    for rel_path, expected_digest in checksums.items():
        actual_digest = sha256(root / rel_path)
        assert actual_digest == expected_digest, f"checksum mismatch for {rel_path}"


def validate_counts(
    manifest: OrderedDict,
    overlay_manifest: OrderedDict,
    coverage: OrderedDict,
    references: OrderedDict,
    live_references: OrderedDict,
) -> None:
    assert manifest["counts"]["coverageFailures"] == 0
    assert manifest["counts"]["referenceFailures"] == 0
    assert manifest["counts"]["liveReferenceFailures"] == 0
    assert manifest["counts"]["liveReferenceStatus"] == "ok"
    assert manifest["counts"]["contractSchemaVersion"] == 1
    assert manifest["counts"]["overlay"] == overlay_manifest["counts"]
    assert manifest["counts"]["coverage"] == coverage["counts"]
    assert manifest["counts"]["references"] == reference_counts(references)
    assert manifest["counts"]["liveReferences"] == live_reference_counts(live_references)

    trainer_counts = coverage["counts"]["trainers"]
    location_counts = coverage["counts"]["locations"]
    overlay_counts = overlay_manifest["counts"]
    assert overlay_counts["trainers"] == trainer_counts["generatedTrainers"]
    assert overlay_counts["partyOrder"] == trainer_counts["generatedPartyOrderEntries"]
    assert overlay_counts["setdexSpecies"] == trainer_counts["generatedSetdexSpecies"]
    assert overlay_counts["locations"] == location_counts["generatedLocations"]
    assert location_counts["generatedEncounterSlots"] == location_counts["expectedEncounterSlots"]
    assert references["categories"]["species"]["count"] == trainer_counts["generatedSetdexSpecies"]
    assert references["categories"]["encounterSpecies"]["count"] > 0
    assert live_references["encounterRoute"]["status"] == "checked"
    assert (
        live_references["encounterRoute"]["locationsWithEncounters"]
        == location_counts["generatedLocationsWithEncounters"]
    )
    assert live_references["encounterRoute"]["expectedRows"] == location_counts["generatedEncounterSlots"]
    assert live_references["encounterRoute"]["renderedRows"] == location_counts["generatedEncounterSlots"]
    for category, data in references["categories"].items():
        if category == "types":
            continue
        assert live_references["categories"][category]["count"] == data["count"]
        assert live_references["categories"][category]["selected"]["resolvedCount"] == data["count"]
        assert live_references["categories"][category]["unresolvedCount"] == 0


def reference_counts(references: OrderedDict) -> OrderedDict:
    return OrderedDict(
        (
            category,
            OrderedDict(
                [
                    ("count", data["count"]),
                    ("occurrences", data["occurrences"]),
                    ("normalizedCount", data["normalizedCount"]),
                ]
            ),
        )
        for category, data in references["categories"].items()
    )


def live_reference_counts(live_references: OrderedDict) -> OrderedDict:
    categories = live_references["categories"]
    return OrderedDict(
        [
            (
                "categories",
                OrderedDict(
                    (
                        category,
                        OrderedDict(
                            [
                                ("count", data["count"]),
                                ("resolvedCount", data["selected"]["resolvedCount"]),
                                ("unresolvedCount", data["unresolvedCount"]),
                                ("selectedExpression", data["selected"]["expression"]),
                            ]
                        ),
                    )
                    for category, data in categories.items()
                ),
            ),
            (
                "encounterRoute",
                OrderedDict(
                    [
                        (
                            "locationsWithEncounters",
                            live_references["encounterRoute"]["locationsWithEncounters"],
                        ),
                        ("expectedRows", live_references["encounterRoute"]["expectedRows"]),
                        ("renderedRows", live_references["encounterRoute"]["renderedRows"]),
                        ("failureCount", live_references["encounterRoute"]["failureCount"]),
                    ]
                ),
            ),
        ]
    )


def validate_verify_notes(text: str) -> None:
    for command in REQUIRED_VERIFY_COMMANDS:
        assert command in text, f"VERIFY.txt does not mention {command}"
    for rel_path in [
        "pkcalc_overlay/js/data/party_order.js",
        "pkcalc_overlay/js/data/sets.js",
        "pkcalc_overlay/js/data/dex/locations.js",
    ]:
        assert rel_path in text, f"VERIFY.txt does not mention {rel_path}"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relpath_posix(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
