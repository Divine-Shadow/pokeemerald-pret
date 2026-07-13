#!/usr/bin/env python3
"""Validate generated PKCalc data-migration proof reports."""

from __future__ import annotations

import argparse
import json
import sys
from collections import OrderedDict
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INPUT_DIR = REPO_ROOT / "build/tracker_export/data_migration"
REQUIRED_NATURE_FIELDS = ["kind", "id", "name", "plus", "minus"]
REQUIRED_ABILITY_FIELDS = ["kind", "id", "name"]
REQUIRED_ITEM_IDENTITY_FIELDS = ["kind", "id", "name"]
REQUIRED_MOVE_METADATA_FIELDS = ["kind", "id", "name", "type", "category", "basePower"]
REQUIRED_CALC_MOVE_FIELDS = ["bp", "type", "category"]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input-dir",
        default=DEFAULT_INPUT_DIR,
        type=Path,
        help="Generated data-migration artifact directory.",
    )
    parser.add_argument(
        "--mapping-report",
        type=Path,
        help="Catalog mapping report path. Defaults under input-dir.",
    )
    parser.add_argument(
        "--validation-report",
        type=Path,
        help="Nature migration validation report path. Defaults under input-dir.",
    )
    parser.add_argument(
        "--ability-gap-report",
        type=Path,
        help="Ability identity gap report path. Defaults under input-dir.",
    )
    parser.add_argument(
        "--held-item-gap-report",
        type=Path,
        help="Held-item identity gap report path. Defaults under input-dir.",
    )
    parser.add_argument(
        "--move-gap-report",
        type=Path,
        help="Move metadata gap report path. Defaults under input-dir.",
    )
    parser.add_argument(
        "--require-identity",
        action="store_true",
        help="Also validate ability and held-item identity source artifacts and gap reports.",
    )
    parser.add_argument(
        "--require-move-metadata",
        action="store_true",
        help="Also validate move metadata source artifacts and the live gap report.",
    )
    args = parser.parse_args()

    input_dir = resolve_from_root(args.input_dir)
    mapping_report_path = (
        resolve_from_root(args.mapping_report)
        if args.mapping_report
        else input_dir / "pkcalc_catalog_mapping_report.json"
    )
    validation_report_path = (
        resolve_from_root(args.validation_report)
        if args.validation_report
        else input_dir / "natures_migration_validation_report.json"
    )
    ability_gap_report_path = (
        resolve_from_root(args.ability_gap_report)
        if args.ability_gap_report
        else input_dir / "ability_identity_gap_report.json"
    )
    held_item_gap_report_path = (
        resolve_from_root(args.held_item_gap_report)
        if args.held_item_gap_report
        else input_dir / "held_item_identity_gap_report.json"
    )
    move_gap_report_path = (
        resolve_from_root(args.move_gap_report)
        if args.move_gap_report
        else input_dir / "move_metadata_gap_report.json"
    )

    failures = validate(
        input_dir=input_dir,
        mapping_report_path=mapping_report_path,
        validation_report_path=validation_report_path,
        ability_gap_report_path=ability_gap_report_path,
        held_item_gap_report_path=held_item_gap_report_path,
        move_gap_report_path=move_gap_report_path,
        require_identity=args.require_identity,
        require_move_metadata=args.require_move_metadata,
    )
    if failures:
        for failure in failures:
            print(f"PKCalc data migration validation failure: {failure}", file=sys.stderr)
        sys.exit(1)

    validation_report = load_json(validation_report_path)
    print(f"Validated {relpath_or_abs(mapping_report_path)}")
    print(f"Validated {relpath_or_abs(validation_report_path)}")
    print(
        "PKCalc natures migration proof passed: "
        f"{validation_report['checks']['sourceCount']} source natures, "
        f"{validation_report['checks']['generatedByIdCount']} by-id entries, "
        f"{validation_report['checks']['generatedCalcCount']} calc entries"
    )
    if args.require_identity:
        ability_report = load_json(ability_gap_report_path)
        held_item_report = load_json(held_item_gap_report_path)
        print(f"Validated {relpath_or_abs(ability_gap_report_path)}")
        print(f"Validated {relpath_or_abs(held_item_gap_report_path)}")
        print(
            "PKCalc identity migration proof passed: "
            f"{ability_report['counts']['generatedById']} abilities "
            f"({ability_report['status']}), "
            f"{held_item_report['counts']['generatedById']} held items "
            f"({held_item_report['status']})"
        )
    if args.require_move_metadata:
        move_report = load_json(move_gap_report_path)
        print(f"Validated {relpath_or_abs(move_gap_report_path)}")
        print(
            "PKCalc move metadata migration proof passed: "
            f"{move_report['counts']['generatedById']} generated moves "
            f"({move_report['status']}), "
            f"{move_report['counts']['sharedIds']} shared live ids, "
            f"{move_report['counts']['incompatibleValues']} incompatible values reported"
        )


