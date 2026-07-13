#!/usr/bin/env python3
"""Audit reference integrity for generated tracker exports."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, OrderedDict, defaultdict
from pathlib import Path

import export_tracker_data as exporter


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT_DIR = REPO_ROOT / "build/tracker_export"
CATEGORIES = [
    "species",
    "moves",
    "abilities",
    "items",
    "natures",
    "types",
    "encounterSpecies",
]
DISPLAY_CATEGORIES = {"species", "moves", "abilities", "items", "natures", "types"}
ID_CATEGORIES = {"encounterSpecies"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        default=DEFAULT_OUTPUT_DIR,
        type=Path,
        help="Directory containing generated tracker_data.json.",
    )
    parser.add_argument(
        "--report",
        type=Path,
        help="Reference report path. Defaults to reference_report.json in output-dir.",
    )
    args = parser.parse_args()

    output_dir = resolve_from_root(args.output_dir)
    report_path = resolve_from_root(args.report) if args.report else output_dir / "reference_report.json"
    tracker_data = load_json(output_dir / "tracker_data.json")

    report = audit_references(tracker_data)
    write_json(report_path, report)
    print_summary(report, report_path)

    if report["failures"]:
        sys.exit(1)


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def load_json(path: Path) -> OrderedDict:
    return json.loads(path.read_text(), object_pairs_hook=OrderedDict)


def audit_references(tracker_data: OrderedDict) -> OrderedDict:
    failures: list[OrderedDict] = []
    generated = collect_generated_references(tracker_data, failures)
    expected = collect_source_references()
    categories = OrderedDict()

    for category in CATEGORIES:
        values = generated[category]
        collisions = normalized_collisions(values)
        for collision in collisions:
            add_failure(
                failures,
                "normalized_reference_collision",
                f"{category} has conflicting values for normalized key {collision['normalized']}",
                category=category,
                normalized=collision["normalized"],
                values=collision["values"],
            )

        missing = sorted(set(expected[category]) - set(values))
        unexpected = sorted(set(values) - set(expected[category]))
        for value in missing:
            add_failure(
                failures,
                "missing_source_reference",
                f"{category} is missing source-derived reference {value}",
                category=category,
                value=value,
            )
        for value in unexpected:
            add_failure(
                failures,
                "unexpected_generated_reference",
                f"{category} has generated reference not present in source-derived expectations: {value}",
                category=category,
                value=value,
            )

        categories[category] = OrderedDict(
            [
                ("count", len(values)),
                ("occurrences", sum(values.values())),
                ("normalizedCount", len({normalize_reference(value) for value in values})),
                ("values", values_report(values)),
                ("collisions", collisions),
                (
                    "sourceComparison",
                    OrderedDict(
                        [
                            ("expectedCount", len(expected[category])),
                            ("missingFromGenerated", missing),
                            ("unexpectedInGenerated", unexpected),
                        ]
                    ),
                ),
            ]
        )

    cross_checks = cross_check_generated_data(tracker_data, failures)
    return OrderedDict(
        [
            ("schemaVersion", 1),
            ("auditSource", "generated tracker_data.json plus exporter source-derived parsing"),
            ("sources", tracker_data.get("sources", OrderedDict())),
            ("categories", categories),
            ("crossChecks", cross_checks),
            ("failures", failures),
        ]
    )


def collect_generated_references(
    tracker_data: OrderedDict, failures: list[OrderedDict]
) -> dict[str, Counter]:
    refs = empty_counters()
    setdex = tracker_data.get("setdex", OrderedDict())
    party_order = tracker_data.get("partyOrder", OrderedDict())
    locations = tracker_data.get("locations", OrderedDict())

    if not isinstance(setdex, dict):
        add_failure(failures, "malformed_setdex", "tracker_data.json setdex must be an object")
        setdex = OrderedDict()
    if not isinstance(party_order, dict):
        add_failure(
            failures,
            "malformed_party_order",
            "tracker_data.json partyOrder must be an object",
        )
        party_order = OrderedDict()
    if not isinstance(locations, dict):
        add_failure(
            failures,
            "malformed_locations",
            "tracker_data.json locations must be an object",
        )
        locations = OrderedDict()

    for species, sets in setdex.items():
        add_reference(refs, failures, "species", species, f"setdex.{species}")
        if not isinstance(sets, dict):
            add_failure(
                failures,
                "malformed_setdex_species",
                f"setdex.{species} must be an object",
                species=species,
            )
            continue
        for set_label, mon_set in sets.items():
            if not is_nonempty_string(set_label):
                add_failure(
                    failures,
                    "malformed_set_label",
                    f"setdex.{species} has an empty or malformed set label",
                    species=species,
                    setLabel=set_label,
                )
            if not isinstance(mon_set, dict):
                add_failure(
                    failures,
                    "malformed_setdex_set",
                    f"setdex.{species}.{set_label} must be an object",
                    species=species,
                    setLabel=set_label,
                )
                continue
            add_optional_reference(refs, failures, "abilities", mon_set, "ability", species, set_label)
            add_optional_reference(refs, failures, "items", mon_set, "item", species, set_label)
            add_required_reference(refs, failures, "natures", mon_set, "nature", species, set_label)
            add_optional_reference(refs, failures, "types", mon_set, "teraType", species, set_label)
            moves = mon_set.get("moves")
            if not isinstance(moves, list):
                add_failure(
                    failures,
                    "malformed_move_list",
                    f"setdex.{species}.{set_label} moves must be a list",
                    species=species,
                    setLabel=set_label,
                )
                continue
            for index, move in enumerate(moves):
                add_reference(
                    refs,
                    failures,
                    "moves",
                    move,
                    f"setdex.{species}.{set_label}.moves[{index}]",
                )

    for label, species_list in party_order.items():
        if not is_nonempty_string(label):
            add_failure(
                failures,
                "malformed_party_order_label",
                "partyOrder has an empty or malformed trainer label",
                label=label,
            )
        if not isinstance(species_list, list):
            add_failure(
                failures,
                "malformed_party_order_species",
                f"partyOrder.{label} must be a list",
                label=label,
            )
            continue
        for index, species in enumerate(species_list):
            add_reference(
                refs,
                failures,
                "species",
                species,
                f"partyOrder.{label}[{index}]",
            )

    for location_id, location in locations.items():
        if not isinstance(location, dict):
            add_failure(
                failures,
                "malformed_location",
                f"locations.{location_id} must be an object",
                locationId=location_id,
            )
            continue
        encounters = location.get("encounters", [])
        if not isinstance(encounters, list):
            add_failure(
                failures,
                "malformed_location_encounters",
                f"locations.{location_id}.encounters must be a list",
                locationId=location_id,
            )
            continue
        for index, encounter in enumerate(encounters):
            if not isinstance(encounter, dict):
                add_failure(
                    failures,
                    "malformed_encounter",
                    f"locations.{location_id}.encounters[{index}] must be an object",
                    locationId=location_id,
                    encounterIndex=index,
                )
                continue
            add_reference(
                refs,
                failures,
                "encounterSpecies",
                encounter.get("species"),
                f"locations.{location_id}.encounters[{index}].species",
            )

    return refs


def collect_source_references() -> dict[str, Counter]:
    refs = empty_counters()
    trainers, setdex, party_order = exporter.parse_trainers(exporter.TRAINERS_PATH)
    locations = exporter.build_locations()

    for trainer in trainers.values():
        for mon in trainer["party"]:
            refs["species"][mon["species"]] += 1
            if mon["ability"]:
                refs["abilities"][mon["ability"]] += 1
            if mon["item"]:
                refs["items"][mon["item"]] += 1
            refs["natures"][mon["nature"]] += 1
            if "teraType" in mon and mon["teraType"]:
                refs["types"][mon["teraType"]] += 1
            for move in mon["moves"]:
                refs["moves"][move] += 1

    for species in setdex:
        refs["species"][species] += 1
    for species_list in party_order.values():
        for species in species_list:
            refs["species"][species] += 1

    for location in locations.values():
        for encounter in location["encounters"]:
            refs["encounterSpecies"][encounter["species"]] += 1

    return refs


def empty_counters() -> dict[str, Counter]:
    return {category: Counter() for category in CATEGORIES}


def add_optional_reference(
    refs: dict[str, Counter],
    failures: list[OrderedDict],
    category: str,
    mon_set: dict,
    field: str,
    species: str,
    set_label: str,
) -> None:
    if field not in mon_set:
        return
    add_reference(
        refs,
        failures,
        category,
        mon_set.get(field),
        f"setdex.{species}.{set_label}.{field}",
    )


def add_required_reference(
    refs: dict[str, Counter],
    failures: list[OrderedDict],
    category: str,
    mon_set: dict,
    field: str,
    species: str,
    set_label: str,
) -> None:
    add_reference(
        refs,
        failures,
        category,
        mon_set.get(field),
        f"setdex.{species}.{set_label}.{field}",
    )


def add_reference(
    refs: dict[str, Counter],
    failures: list[OrderedDict],
    category: str,
    value: object,
    path: str,
) -> None:
    if not is_nonempty_string(value):
        add_failure(
            failures,
            "empty_reference",
            f"{path} must be a non-empty string reference",
            category=category,
            path=path,
            value=value,
        )
        return

    if value != value.strip() or has_control_chars(value):
        add_failure(
            failures,
            "malformed_reference",
            f"{path} has whitespace padding or control characters",
            category=category,
            path=path,
            value=value,
        )
        return

    if category in DISPLAY_CATEGORIES and not looks_like_display_reference(value):
        add_failure(
            failures,
            "malformed_reference",
            f"{path} does not look like a PKCalc display reference",
            category=category,
            path=path,
            value=value,
        )
    if category in ID_CATEGORIES and not re.fullmatch(r"[a-z0-9]+", value):
        add_failure(
            failures,
            "malformed_reference",
            f"{path} does not look like a PKCalc id reference",
            category=category,
            path=path,
            value=value,
        )

    refs[category][value] += 1


def is_nonempty_string(value: object) -> bool:
    return isinstance(value, str) and bool(value.strip())


def has_control_chars(value: str) -> bool:
    return any(ord(ch) < 32 for ch in value)


def looks_like_display_reference(value: str) -> bool:
    return bool(re.search(r"[A-Za-z0-9]", value))


def normalized_collisions(values: Counter) -> list[OrderedDict]:
    buckets = defaultdict(list)
    for value in values:
        buckets[normalize_reference(value)].append(value)

    collisions = []
    for normalized, bucket in sorted(buckets.items()):
        if len(bucket) > 1:
            collisions.append(
                OrderedDict(
                    [
                        ("normalized", normalized),
                        ("values", sorted(bucket)),
                    ]
                )
            )
    return collisions


def normalize_reference(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def values_report(values: Counter) -> OrderedDict:
    return OrderedDict(
        (
            value,
            OrderedDict(
                [
                    ("occurrences", values[value]),
                    ("normalized", normalize_reference(value)),
                ]
            ),
        )
        for value in sorted(values)
    )


def cross_check_generated_data(
    tracker_data: OrderedDict, failures: list[OrderedDict]
) -> OrderedDict:
    setdex = tracker_data.get("setdex", OrderedDict())
    party_order = tracker_data.get("partyOrder", OrderedDict())
    locations = tracker_data.get("locations", OrderedDict())
    setdex_species = set(setdex.keys()) if isinstance(setdex, dict) else set()
    party_species = set()

    if isinstance(party_order, dict):
        for label, species_list in party_order.items():
            if not isinstance(species_list, list):
                continue
            for species in species_list:
                if isinstance(species, str):
                    party_species.add(species)

    missing_setdex_species = sorted(party_species - setdex_species)
    for species in missing_setdex_species:
        add_failure(
            failures,
            "party_species_missing_setdex",
            f"partyOrder references {species}, but setdex has no species entry",
            species=species,
        )

    empty_locations = []
    malformed_methods = []
    if isinstance(locations, dict):
        for location_id, location in locations.items():
            if not isinstance(location, dict):
                continue
            encounters = location.get("encounters", [])
            if not encounters:
                empty_locations.append(location_id)
                continue
            for index, encounter in enumerate(encounters):
                method = encounter.get("method") if isinstance(encounter, dict) else None
                if not is_nonempty_string(method):
                    malformed_methods.append(f"{location_id}[{index}]")
                    add_failure(
                        failures,
                        "malformed_encounter_method",
                        f"locations.{location_id}.encounters[{index}].method must be non-empty",
                        locationId=location_id,
                        encounterIndex=index,
                    )

    return OrderedDict(
        [
            (
                "partySpeciesHaveSetdex",
                OrderedDict(
                    [
                        ("partySpecies", len(party_species)),
                        ("setdexSpecies", len(setdex_species)),
                        ("missingSetdexSpecies", missing_setdex_species),
                    ]
                ),
            ),
            (
                "locations",
                OrderedDict(
                    [
                        ("total", len(locations) if isinstance(locations, dict) else 0),
                        ("emptyEncounterLocations", len(empty_locations)),
                        ("malformedEncounterMethods", malformed_methods),
                    ]
                ),
            ),
        ]
    )


def add_failure(
    failures: list[OrderedDict],
    code: str,
    message: str,
    **details: object,
) -> None:
    failure = OrderedDict([("code", code), ("message", message)])
    for key, value in details.items():
        failure[key] = value
    failures.append(failure)


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


def print_summary(report: OrderedDict, report_path: Path) -> None:
    print(f"Wrote {relative_or_abs(report_path)}")
    parts = []
    for category in CATEGORIES:
        data = report["categories"][category]
        parts.append(f"{category}: {data['count']}")
    print("Reference categories: " + ", ".join(parts))
    if report["failures"]:
        print(f"Reference audit failed: {len(report['failures'])} failure(s)")
        for failure in report["failures"][:10]:
            print(f"- {failure['code']}: {failure['message']}")
    else:
        print("Reference audit passed: no malformed, missing, unexpected, or conflicting references")


def relative_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
