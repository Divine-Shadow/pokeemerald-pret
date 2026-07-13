#!/usr/bin/env python3
"""Audit source-to-output coverage for generated tracker exports."""

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
        help="Coverage report path. Defaults to coverage_report.json in output-dir.",
    )
    args = parser.parse_args()

    output_dir = resolve_from_root(args.output_dir)
    report_path = resolve_from_root(args.report) if args.report else output_dir / "coverage_report.json"
    tracker_data = load_json(output_dir / "tracker_data.json")

    report = audit_coverage(tracker_data)
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


def audit_coverage(tracker_data: OrderedDict) -> OrderedDict:
    failures: list[OrderedDict] = []
    trainer_result = audit_trainers(tracker_data, failures)
    location_result = audit_locations(tracker_data, failures)

    return OrderedDict(
        [
            ("schemaVersion", 1),
            ("sources", tracker_data.get("sources", OrderedDict())),
            (
                "counts",
                OrderedDict(
                    [
                        ("trainers", trainer_result["counts"]),
                        ("locations", location_result["counts"]),
                    ]
                ),
            ),
            (
                "skips",
                OrderedDict(
                    [
                        ("trainers", trainer_result["skips"]),
                        ("wildEncounterGroups", location_result["groupSkips"]),
                        ("wildEncounterHeaders", location_result["headerSkips"]),
                    ]
                ),
            ),
            ("failures", failures),
        ]
    )


def audit_trainers(tracker_data: OrderedDict, failures: list[OrderedDict]) -> OrderedDict:
    section_ids = trainer_section_ids()
    for trainer_id, count in sorted(Counter(section_ids).items()):
        if count > 1:
            add_failure(
                failures,
                "duplicate_trainer_id",
                f"{trainer_id} appears {count} times in {exporter.relpath(exporter.TRAINERS_PATH)}",
                trainerId=trainer_id,
                count=count,
            )

    expected_trainers, expected_setdex, expected_party_order = exporter.parse_trainers(
        exporter.TRAINERS_PATH
    )
    actual_trainers = tracker_data.get("trainers", OrderedDict())
    actual_setdex = tracker_data.get("setdex", OrderedDict())
    actual_party_order = tracker_data.get("partyOrder", OrderedDict())

    compare_keys(
        failures,
        "trainers",
        expected_trainers.keys(),
        actual_trainers.keys(),
        "trainer_id",
    )
    compare_keys(
        failures,
        "partyOrder",
        expected_party_order.keys(),
        actual_party_order.keys(),
        "trainer_label",
    )
    compare_keys(
        failures,
        "setdex_species",
        expected_setdex.keys(),
        actual_setdex.keys(),
        "species",
    )

    skips = []
    for trainer_id, trainer in expected_trainers.items():
        validate_trainer_record(failures, trainer_id, trainer)
        actual_trainer = actual_trainers.get(trainer_id)
        if actual_trainer is not None and not same_json_value(actual_trainer, trainer):
            add_failure(
                failures,
                "trainer_payload_mismatch",
                f"{trainer_id} generated trainer payload does not match parsed source",
                trainerId=trainer_id,
            )

        label = trainer["label"]
        party = trainer["party"]
        if not party:
            skips.append(
                OrderedDict(
                    [
                        ("trainerId", trainer_id),
                        ("label", label),
                        ("reason", "empty_party_no_pkcalc_sets"),
                    ]
                )
            )
            continue

        expected_species = [mon["species"] for mon in party]
        actual_species = actual_party_order.get(label)
        if actual_species != expected_species:
            add_failure(
                failures,
                "party_order_mismatch",
                f"{label} partyOrder species do not match parsed party",
                trainerId=trainer_id,
                label=label,
                expected=expected_species,
                actual=actual_species,
            )

    for species, expected_sets in expected_setdex.items():
        actual_sets = actual_setdex.get(species, OrderedDict())
        compare_keys(
            failures,
            f"setdex.{species}",
            expected_sets.keys(),
            actual_sets.keys(),
            "set_label",
            species=species,
        )
        for set_label, expected_set in expected_sets.items():
            actual_set = actual_sets.get(set_label)
            if not same_json_value(actual_set, expected_set):
                add_failure(
                    failures,
                    "setdex_payload_mismatch",
                    f"{species} / {set_label} generated set does not match parsed source",
                    species=species,
                    setLabel=set_label,
                )

    expected_set_count = sum(len(sets) for sets in expected_setdex.values())
    actual_set_count = sum(len(sets) for sets in actual_setdex.values())
    return OrderedDict(
        [
            (
                "counts",
                OrderedDict(
                    [
                        ("sourceSections", len(section_ids)),
                        ("uniqueSourceSections", len(expected_trainers)),
                        ("generatedTrainers", len(actual_trainers)),
                        ("nonEmptySourceParties", len(expected_party_order)),
                        ("emptyPartySkips", len(skips)),
                        ("generatedPartyOrderEntries", len(actual_party_order)),
                        ("expectedSetdexSpecies", len(expected_setdex)),
                        ("generatedSetdexSpecies", len(actual_setdex)),
                        ("expectedSetdexSets", expected_set_count),
                        ("generatedSetdexSets", actual_set_count),
                    ]
                ),
            ),
            ("skips", skips),
        ]
    )


