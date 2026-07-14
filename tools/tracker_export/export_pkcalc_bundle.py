#!/usr/bin/env python3
"""Create an auditable PKCalc tracker release bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tarfile
from collections import OrderedDict
from datetime import datetime, timezone
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TRACKER_DIR = REPO_ROOT / "build/tracker_export"
DEFAULT_OVERLAY_DIR = DEFAULT_TRACKER_DIR / "pkcalc_overlay"
DEFAULT_COVERAGE_REPORT = DEFAULT_TRACKER_DIR / "coverage_report.json"
DEFAULT_REFERENCE_REPORT = DEFAULT_TRACKER_DIR / "reference_report.json"
DEFAULT_LIVE_REFERENCE_REPORT = DEFAULT_TRACKER_DIR / "live_reference_report.json"
DEFAULT_CONTRACT = REPO_ROOT / "tools/tracker_export/pkcalc_compat_contract.json"
DEFAULT_BUNDLE_DIR = DEFAULT_TRACKER_DIR / "release_bundle"
DEFAULT_BUNDLE = DEFAULT_TRACKER_DIR / "pkcalc_tracker_release_bundle.tar.gz"
BUNDLE_ROOT = "pkcalc_tracker_release_bundle"

REQUIRED_OVERLAY_FILES = OrderedDict(
    [
        ("partyOrder", "js/data/party_order.js"),
        ("sets", "js/data/sets.js"),
        ("locations", "js/data/dex/locations.js"),
    ]
)
VERIFY_COMMANDS = [
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
        "--tracker-dir",
        default=DEFAULT_TRACKER_DIR,
        type=Path,
        help="Directory containing generated tracker_data.json.",
    )
    parser.add_argument(
        "--overlay-dir",
        default=DEFAULT_OVERLAY_DIR,
        type=Path,
        help="Generated PKCalc overlay directory.",
    )
    parser.add_argument(
        "--coverage-report",
        default=DEFAULT_COVERAGE_REPORT,
        type=Path,
        help="Generated coverage_report.json path.",
    )
    parser.add_argument(
        "--reference-report",
        default=DEFAULT_REFERENCE_REPORT,
        type=Path,
        help="Generated reference_report.json path.",
    )
    parser.add_argument(
        "--live-reference-report",
        default=DEFAULT_LIVE_REFERENCE_REPORT,
        type=Path,
        help="Generated live_reference_report.json path.",
    )
    parser.add_argument(
        "--contract",
        default=DEFAULT_CONTRACT,
        type=Path,
        help="Committed PKCalc compatibility contract to snapshot.",
    )
    parser.add_argument(
        "--bundle-dir",
        default=DEFAULT_BUNDLE_DIR,
        type=Path,
        help="Staging directory for bundle contents.",
    )
    parser.add_argument(
        "--bundle",
        default=DEFAULT_BUNDLE,
        type=Path,
        help="Output .tar.gz bundle path.",
    )
    args = parser.parse_args()

    tracker_dir = resolve_from_root(args.tracker_dir)
    overlay_dir = resolve_from_root(args.overlay_dir)
    coverage_report = resolve_from_root(args.coverage_report)
    reference_report = resolve_from_root(args.reference_report)
    live_reference_report = resolve_from_root(args.live_reference_report)
    contract = resolve_from_root(args.contract)
    bundle_dir = resolve_from_root(args.bundle_dir)
    bundle = resolve_from_root(args.bundle)

    build_bundle(
        tracker_dir=tracker_dir,
        overlay_dir=overlay_dir,
        coverage_report=coverage_report,
        reference_report=reference_report,
        live_reference_report=live_reference_report,
        contract=contract,
        bundle_dir=bundle_dir,
        bundle=bundle,
    )

    print(f"Wrote {relpath_or_abs(bundle_dir / BUNDLE_ROOT)}")
    print(f"Wrote {relpath_or_abs(bundle)}")


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def build_bundle(
    tracker_dir: Path,
    overlay_dir: Path,
    coverage_report: Path,
    reference_report: Path,
    live_reference_report: Path,
    contract: Path,
    bundle_dir: Path,
    bundle: Path,
) -> None:
    tracker_data = load_json(tracker_dir / "tracker_data.json")
    overlay_manifest = load_json(overlay_dir / "manifest.json")
    coverage = load_json(coverage_report)
    references = load_json(reference_report)
    live_references = load_json(live_reference_report)
    compat_contract = load_json(contract)
    validate_inputs(overlay_dir, coverage, references, live_references, compat_contract)

    if bundle_dir.exists():
        shutil.rmtree(bundle_dir)
    root = bundle_dir / BUNDLE_ROOT
    root.mkdir(parents=True, exist_ok=True)

    shutil.copytree(overlay_dir, root / "pkcalc_overlay")
    shutil.copyfile(coverage_report, root / "coverage_report.json")
    shutil.copyfile(reference_report, root / "reference_report.json")
    shutil.copyfile(live_reference_report, root / "live_reference_report.json")
    shutil.copyfile(contract, root / "pkcalc_compat_contract.json")
    write_verify(root / "VERIFY.txt")

    write_json(
        root / "bundle_manifest.json",
        make_bundle_manifest(
            root=root,
            tracker_dir=tracker_dir,
            overlay_dir=overlay_dir,
            coverage_report=coverage_report,
            reference_report=reference_report,
            live_reference_report=live_reference_report,
            contract=contract,
            tracker_data=tracker_data,
            overlay_manifest=overlay_manifest,
            coverage=coverage,
            references=references,
            live_references=live_references,
            compat_contract=compat_contract,
        ),
    )

    bundle.parent.mkdir(parents=True, exist_ok=True)
    if bundle.exists():
        bundle.unlink()
    write_tarball(root, bundle)


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def validate_inputs(
    overlay_dir: Path,
    coverage: OrderedDict,
    references: OrderedDict,
    live_references: OrderedDict,
    compat_contract: OrderedDict,
) -> None:
    for rel_path in REQUIRED_OVERLAY_FILES.values():
        path = overlay_dir / rel_path
        if not path.exists():
            raise FileNotFoundError(path)
    for name in ["manifest.json", "README.txt"]:
        path = overlay_dir / name
        if not path.exists():
            raise FileNotFoundError(path)
    if coverage.get("failures"):
        raise ValueError("coverage_report.json has failures; run tracker-export-coverage-check")
    if references.get("failures"):
        raise ValueError("reference_report.json has failures; run tracker-export-reference-check")
    if live_references.get("schemaVersion") != 1:
        raise ValueError("live_reference_report.json schemaVersion must be 1")
    if live_references.get("status") != "ok" or live_references.get("failures"):
        raise ValueError(
            "live_reference_report.json has failures; run tracker-export-live-reference-check"
        )
    if compat_contract.get("schemaVersion") != 1:
        raise ValueError("PKCalc compatibility contract schemaVersion must be 1")


def write_verify(path: Path) -> None:
    commands = "\n".join(f"  {command}" for command in VERIFY_COMMANDS)
    text = f"""PKCalc Tracker Release Bundle

