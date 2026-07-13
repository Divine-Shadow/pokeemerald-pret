#!/usr/bin/env python3
"""Export static tracker data in a PKCalc-compatible MVP shape."""

from __future__ import annotations

import argparse
import json
import re
from collections import OrderedDict, defaultdict
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]

TRAINERS_PATH = REPO_ROOT / "src/data/trainers.party"
WILD_ENCOUNTERS_PATH = REPO_ROOT / "src/data/wild_encounters.json"
REGION_MAP_PATH = REPO_ROOT / "src/data/region_map/region_map_sections.json"
MAPS_ROOT = REPO_ROOT / "data/maps"

STAT_KEYS = OrderedDict(
    [
        ("HP", "hp"),
        ("Atk", "at"),
        ("Def", "df"),
        ("SpA", "sa"),
        ("SpD", "sd"),
        ("Spe", "sp"),
    ]
)

DEFAULT_IVS = OrderedDict((key, 31) for key in STAT_KEYS.values())
DEFAULT_EVS = OrderedDict((key, 0) for key in STAT_KEYS.values())

CONSTANT_PREFIXES = (
    "SPECIES_",
    "ITEM_",
    "MOVE_",
    "ABILITY_",
    "NATURE_",
    "TYPE_",
)

SPECIAL_CONSTANT_NAMES = {
    "NIDORAN_F": "Nidoran-F",
    "NIDORAN_M": "Nidoran-M",
    "MR_MIME": "Mr. Mime",
    "MIME_JR": "Mime Jr.",
    "FARFETCHD": "Farfetch'd",
    "SIRFETCHD": "Sirfetch'd",
    "HO_OH": "Ho-Oh",
    "PORYGON_Z": "Porygon-Z",
    "JANGMO_O": "Jangmo-o",
    "HAKAMO_O": "Hakamo-o",
    "KOMMO_O": "Kommo-o",
    "U_TURN": "U-turn",
}

SPECIAL_DISPLAY_NAMES = {
    "Double Edge": "Double-Edge",
}

FISHING_METHODS = {
    "old_rod": "oldrod",
    "good_rod": "goodrod",
    "super_rod": "superrod",
}

ENCOUNTER_METHODS = {
    "land_mons": "grass",
    "water_mons": "surf",
    "rock_smash_mons": "rocksmash",
}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        default=REPO_ROOT / "build/tracker_export",
        type=Path,
        help="Directory for tracker_data.json and pkcalc adapter files.",
    )
    args = parser.parse_args()

    output_dir = args.output_dir
    if not output_dir.is_absolute():
        output_dir = REPO_ROOT / output_dir

    tracker_data = build_tracker_data()
    write_outputs(tracker_data, output_dir)


def build_tracker_data() -> OrderedDict:
    trainers, setdex, party_order = parse_trainers(TRAINERS_PATH)
    locations = build_locations()

    return OrderedDict(
        [
            ("schemaVersion", 1),
            (
                "sources",
                OrderedDict(
                    [
                        ("trainers", relpath(TRAINERS_PATH)),
                        ("wildEncounters", relpath(WILD_ENCOUNTERS_PATH)),
                        ("maps", relpath(MAPS_ROOT)),
                        ("regionMapSections", relpath(REGION_MAP_PATH)),
                    ]
                ),
            ),
            (
                "postMvpGaps",
                [
                    "Full species, move, item, and ability Dex exports are deferred.",
                    "Damage-calculator correctness data is deferred.",
                    "Lua/save sync and emulator integration are deferred.",
                ],
            ),
            ("trainers", trainers),
            ("setdex", setdex),
            ("partyOrder", party_order),
            ("locations", locations),
        ]
    )


def relpath(path: Path) -> str:
    return str(path.relative_to(REPO_ROOT))