def validate(
    input_dir: Path,
    mapping_report_path: Path,
    validation_report_path: Path,
    ability_gap_report_path: Path,
    held_item_gap_report_path: Path,
    move_gap_report_path: Path,
    require_identity: bool,
    require_move_metadata: bool,
) -> list[str]:
    failures: list[str] = []
    source_natures = load_json(input_dir / "source_natures.json")
    natures_by_id = load_json(input_dir / "pkcalc_natures_by_id.json")
    calc_natures = load_json(input_dir / "pkcalc_calc_natures.json")
    mapping_report = load_json(mapping_report_path)
    validation_report = load_json(validation_report_path)

    require(failures, source_natures["schemaVersion"] == 1, "source_natures schemaVersion must be 1")
    require(failures, source_natures["category"] == "natures", "source_natures category must be natures")
    require(failures, source_natures["failures"] == [], "source_natures must have no failures")
    require(failures, len(source_natures["natures"]) == 25, "source_natures must contain 25 natures")
    require(failures, len(natures_by_id) == 25, "pkcalc_natures_by_id must contain 25 entries")
    require(failures, len(calc_natures) == 25, "pkcalc_calc_natures must contain 25 entries")

    for nature in source_natures["natures"]:
        by_id = natures_by_id.get(nature["pkcalcId"])
        require(failures, by_id is not None, f"missing by-id nature {nature['pkcalcId']}")
        if by_id is not None:
            expected = OrderedDict(
                [
                    ("kind", "Nature"),
                    ("id", nature["pkcalcId"]),
                    ("name", nature["name"]),
                    ("plus", nature["pkcalcPlus"]),
                    ("minus", nature["pkcalcMinus"]),
                ]
            )
            for field in REQUIRED_NATURE_FIELDS:
                require(
                    failures,
                    by_id.get(field) == expected[field],
                    f"by-id nature {nature['pkcalcId']} field {field} mismatch",
                )
        calc = calc_natures.get(nature["name"])
        require(
            failures,
            calc == [nature["pkcalcPlus"], nature["pkcalcMinus"]],
            f"calc nature {nature['name']} mismatch",
        )

    require(failures, mapping_report["schemaVersion"] == 1, "mapping report schemaVersion must be 1")
    require(failures, mapping_report["status"] == "ok", "mapping report status must be ok")
    require(
        failures,
        mapping_report["chosenMigrationCategory"] == "natures",
        "mapping report chosenMigrationCategory must be natures",
    )
    selected = mapping_report["repoSourceMappings"]["natures"]
    require(failures, selected["status"] == "ready", "natures mapping status must be ready")
    require(failures, selected["missingFields"] == [], "natures mapping must have no missing fields")
    require(
        failures,
        selected["incompatibleValues"] == [],
        "natures mapping must have no incompatible values",
    )
    require(failures, mapping_report["failures"] == [], "mapping report failures must be empty")

    require(
        failures,
        validation_report["schemaVersion"] == 1,
        "validation report schemaVersion must be 1",
    )
    require(failures, validation_report["status"] == "ok", "validation report status must be ok")
    require(failures, validation_report["category"] == "natures", "validation category must be natures")
    require(
        failures,
        validation_report["missingFields"] == [],
        "validation report missingFields must be empty",
    )
    require(
        failures,
        validation_report["incompatibleValues"] == [],
        "validation report incompatibleValues must be empty",
    )
    require(failures, validation_report["failures"] == [], "validation report failures must be empty")
    checks = validation_report["checks"]
    for check in [
        "sourceHasNoFailures",
        "generatedMatchesSource",
        "liveByIdMatchesGenerated",
        "liveCalcMatchesGenerated",
    ]:
        require(failures, checks[check] is True, f"validation check {check} must be true")
    require(failures, checks["missingFieldCount"] == 0, "missingFieldCount must be 0")
    require(failures, checks["incompatibleValueCount"] == 0, "incompatibleValueCount must be 0")

    if require_identity:
        validate_identity_artifacts(
            failures=failures,
            source_report=load_json(input_dir / "source_abilities.json"),
            by_id=load_json(input_dir / "pkcalc_abilities_by_id.json"),
            gap_report=load_json(ability_gap_report_path),
            category="abilities",
            source_key="abilities",
            expected_kind="Ability",
            required_fields=REQUIRED_ABILITY_FIELDS,
        )
        validate_identity_artifacts(
            failures=failures,
            source_report=load_json(input_dir / "source_held_items.json"),
            by_id=load_json(input_dir / "pkcalc_held_items_by_id.json"),
            gap_report=load_json(held_item_gap_report_path),
            category="heldItems",
            source_key="heldItems",
            expected_kind="Item",
            required_fields=REQUIRED_ITEM_IDENTITY_FIELDS,
        )
        for category in ["abilities", "heldItems"]:
            require(
                failures,
                category in mapping_report["repoSourceMappings"],
                f"mapping report must include {category}",
            )
    if require_move_metadata:
        validate_move_metadata_artifacts(
            failures=failures,
            source_report=load_json(input_dir / "source_moves.json"),
            by_id=load_json(input_dir / "pkcalc_moves_by_id.json"),
            calc_moves=load_json(input_dir / "pkcalc_calc_moves.json"),
            gap_report=load_json(move_gap_report_path),
        )
        require(
            failures,
            "moves" in mapping_report["repoSourceMappings"],
            "mapping report must include moves",
        )
        require(
            failures,
            "moves" in mapping_report.get("metadataProofCategories", []),
            "mapping report metadataProofCategories must include moves",
        )
    return failures