def trainer_section_ids() -> list[str]:
    source = exporter.strip_c_comments(exporter.TRAINERS_PATH.read_text())
    section_pattern = re.compile(r"^===\s+([A-Z0-9_]+)\s+===$", re.MULTILINE)
    return [match.group(1) for match in section_pattern.finditer(source)]


def validate_trainer_record(
    failures: list[OrderedDict], trainer_id: str, trainer: OrderedDict
) -> None:
    if trainer.get("id") != trainer_id:
        add_failure(
            failures,
            "malformed_trainer_record",
            f"{trainer_id} parsed trainer id is malformed",
            trainerId=trainer_id,
            actual=trainer.get("id"),
        )
    if not trainer.get("label"):
        add_failure(
            failures,
            "malformed_trainer_record",
            f"{trainer_id} parsed trainer label is empty",
            trainerId=trainer_id,
        )

    for index, mon in enumerate(trainer.get("party", [])):
        species = mon.get("species", "")
        if not species:
            add_failure(
                failures,
                "malformed_trainer_party",
                f"{trainer_id} party slot {index + 1} has no species",
                trainerId=trainer_id,
                partyIndex=index,
            )

        level = mon.get("level")
        if not isinstance(level, int) or level < 1 or level > 100:
            add_failure(
                failures,
                "malformed_trainer_party",
                f"{trainer_id} party slot {index + 1} has invalid level {level}",
                trainerId=trainer_id,
                partyIndex=index,
                level=level,
            )

        moves = mon.get("moves")
        if not isinstance(moves, list) or len(moves) > 4:
            add_failure(
                failures,
                "malformed_trainer_party",
                f"{trainer_id} party slot {index + 1} has invalid move list",
                trainerId=trainer_id,
                partyIndex=index,
                moves=moves,
            )

        validate_stat_block(failures, trainer_id, index, mon, "ivs", 0, 31)
        validate_stat_block(failures, trainer_id, index, mon, "evs", 0, 252)


def validate_stat_block(
    failures: list[OrderedDict],
    trainer_id: str,
    party_index: int,
    mon: OrderedDict,
    stat_key: str,
    minimum: int,
    maximum: int,
) -> None:
    stats = mon.get(stat_key)
    expected_keys = list(exporter.DEFAULT_IVS.keys())
    if not isinstance(stats, dict) or sorted(stats.keys()) != sorted(expected_keys):
        add_failure(
            failures,
            "malformed_trainer_party",
            f"{trainer_id} party slot {party_index + 1} has malformed {stat_key}",
            trainerId=trainer_id,
            partyIndex=party_index,
            statBlock=stat_key,
            actual=stats,
        )
        return

    invalid = {
        key: value
        for key, value in stats.items()
        if not isinstance(value, int) or value < minimum or value > maximum
    }
    if invalid:
        add_failure(
            failures,
            "malformed_trainer_party",
            f"{trainer_id} party slot {party_index + 1} has out-of-range {stat_key}",
            trainerId=trainer_id,
            partyIndex=party_index,
            statBlock=stat_key,
            invalid=invalid,
        )