def write_outputs(tracker_data: OrderedDict, output_dir: Path) -> None:
    pkcalc_dir = output_dir / "pkcalc"
    pkcalc_dir.mkdir(parents=True, exist_ok=True)

    write_json(output_dir / "tracker_data.json", tracker_data)
    write_js_constant(pkcalc_dir / "sets.js", "SETDEX_PK", tracker_data["setdex"])
    write_js_constant(
        pkcalc_dir / "party_order.js", "PARTY_ORDER_PK", tracker_data["partyOrder"]
    )
    write_js_constant(pkcalc_dir / "locations.js", "LOCATIONS", tracker_data["locations"])

    print(f"Wrote {relpath_or_abs(output_dir / 'tracker_data.json')}")
    print(f"Wrote {relpath_or_abs(pkcalc_dir / 'sets.js')}")
    print(f"Wrote {relpath_or_abs(pkcalc_dir / 'party_order.js')}")
    print(f"Wrote {relpath_or_abs(pkcalc_dir / 'locations.js')}")


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n")


def write_js_constant(path: Path, name: str, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(obj, indent=4, sort_keys=True)
    path.write_text(f"const {name} = {payload};\n")


def parse_trainers(path: Path) -> tuple[OrderedDict, OrderedDict, OrderedDict]:
    source = strip_c_comments(path.read_text())
    section_pattern = re.compile(r"^===\s+([A-Z0-9_]+)\s+===$", re.MULTILINE)
    matches = list(section_pattern.finditer(source))

    trainers = OrderedDict()
    setdex_nested: dict[str, OrderedDict] = defaultdict(OrderedDict)
    party_order = OrderedDict()

    for index, match in enumerate(matches):
        trainer_id = match.group(1)
        start = match.end()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(source)
        attrs, pokemon = parse_trainer_body(source[start:end])

        trainer_class = normalize_display_name(attrs.get("Class", "Pkmn Trainer"))
        trainer_name = normalize_trainer_name(attrs.get("Name", ""))
        label = make_trainer_label(trainer_class, trainer_name, trainer_id)

        trainer = OrderedDict(
            [
                ("id", trainer_id),
                ("label", label),
                ("class", trainer_class),
                ("name", trainer_name),
                ("battleType", attrs.get("Battle Type") or attrs.get("Double Battle", "")),
                ("aiFlags", split_slash_values(attrs.get("AI", ""))),
                ("party", pokemon),
            ]
        )
        trainers[trainer_id] = trainer

        if not pokemon:
            continue

        party_order[label] = [mon["species"] for mon in pokemon]
        species_seen: dict[str, int] = defaultdict(int)
        for mon in pokemon:
            species = mon["species"]
            species_seen[species] += 1
            set_label = label
            if species_seen[species] > 1:
                set_label = f"{label} #{species_seen[species]}"
            setdex_nested[species][set_label] = make_pkcalc_set(mon)

    setdex = OrderedDict((species, setdex_nested[species]) for species in sorted(setdex_nested))
    return trainers, setdex, party_order


def strip_c_comments(text: str) -> str:
    return re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)


def parse_trainer_body(body: str) -> tuple[OrderedDict, list[OrderedDict]]:
    chunks = split_chunks(body)
    if not chunks:
        return OrderedDict(), []

    attrs = parse_attrs(chunks[0])
    pokemon = [parse_pokemon_chunk(chunk) for chunk in chunks[1:]]
    pokemon = [mon for mon in pokemon if mon is not None]
    return attrs, pokemon


def split_chunks(text: str) -> list[list[str]]:
    chunks: list[list[str]] = []
    current: list[str] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            if current:
                chunks.append(current)
                current = []
            continue
        if line.startswith("#"):
            continue
        current.append(line)
    if current:
        chunks.append(current)
    return chunks


def parse_attrs(lines: list[str]) -> OrderedDict:
    attrs = OrderedDict()
    for line in lines:
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        attrs[key.strip()] = value.strip()
    return attrs


def parse_pokemon_chunk(lines: list[str]) -> OrderedDict | None:
    if not lines:
        return None

    header = parse_pokemon_header(lines[0])
    mon = OrderedDict(
        [
            ("species", normalize_display_name(header["species"])),
            ("nickname", header["nickname"]),
            ("gender", header["gender"]),
            ("item", normalize_display_name(header["item"]) if header["item"] else ""),
            ("ability", ""),
            ("level", 100),
            ("nature", "Hardy"),
            ("ivs", OrderedDict(DEFAULT_IVS)),
            ("evs", OrderedDict(DEFAULT_EVS)),
            ("moves", []),
        ]
    )

    for line in lines[1:]:
        if line.startswith("- "):
            mon["moves"].append(normalize_display_name(line[2:].strip()))
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        key = key.strip()
        value = value.strip()
        if key == "Ability":
            mon["ability"] = normalize_display_name(value)
        elif key == "Level":
            mon["level"] = int(value)
        elif key == "Nature":
            mon["nature"] = normalize_display_name(value)
        elif key == "IVs":
            mon["ivs"] = parse_stats(value, 31)
        elif key == "EVs":
            mon["evs"] = parse_stats(value, 0)
        elif key == "Tera Type":
            mon["teraType"] = normalize_display_name(value)

    return mon