def validate_identity_artifacts(
    failures: list[str],
    source_report: OrderedDict,
    by_id: OrderedDict,
    gap_report: OrderedDict,
    category: str,
    source_key: str,
    expected_kind: str,
    required_fields: list[str],
) -> None:
    require(failures, source_report["schemaVersion"] == 1, f"{category} source schemaVersion must be 1")
    require(failures, source_report["category"] == category, f"{category} source category mismatch")
    require(failures, source_report["failures"] == [], f"{category} source failures must be empty")
    entries = source_report[source_key]
    require(failures, len(entries) > 0, f"{category} source must contain entries")
    require(
        failures,
        source_report["identityCandidateCount"] == len(entries),
        f"{category} identityCandidateCount must match entries",
    )
    require(failures, len(by_id) == len(entries), f"{category} by-id count must match source entries")

    for entry in entries:
        generated = by_id.get(entry["pkcalcId"])
        require(failures, generated is not None, f"{category} missing by-id entry {entry['pkcalcId']}")
        if generated is None:
            continue
        expected = OrderedDict(
            [
                ("kind", expected_kind),
                ("id", entry["pkcalcId"]),
                ("name", entry["name"]),
            ]
        )
        for field in required_fields:
            require(
                failures,
                generated.get(field) == expected[field],
                f"{category} {entry['pkcalcId']} field {field} mismatch",
            )

    require(failures, gap_report["schemaVersion"] == 1, f"{category} gap schemaVersion must be 1")
    require(failures, gap_report["category"] == category, f"{category} gap category mismatch")
    require(
        failures,
        gap_report["status"] in {"ok", "ok_with_gaps"},
        f"{category} gap status must be ok or ok_with_gaps",
    )
    require(failures, gap_report["failures"] == [], f"{category} gap failures must be empty")
    require(failures, gap_report["missingFields"] == [], f"{category} gap missingFields must be empty")
    require(
        failures,
        gap_report["incompatibleValues"] == [],
        f"{category} gap incompatibleValues must be empty",
    )
    require(
        failures,
        isinstance(gap_report["customRepoOnly"], list),
        f"{category} gap customRepoOnly must be a list",
    )
    require(
        failures,
        isinstance(gap_report["incompatibleIds"], list),
        f"{category} gap incompatibleIds must be a list",
    )
    require(
        failures,
        gap_report["incompatibleIds"] == [],
        f"{category} gap incompatibleIds must be empty",
    )
    counts = gap_report["counts"]
    require(
        failures,
        counts["generatedById"] == len(by_id),
        f"{category} gap generatedById count must match generated artifact",
    )
    require(failures, counts["sharedIds"] >= 1, f"{category} gap report must have at least one shared id")
    for count_name in [
        "liveById",
        "liveCalcNames",
        "missingFromLiveById",
        "missingFromCalc",
        "liveOnlyById",
        "liveCalcOnly",
        "displayNameDifferences",
        "missingFields",
        "incompatibleValues",
    ]:
        require(
            failures,
            isinstance(counts[count_name], int) and counts[count_name] >= 0,
            f"{category} gap count {count_name} must be a non-negative integer",
        )