def compare_keys(
    failures: list[OrderedDict],
    area: str,
    expected_keys,
    actual_keys,
    key_name: str,
    **extra: object,
) -> None:
    expected = set(expected_keys)
    actual = set(actual_keys)
    for key in sorted(expected - actual):
        add_failure(
            failures,
            f"missing_{area}",
            f"{area} is missing expected {key_name} {key}",
            **extra,
            **{key_name: key},
        )
    for key in sorted(actual - expected):
        add_failure(
            failures,
            f"unexpected_{area}",
            f"{area} has unexpected {key_name} {key}",
            **extra,
            **{key_name: key},
        )


def audit_locations(tracker_data: OrderedDict, failures: list[OrderedDict]) -> OrderedDict:
    actual_locations = tracker_data.get("locations", OrderedDict())
    region_sections = exporter.load_region_sections()
    section_to_location_id, section_coords, duplicate_location_ids = build_section_index(
        region_sections
    )
    for location_id, sections in duplicate_location_ids.items():
        add_failure(
            failures,
            "duplicate_location_id",
            f"region map sections produce duplicate location id {location_id}",
            locationId=location_id,
            sections=sections,
        )

    compare_keys(
        failures,
        "locations",
        section_to_location_id.values(),
        actual_locations.keys(),
        "location_id",
    )

    map_id_to_data, duplicate_map_ids = load_map_jsons()
    for map_id, paths in duplicate_map_ids.items():
        add_failure(
            failures,
            "duplicate_map_id",
            f"{map_id} appears in multiple map.json files",
            mapId=map_id,
            paths=paths,
        )

    expected_source_maps = defaultdict(list)
    maps_with_known_sections = 0
    maps_with_coord_sections = 0
    for map_id, map_data in map_id_to_data.items():
        section_id = map_data.get("region_map_section")
        location_id = section_to_location_id.get(section_id)
        if not location_id:
            continue
        maps_with_known_sections += 1
        expected_source_maps[location_id].append(map_id)
        if section_coords.get(section_id):
            maps_with_coord_sections += 1

    for location_id, expected_maps in expected_source_maps.items():
        actual_maps = actual_locations.get(location_id, {}).get("sourceMapIds", [])
        if sorted(actual_maps) != sorted(expected_maps):
            add_failure(
                failures,
                "source_map_ids_mismatch",
                f"{location_id} sourceMapIds do not match map.json region sections",
                locationId=location_id,
                expected=sorted(expected_maps),
                actual=sorted(actual_maps),
            )

    wild_result = audit_wild_encounters(
        tracker_data,
        failures,
        map_id_to_data,
        section_to_location_id,
        section_coords,
    )

    actual_encounter_slots = sum(
        len(location.get("encounters", [])) for location in actual_locations.values()
    )
    return OrderedDict(
        [
            (
                "counts",
                OrderedDict(
                    [
                        ("regionSections", len(region_sections)),
                        ("regionSectionsWithCoords", sum(1 for coords in section_coords.values() if coords)),
                        ("generatedLocations", len(actual_locations)),
                        ("mapJsonFiles", len(map_id_to_data)),
                        ("mapsWithKnownRegionSections", maps_with_known_sections),
                        ("mapsWithCoordinateSections", maps_with_coord_sections),
                        ("generatedLocationsWithSourceMaps", sum(1 for loc in actual_locations.values() if loc.get("sourceMapIds"))),
                        ("generatedLocationsWithEncounters", sum(1 for loc in actual_locations.values() if loc.get("encounters"))),
                        ("wildEncounterGroups", wild_result["wildEncounterGroups"]),
                        ("wildEncounterGroupsSkipped", len(wild_result["groupSkips"])),
                        ("wildEncounterHeaders", wild_result["wildEncounterHeaders"]),
                        ("mapBackedWildEncounterHeaders", wild_result["mapBackedWildEncounterHeaders"]),
                        ("wildEncounterHeadersSkipped", wild_result["wildEncounterHeadersSkipped"]),
                        ("expectedEncounterSlots", wild_result["expectedEncounterSlots"]),
                        ("generatedEncounterSlots", actual_encounter_slots),
                    ]
                ),
            ),
            ("groupSkips", wild_result["groupSkips"]),
            ("headerSkips", wild_result["headerSkips"]),
        ]
    )