This archive contains the generated PKCalc tracker overlay and proof artifacts for this repository.

Contents:
  pkcalc_overlay/
  coverage_report.json
  reference_report.json
  live_reference_report.json
  pkcalc_compat_contract.json
  bundle_manifest.json
  VERIFY.txt

Apply the overlay by copying pkcalc_overlay/ over a PKCalc build root. It replaces only:
  pkcalc_overlay/js/data/party_order.js
  pkcalc_overlay/js/data/sets.js
  pkcalc_overlay/js/data/dex/locations.js

Verification commands used by maintainers:
{commands}

This bundle is static tracker data only. It does not include PKCalc source, hosted UI support, full Dex data, damage-calculator correctness data, Lua/save sync, or emulator sync.
"""
    path.write_text(text)


def make_bundle_manifest(
    root: Path,
    tracker_dir: Path,
    overlay_dir: Path,
    coverage_report: Path,
    reference_report: Path,
    live_reference_report: Path,
    contract: Path,
    tracker_data: OrderedDict,
    overlay_manifest: OrderedDict,
    coverage: OrderedDict,
    references: OrderedDict,
    live_references: OrderedDict,
    compat_contract: OrderedDict,
) -> OrderedDict:
    checksums = file_checksums(root, exclude={"bundle_manifest.json"})
    coverage_counts = coverage["counts"]
    return OrderedDict(
        [
            ("schemaVersion", 1),
            ("artifact", "pkcalc-tracker-release-bundle"),
            ("bundleRoot", BUNDLE_ROOT),
            ("generatedAt", datetime.now(timezone.utc).replace(microsecond=0).isoformat()),
            (
                "repo",
                OrderedDict(
                    [
                        ("commit", git_output(["rev-parse", "HEAD"]) or "unknown"),
                        ("dirty", bool(git_output(["status", "--porcelain"]))),
                    ]
                ),
            ),
            (
                "inputs",
                OrderedDict(
                    [
                        ("trackerData", relpath_or_abs(tracker_dir / "tracker_data.json")),
                        ("overlayDir", relpath_or_abs(overlay_dir)),
                        ("coverageReport", relpath_or_abs(coverage_report)),
                        ("referenceReport", relpath_or_abs(reference_report)),
                        ("liveReferenceReport", relpath_or_abs(live_reference_report)),
                        ("compatContract", relpath_or_abs(contract)),
                    ]
                ),
            ),
            (
                "files",
                OrderedDict(
                    [
                        ("overlayRoot", "pkcalc_overlay"),
                        ("overlayFiles", REQUIRED_OVERLAY_FILES),
                        ("overlayManifest", "pkcalc_overlay/manifest.json"),
                        ("overlayReadme", "pkcalc_overlay/README.txt"),
                        ("coverageReport", "coverage_report.json"),
                        ("referenceReport", "reference_report.json"),
                        ("liveReferenceReport", "live_reference_report.json"),
                        ("compatContract", "pkcalc_compat_contract.json"),
                        ("verifyNotes", "VERIFY.txt"),
                    ]
                ),
            ),
            (
                "counts",
                OrderedDict(
                    [
                        ("overlay", overlay_manifest["counts"]),
                        ("coverage", coverage_counts),
                        ("references", reference_counts(references)),
                        ("liveReferences", live_reference_counts(live_references)),
                        ("coverageFailures", len(coverage.get("failures", []))),
                        ("referenceFailures", len(references.get("failures", []))),
                        ("liveReferenceFailures", len(live_references.get("failures", []))),
                        ("liveReferenceStatus", live_references["status"]),
                        ("contractSchemaVersion", compat_contract["schemaVersion"]),
                        ("trackerDataTrainers", len(tracker_data["trainers"])),
                        ("trackerDataLocations", len(tracker_data["locations"])),
                    ]
                ),
            ),
            ("verificationCommands", VERIFY_COMMANDS),
            ("checksumAlgorithm", "sha256"),
            ("checksumScope", "all bundled files except bundle_manifest.json"),
            ("checksums", checksums),
        ]
    )


def file_checksums(root: Path, exclude: set[str] | None = None) -> OrderedDict:
    exclude = exclude or set()
    checksums = OrderedDict()
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        rel_path = relpath_posix(path, root)
        if rel_path in exclude:
            continue
        checksums[rel_path] = sha256(path)
    return checksums


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


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_tarball(root: Path, bundle: Path) -> None:
    with tarfile.open(bundle, "w:gz") as tar:
        for path in sorted([root, *root.rglob("*")]):
            arcname = str(Path(BUNDLE_ROOT) / path.relative_to(root)) if path != root else BUNDLE_ROOT
            tar.add(path, arcname=arcname, recursive=False)


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


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


def relpath_posix(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