def parse_pokemon_header(header: str) -> dict[str, str]:
    left, item = split_optional(header, "@")
    left = left.strip()
    item = item.strip()

    gender = ""
    gender_match = re.search(r"\s+\((M|F)\)\s*$", left)
    if gender_match:
        gender = "Male" if gender_match.group(1) == "M" else "Female"
        left = left[: gender_match.start()].strip()

    nickname = ""
    nickname_match = re.match(r"^(?P<nickname>.+?)\s+\((?P<species>[^()]+)\)$", left)
    if nickname_match:
        nickname = nickname_match.group("nickname").strip()
        species = nickname_match.group("species").strip()
    else:
        species = left

    return {"species": species, "nickname": nickname, "gender": gender, "item": item}


def split_optional(text: str, separator: str) -> tuple[str, str]:
    if separator not in text:
        return text, ""
    left, right = text.split(separator, 1)
    return left, right


def parse_stats(value: str, default: int) -> OrderedDict:
    stats = OrderedDict((key, default) for key in STAT_KEYS.values())
    for part in value.split("/"):
        match = re.match(r"\s*(\d+)\s+([A-Za-z]+)\s*$", part)
        if not match:
            continue
        number = int(match.group(1))
        stat_name = match.group(2)
        stat_key = STAT_KEYS.get(stat_name)
        if stat_key:
            stats[stat_key] = number
    return stats


def make_pkcalc_set(mon: OrderedDict) -> OrderedDict:
    result = OrderedDict()
    if mon["ability"]:
        result["ability"] = mon["ability"]
    if mon["gender"]:
        result["gender"] = mon["gender"]
    if mon["item"]:
        result["item"] = mon["item"]
    result["ivs"] = mon["ivs"]
    if any(value != 0 for value in mon["evs"].values()):
        result["evs"] = mon["evs"]
    result["level"] = mon["level"]
    result["moves"] = mon["moves"]
    result["nature"] = mon["nature"]
    if "teraType" in mon:
        result["teraType"] = mon["teraType"]
    return result


def normalize_display_name(value: str) -> str:
    value = value.strip()
    if not value:
        return ""
    for prefix in CONSTANT_PREFIXES:
        if value.startswith(prefix):
            value = value[len(prefix) :]
            break
    if value in SPECIAL_CONSTANT_NAMES:
        return SPECIAL_CONSTANT_NAMES[value]
    if "_" in value:
        value = " ".join(format_constant_word(word) for word in value.split("_"))
    else:
        value = re.sub(r"\s+", " ", value)
    return SPECIAL_DISPLAY_NAMES.get(value, value)


def format_constant_word(word: str) -> str:
    if word in {"HP", "PP", "TM", "HM"}:
        return word
    return word[:1].upper() + word[1:].lower()


def normalize_trainer_name(value: str) -> str:
    value = normalize_display_name(value)
    if not value:
        return ""
    letters = re.sub(r"[^A-Za-z]", "", value)
    if letters and letters.upper() == letters:
        return value.title()
    return value


def make_trainer_label(trainer_class: str, trainer_name: str, trainer_id: str) -> str:
    if trainer_name:
        base = f"{trainer_class} {trainer_name}"
    else:
        base = trainer_class
    return f"{base} [{trainer_id}]"


def split_slash_values(value: str) -> list[str]:
    if not value:
        return []
    return [normalize_display_name(part.strip()) for part in value.split("/") if part.strip()]