def build_section_index(
    region_sections: list[dict],
) -> tuple[OrderedDict, dict[str, list[list[int]]], OrderedDict]:
    section_to_location_id = OrderedDict()
    section_coords = {}
    location_sources = defaultdict(list)

    for section in region_sections:
        location_id = exporter.to_id(exporter.strip_mapsec_prefix(section["id"]))
        section_to_location_id[section["id"]] = location_id
        section_coords[section["id"]] = exporter.expand_coords(section)
        location_sources[location_id].append(section["id"])

    duplicates = OrderedDict(
        (location_id, sections)
        for location_id, sections in sorted(location_sources.items())
        if len(sections) > 1
    )
    return section_to_location_id, section_coords, duplicates


def load_map_jsons() -> tuple[OrderedDict, OrderedDict]:
    map_id_to_data = OrderedDict()
    map_id_to_paths = defaultdict(list)
    for map_path in sorted(exporter.MAPS_ROOT.glob("*/map.json")):
        map_data = load_json(map_path)
        map_id = map_data["id"]
        map_id_to_paths[map_id].append(exporter.relpath(map_path))
        if map_id not in map_id_to_data:
            map_id_to_data[map_id] = map_data

    duplicates = OrderedDict(
        (map_id, paths)
        for map_id, paths in sorted(map_id_to_paths.items())
        if len(paths) > 1
    )
    return map_id_to_data, duplicates


def audit_wild_encounters(
    tracker_data: OrderedDict,
    failures: list[OrderedDict],
    map_id_to_data: OrderedDict,
    section_to_location_id: OrderedDict,
    section_coords: dict[str, list[list[int]]],
) -> OrderedDict:
    wild_data = load_json(exporter.WILD_ENCOUNTERS_PATH)
    expected_by_location: dict[str, Counter] = defaultdict(Counter)
    expected_slots_by_location = defaultdict(int)
    group_skips = []
    header_skips = []
    wild_headers = 0
    map_backed_headers = 0
    skipped_headers = 0

    for group_index, group in enumerate(wild_data["wild_encounter_groups"]):
        encounters = group.get("encounters", [])
        wild_headers += len(encounters)
        group_label = group.get("label", f"group_{group_index}")
        if not group.get("for_maps"):
            skipped_headers += len(encounters)
            group_skips.append(
                OrderedDict(
                    [
                        ("group", group_label),
                        ("reason", "non_map_wild_encounter_group"),
                        ("headers", len(encounters)),
                    ]
                )
            )
            continue

        fields = {field["type"]: field for field in group.get("fields", [])}
        if not fields:
            add_failure(
                failures,
                "malformed_wild_group",
                f"{group_label} is map-backed but has no fields",
                group=group_label,
            )
            continue

        for header_index, header in enumerate(encounters):
            map_id = header.get("map")
            base_label = header.get("base_label", "")
            skip = wild_header_skip(
                header,
                map_id_to_data,
                section_to_location_id,
                section_coords,
            )
            if skip:
                skipped_headers += 1
                header_skips.append(
                    OrderedDict(
                        [
                            ("group", group_label),
                            ("headerIndex", header_index),
                            ("baseLabel", base_label),
                            ("map", map_id or ""),
                            ("reason", skip),
                        ]
                    )
                )
                continue

            map_data = map_id_to_data[map_id]
            section_id = map_data["region_map_section"]
            location_id = section_to_location_id[section_id]
            expected_slots, slot_failures = expected_encounter_slots(
                group_label, header_index, header, fields
            )
            failures.extend(slot_failures)
            if slot_failures:
                continue

            map_backed_headers += 1
            expected_slots_by_location[location_id] += len(expected_slots)
            expected_by_location[location_id].update(
                encounter_key(slot) for slot in expected_slots
            )

    compare_expected_encounters(tracker_data, failures, expected_by_location)

    return OrderedDict(
        [
            ("wildEncounterGroups", len(wild_data["wild_encounter_groups"])),
            ("wildEncounterHeaders", wild_headers),
            ("mapBackedWildEncounterHeaders", map_backed_headers),
            ("wildEncounterHeadersSkipped", skipped_headers),
            ("expectedEncounterSlots", sum(expected_slots_by_location.values())),
            ("groupSkips", group_skips),
            ("headerSkips", header_skips),
        ]
    )