def validate_move_metadata_artifacts(
    failures: list[str],
    source_report: OrderedDict,
    by_id: OrderedDict,
    calc_moves: OrderedDict,
    gap_report: OrderedDict,
) -> None:
    require(failures, source_report["schemaVersion"] == 1, "moves source schemaVersion must be 1")
    require(failures, source_report["category"] == "moves", "moves source category mismatch")
    require(failures, source_report["failures"] == [], "moves source failures must be empty")
    entries = source_report["moves"]
    complete_entries = [entry for entry in entries if entry["metadataComplete"]]
    require(failures, len(entries) > 0, "moves source must contain entries")
    require(
        failures,
        source_report["metadataCandidateCount"] == len(entries),
        "moves metadataCandidateCount must match entries",
    )
    require(
        failures,
        source_report["metadataCompleteCount"] == len(complete_entries),
        "moves metadataCompleteCount must match complete entries",
    )
    require(
        failures,
        len(complete_entries) == len(entries),
        "all move metadata candidates must be complete",
    )
    require(failures, len(by_id) == len(complete_entries), "moves by-id count must match complete entries")
    require(failures, len(calc_moves) == len(complete_entries), "moves calc count must match complete entries")

    for entry in complete_entries:
        generated = by_id.get(entry["pkcalcId"])
        require(failures, generated is not None, f"moves missing by-id entry {entry['pkcalcId']}")
        if generated is not None:
            expected = OrderedDict(
                [
                    ("kind", "Move"),
                    ("id", entry["pkcalcId"]),
                    ("name", entry["name"]),
                    ("type", entry["type"]),
                    ("category", entry["category"]),
                    ("basePower", entry["basePower"]),
                ]
            )
            for field in REQUIRED_MOVE_METADATA_FIELDS:
                require(
                    failures,
                    generated.get(field) == expected[field],
                    f"moves {entry['pkcalcId']} field {field} mismatch",
                )
        calc = calc_moves.get(entry["name"])
        require(failures, calc is not None, f"moves missing calc entry {entry['name']}")
        if calc is not None:
            expected_calc = OrderedDict(
                [
                    ("bp", entry["basePower"]),
                    ("type", entry["type"]),
                    ("category", entry["category"]),
                ]
            )
            for field in REQUIRED_CALC_MOVE_FIELDS:
                require(
                    failures,
                    calc.get(field) == expected_calc[field],
                    f"moves calc {entry['name']} field {field} mismatch",
                )

    require(failures, gap_report["schemaVersion"] == 1, "moves gap schemaVersion must be 1")
    require(failures, gap_report["category"] == "moves", "moves gap category mismatch")
    require(
        failures,
        gap_report["status"] in {"ok", "ok_with_gaps"},
        "moves gap status must be ok or ok_with_gaps",
    )
    require(failures, gap_report["failures"] == [], "moves gap failures must be empty")
    require(failures, isinstance(gap_report["missingFields"], list), "moves missingFields must be a list")
    require(
        failures,
        isinstance(gap_report["incompatibleValues"], list),
        "moves incompatibleValues must be a list",
    )
    require(
        failures,
        isinstance(gap_report["customRepoOnly"], list),
        "moves customRepoOnly must be a list",
    )
    require(
        failures,
        isinstance(gap_report["incompatibleIds"], list),
        "moves incompatibleIds must be a list",
    )
    require(failures, gap_report["incompatibleIds"] == [], "moves incompatibleIds must be empty")
    require(
        failures,
        isinstance(gap_report["deferredSemanticFields"], list)
        and len(gap_report["deferredSemanticFields"]) > 0,
        "moves deferredSemanticFields must be a non-empty list",
    )
    counts = gap_report["counts"]
    require(
        failures,
        counts["generatedById"] == len(by_id),
        "moves gap generatedById count must match generated artifact",
    )
    require(
        failures,
        counts["generatedCalc"] == len(calc_moves),
        "moves gap generatedCalc count must match generated artifact",
    )
    require(failures, counts["sharedIds"] >= 1, "moves gap report must have at least one shared id")
    require(
        failures,
        counts["incompatibleValues"] == len(gap_report["incompatibleValues"]),
        "moves incompatibleValues count must match report",
    )
    require(
        failures,
        counts["missingFields"] == len(gap_report["missingFields"]),
        "moves missingFields count must match report",
    )
    for count_name in [
        "liveById",
        "liveCalcNames",
        "sharedCalcNames",
        "missingFromLiveById",
        "missingFromCalc",
        "liveOnlyById",
        "liveCalcOnly",
        "displayNameDifferences",
        "missingFields",
        "incompatibleValues",
    ]:
        require(
            failures,
            isinstance(counts[count_name], int) and counts[count_name] >= 0,
            f"moves gap count {count_name} must be a non-negative integer",
        )


def require(failures: list[str], condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