def build_locations() -> OrderedDict:
    region_sections = load_region_sections()
    locations = OrderedDict()
    section_to_location_id = {}

    for met_location_id, section in enumerate(region_sections):
        location_id = to_id(strip_mapsec_prefix(section["id"]))
        section_to_location_id[section["id"]] = location_id
        location_name = title_region_name(section["name"]) or title_region_name(
            strip_mapsec_prefix(section["id"]).replace("_", " ")
        )
        locations[location_id] = OrderedDict(
            [
                ("id", location_id),
                ("name", location_name),
                ("coords", expand_coords(section)),
                ("sublocations", []),
                ("desc", ""),
                ("encounters", []),
                ("metLocationId", met_location_id),
                ("search", True),
                ("mapSection", section["id"]),
                ("sourceMapIds", []),
            ]
        )

    map_id_to_location_id = load_map_location_ids(section_to_location_id, locations)
    append_wild_encounters(locations, map_id_to_location_id)

    return locations


def load_region_sections() -> list[dict]:
    data = json.loads(REGION_MAP_PATH.read_text())
    return data["map_sections"]


def strip_mapsec_prefix(section_id: str) -> str:
    prefix = "MAPSEC_"
    if section_id.startswith(prefix):
        return section_id[len(prefix) :]
    return section_id


def expand_coords(section: dict) -> list[list[int]]:
    if not all(key in section for key in ("x", "y", "width", "height")):
        return []

    coords = []
    for y in range(section["y"], section["y"] + section["height"]):
        for x in range(section["x"], section["x"] + section["width"]):
            coords.append([x, y])
    return coords


def title_region_name(name: str) -> str:
    if name.startswith("ROUTE "):
        return "Route " + name.split(" ", 1)[1]
    return " ".join(word[:1].upper() + word[1:].lower() for word in name.split())


def load_map_location_ids(
    section_to_location_id: dict[str, str], locations: OrderedDict
) -> dict[str, str]:
    map_id_to_location_id = {}
    for map_path in sorted(MAPS_ROOT.glob("*/map.json")):
        map_data = json.loads(map_path.read_text())
        section_id = map_data.get("region_map_section")
        location_id = section_to_location_id.get(section_id)
        if not location_id:
            continue
        map_id = map_data["id"]
        map_id_to_location_id[map_id] = location_id
        locations[location_id]["sourceMapIds"].append(map_id)
    return map_id_to_location_id


def append_wild_encounters(
    locations: OrderedDict, map_id_to_location_id: dict[str, str]
) -> None:
    data = json.loads(WILD_ENCOUNTERS_PATH.read_text())
    for group in data["wild_encounter_groups"]:
        if not group.get("for_maps"):
            continue
        fields = {field["type"]: field for field in group["fields"]}
        for encounter_header in group["encounters"]:
            location_id = map_id_to_location_id.get(encounter_header["map"])
            if not location_id:
                continue
            encounters = locations[location_id]["encounters"]
            for field_type, method in ENCOUNTER_METHODS.items():
                if field_type in encounter_header:
                    append_slots(encounters, encounter_header[field_type], fields[field_type], method)
            if "fishing_mons" in encounter_header:
                append_fishing_slots(
                    encounters,
                    encounter_header["fishing_mons"],
                    fields["fishing_mons"],
                )


def append_slots(
    destination: list[OrderedDict], source: dict, field: dict, method: str
) -> None:
    rates = field["encounter_rates"]
    for index, mon in enumerate(source["mons"]):
        destination.append(make_encounter(mon, rates[index], method))


def append_fishing_slots(destination: list[OrderedDict], source: dict, field: dict) -> None:
    rates = field["encounter_rates"]
    mons = source["mons"]
    for group_name, indexes in field["groups"].items():
        method = FISHING_METHODS[group_name]
        for index in indexes:
            destination.append(make_encounter(mons[index], rates[index], method))


def make_encounter(mon: dict, chance: int, method: str) -> OrderedDict:
    return OrderedDict(
        [
            ("species", species_id(mon["species"])),
            ("chance", chance),
            ("minLevel", mon["min_level"]),
            ("maxLevel", mon["max_level"]),
            ("method", method),
        ]
    )


def species_id(value: str) -> str:
    return to_id(normalize_display_name(value))


def to_id(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


if __name__ == "__main__":
    main()