def wild_header_skip(
    header: OrderedDict,
    map_id_to_data: OrderedDict,
    section_to_location_id: OrderedDict,
    section_coords: dict[str, list[list[int]]],
) -> str:
    map_id = header.get("map")
    if not map_id:
        return "missing_map_id"
    map_data = map_id_to_data.get(map_id)
    if not map_data:
        return "missing_map_json"
    section_id = map_data.get("region_map_section")
    if not section_id or section_id not in section_to_location_id:
        return "missing_region_map_section"
    if not section_coords.get(section_id):
        return "region_section_without_coordinates"
    return ""


def expected_encounter_slots(
    group_label: str,
    header_index: int,
    header: OrderedDict,
    fields: dict[str, OrderedDict],
) -> tuple[list[OrderedDict], list[OrderedDict]]:
    slots = []
    failures: list[OrderedDict] = []
    for field_type, method in exporter.ENCOUNTER_METHODS.items():
        if field_type in header:
            append_expected_slots(
                slots,
                failures,
                group_label,
                header_index,
                field_type,
                header[field_type],
                fields.get(field_type),
                method,
            )
    if "fishing_mons" in header:
        append_expected_fishing_slots(
            slots,
            failures,
            group_label,
            header_index,
            header["fishing_mons"],
            fields.get("fishing_mons"),
        )
    if not slots:
        add_failure(
            failures,
            "malformed_wild_header",
            f"{group_label}[{header_index}] has no supported encounter slots",
            group=group_label,
            headerIndex=header_index,
            map=header.get("map", ""),
        )
    return slots, failures


def append_expected_slots(
    slots: list[OrderedDict],
    failures: list[OrderedDict],
    group_label: str,
    header_index: int,
    field_type: str,
    source: OrderedDict,
    field: OrderedDict | None,
    method: str,
) -> None:
    if field is None:
        add_failure(
            failures,
            "missing_wild_field",
            f"{group_label}[{header_index}] uses {field_type} but the group has no field definition",
            group=group_label,
            headerIndex=header_index,
            fieldType=field_type,
        )
        return

    mons = source.get("mons", [])
    rates = field.get("encounter_rates", [])
    if len(mons) != len(rates):
        add_failure(
            failures,
            "wild_field_rate_mismatch",
            f"{group_label}[{header_index}] {field_type} has {len(mons)} mons but {len(rates)} rates",
            group=group_label,
            headerIndex=header_index,
            fieldType=field_type,
            mons=len(mons),
            rates=len(rates),
        )
        return

    for index, mon in enumerate(mons):
        slots.append(exporter.make_encounter(mon, rates[index], method))


def append_expected_fishing_slots(
    slots: list[OrderedDict],
    failures: list[OrderedDict],
    group_label: str,
    header_index: int,
    source: OrderedDict,
    field: OrderedDict | None,
) -> None:
    if field is None:
        add_failure(
            failures,
            "missing_wild_field",
            f"{group_label}[{header_index}] uses fishing_mons but the group has no field definition",
            group=group_label,
            headerIndex=header_index,
            fieldType="fishing_mons",
        )
        return

    mons = source.get("mons", [])
    rates = field.get("encounter_rates", [])
    if len(mons) != len(rates):
        add_failure(
            failures,
            "wild_field_rate_mismatch",
            f"{group_label}[{header_index}] fishing_mons has {len(mons)} mons but {len(rates)} rates",
            group=group_label,
            headerIndex=header_index,
            fieldType="fishing_mons",
            mons=len(mons),
            rates=len(rates),
        )
        return

    for group_name, indexes in field.get("groups", {}).items():
        method = exporter.FISHING_METHODS.get(group_name)
        if not method:
            add_failure(
                failures,
                "unsupported_fishing_group",
                f"{group_label}[{header_index}] has unsupported fishing group {group_name}",
                group=group_label,
                headerIndex=header_index,
                fieldType="fishing_mons",
                groupName=group_name,
            )
            continue
        for index in indexes:
            if index >= len(mons):
                add_failure(
                    failures,
                    "wild_field_index_out_of_range",
                    f"{group_label}[{header_index}] fishing index {index} is outside mons list",
                    group=group_label,
                    headerIndex=header_index,
                    fieldType="fishing_mons",
                    index=index,
                    mons=len(mons),
                )
                continue
            slots.append(exporter.make_encounter(mons[index], rates[index], method))


def compare_expected_encounters(
    tracker_data: OrderedDict,
    failures: list[OrderedDict],
    expected_by_location: dict[str, Counter],
) -> None:
    actual_locations = tracker_data.get("locations", OrderedDict())
    for location_id, expected_counter in sorted(expected_by_location.items()):
        location = actual_locations.get(location_id)
        if not location:
            add_failure(
                failures,
                "missing_location_for_wild_encounters",
                f"{location_id} is missing but has map-backed wild encounter slots",
                locationId=location_id,
                expectedSlots=sum(expected_counter.values()),
            )
            continue
        actual_counter = Counter(encounter_key(enc) for enc in location.get("encounters", []))
        for key, expected_count in sorted(expected_counter.items()):
            actual_count = actual_counter.get(key, 0)
            if actual_count < expected_count:
                add_failure(
                    failures,
                    "missing_wild_encounter_slots",
                    f"{location_id} is missing {expected_count - actual_count} generated encounter slot(s)",
                    locationId=location_id,
                    encounter=json.loads(key, object_pairs_hook=OrderedDict),
                    expected=expected_count,
                    actual=actual_count,
                )
        for key, actual_count in sorted(actual_counter.items()):
            expected_count = expected_counter.get(key, 0)
            if actual_count > expected_count:
                add_failure(
                    failures,
                    "unexpected_wild_encounter_slots",
                    f"{location_id} has {actual_count - expected_count} unexpected generated encounter slot(s)",
                    locationId=location_id,
                    encounter=json.loads(key, object_pairs_hook=OrderedDict),
                    expected=expected_count,
                    actual=actual_count,
                )


def encounter_key(encounter: OrderedDict) -> str:
    return json.dumps(encounter, sort_keys=True, separators=(",", ":"))


def same_json_value(left: object, right: object) -> bool:
    return json.dumps(left, sort_keys=True) == json.dumps(right, sort_keys=True)


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
    trainer_counts = report["counts"]["trainers"]
    location_counts = report["counts"]["locations"]
    print(f"Wrote {relative_or_abs(report_path)}")
    print(
        "Trainer coverage: "
        f"{trainer_counts['generatedTrainers']}/{trainer_counts['uniqueSourceSections']} trainers, "
        f"{trainer_counts['generatedPartyOrderEntries']}/{trainer_counts['nonEmptySourceParties']} parties, "
        f"{trainer_counts['generatedSetdexSets']}/{trainer_counts['expectedSetdexSets']} sets, "
        f"{trainer_counts['emptyPartySkips']} skipped empty-party trainer(s)"
    )
    print(
        "Location coverage: "
        f"{location_counts['mapBackedWildEncounterHeaders']} map-backed wild header(s), "
        f"{location_counts['generatedEncounterSlots']}/{location_counts['expectedEncounterSlots']} encounter slots, "
        f"{location_counts['wildEncounterGroupsSkipped']} skipped non-map group(s), "
        f"{location_counts['wildEncounterHeadersSkipped']} skipped unsupported header(s)"
    )
    if report["failures"]:
        print(f"Coverage audit failed: {len(report['failures'])} failure(s)")
        for failure in report["failures"][:10]:
            print(f"- {failure['code']}: {failure['message']}")
    else:
        print("Coverage audit passed: no missing or unexpected tracker records")


def relative_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
