#!/usr/bin/env python3
"""Generate proof artifacts for PKCalc data migration."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import OrderedDict
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT_DIR = REPO_ROOT / "build/tracker_export/data_migration"
POKEMON_C = REPO_ROOT / "src/pokemon.c"
POKEMON_CONSTANTS = REPO_ROOT / "include/constants/pokemon.h"
ABILITY_CONSTANTS = REPO_ROOT / "include/constants/abilities.h"
ABILITY_DATA = REPO_ROOT / "src/data/abilities.h"
ITEM_CONSTANTS = REPO_ROOT / "include/constants/items.h"
ITEM_DATA = REPO_ROOT / "src/data/items.h"
HOLD_EFFECT_CONSTANTS = REPO_ROOT / "include/constants/hold_effects.h"
MOVE_CONSTANTS = REPO_ROOT / "include/constants/moves.h"
MOVE_DATA = REPO_ROOT / "src/data/moves_info.h"
TYPE_DATA = REPO_ROOT / "src/data/types_info.h"
CONFIG_GENERAL = REPO_ROOT / "include/config/general.h"
CONFIG_BATTLE = REPO_ROOT / "include/config/battle.h"

PKCALC_STAT_BY_REPO_STAT = OrderedDict(
    [
        ("STAT_ATK", "atk"),
        ("STAT_DEF", "def"),
        ("STAT_SPEED", "spe"),
        ("STAT_SPATK", "spa"),
        ("STAT_SPDEF", "spd"),
    ]
)

ABILITY_TRACE_FIELDS = [
    "description",
    "aiRating",
    "cantBeCopied",
    "cantBeSwapped",
    "cantBeTraced",
    "cantBeSuppressed",
    "cantBeOverwritten",
    "breakable",
    "failsOnImposter",
]

CATEGORY_BY_CONSTANT = OrderedDict(
    [
        ("DAMAGE_CATEGORY_PHYSICAL", "Physical"),
        ("DAMAGE_CATEGORY_SPECIAL", "Special"),
        ("DAMAGE_CATEGORY_STATUS", "Status"),
    ]
)

MOVE_DEFERRED_SEMANTIC_FIELDS = [
    "effect",
    "description",
    "accuracy",
    "pp",
    "target",
    "priority",
    "strikeCount",
    "criticalHitStage",
    "alwaysCriticalHit",
    "flags",
    "argument",
    "additionalEffects",
    "zMove",
    "contestEffect",
    "contestCategory",
    "contestComboStarterId",
    "contestComboMoves",
    "battleAnimScript",
]

MOVE_FLAG_TRACE_FIELDS = [
    "makesContact",
    "ignoresProtect",
    "magicCoatAffected",
    "snatchAffected",
    "ignoresKingsRock",
    "punchingMove",
    "bitingMove",
    "pulseMove",
    "soundMove",
    "ballisticMove",
    "powderMove",
    "danceMove",
    "windMove",
    "slicingMove",
    "healingMove",
    "minimizeDoubleDamage",
    "ignoresTargetAbility",
    "ignoresTargetDefenseEvasionStages",
    "damagesUnderground",
    "damagesUnderwater",
    "damagesAirborne",
    "damagesAirborneDoubleDamage",
    "ignoreTypeIfFlyingAndUngrounded",
    "thawsUser",
    "ignoresSubstitute",
    "forcePressure",
    "cantUseTwice",
    "alwaysHitsInRain",
    "accuracy50InSun",
    "alwaysHitsInHailSnow",
    "gravityBanned",
    "mirrorMoveBanned",
    "meFirstBanned",
    "mimicBanned",
    "metronomeBanned",
    "copycatBanned",
    "assistBanned",
    "sleepTalkBanned",
    "instructBanned",
    "encoreBanned",
    "parentalBondBanned",
    "skyBattleBanned",
    "sketchBanned",
    "dampBanned",
    "validApprenticeMove",
]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        default=DEFAULT_OUTPUT_DIR,
        type=Path,
        help="Directory for generated PKCalc data-migration proof artifacts.",
    )
    args = parser.parse_args()

    output_dir = resolve_from_root(args.output_dir)
    source_natures, natures_by_id, calc_natures = build_nature_artifacts()
    source_abilities, abilities_by_id = build_ability_artifacts()
    source_held_items, held_items_by_id = build_held_item_artifacts()
    source_moves, moves_by_id, calc_moves = build_move_artifacts()
    write_outputs(
        output_dir=output_dir,
        source_natures=source_natures,
        natures_by_id=natures_by_id,
        calc_natures=calc_natures,
        source_abilities=source_abilities,
        abilities_by_id=abilities_by_id,
        source_held_items=source_held_items,
        held_items_by_id=held_items_by_id,
        source_moves=source_moves,
        moves_by_id=moves_by_id,
        calc_moves=calc_moves,
    )

    for name in [
        "source_natures.json",
        "pkcalc_natures_by_id.json",
        "pkcalc_calc_natures.json",
        "pkcalc_natures.js",
        "source_abilities.json",
        "pkcalc_abilities_by_id.json",
        "pkcalc_abilities.js",
        "source_held_items.json",
        "pkcalc_held_items_by_id.json",
        "pkcalc_held_items.js",
        "source_moves.json",
        "pkcalc_moves_by_id.json",
        "pkcalc_calc_moves.json",
        "pkcalc_moves.js",
    ]:
        print(f"Wrote {relpath_or_abs(output_dir / name)}")

    failures = [
        *source_natures["failures"],
        *source_abilities["failures"],
        *source_held_items["failures"],
        *source_moves["failures"],
    ]
    if failures:
        for failure in failures:
            print(f"PKCalc data migration source failure: {failure}", file=sys.stderr)
        sys.exit(1)
    print(
        "Generated PKCalc data-migration proof artifacts: "
        f"{len(source_natures['natures'])} natures, "
        f"{len(source_abilities['abilities'])} abilities, "
        f"{len(source_held_items['heldItems'])} held items, "
        f"{len(source_moves['moves'])} moves"
    )


def resolve_from_root(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def build_nature_artifacts() -> tuple[OrderedDict, OrderedDict, OrderedDict]:
    nature_constants = parse_numeric_defines(POKEMON_CONSTANTS, "NATURE_")
    natures_info = parse_natures_info(POKEMON_C)
    failures: list[str] = []
    source_natures = []

    for constant, index in sorted(nature_constants.items(), key=lambda item: item[1]):
        info = natures_info.get(constant)
        if info is None:
            failures.append(f"missing gNaturesInfo entry for {constant}")
            continue
        missing_fields = [
            field
            for field in ("name", "statUp", "statDown")
            if not isinstance(info.get(field), str) or not info[field]
        ]
        for field in missing_fields:
            failures.append(f"{constant} missing field {field}")
        stat_up = info.get("statUp", "")
        stat_down = info.get("statDown", "")
        pkcalc_plus = PKCALC_STAT_BY_REPO_STAT.get(stat_up, "")
        pkcalc_minus = PKCALC_STAT_BY_REPO_STAT.get(stat_down, "")
        if not pkcalc_plus:
            failures.append(f"{constant} has unmapped statUp {stat_up}")
        if not pkcalc_minus:
            failures.append(f"{constant} has unmapped statDown {stat_down}")

        source_natures.append(
            OrderedDict(
                [
                    ("constant", constant),
                    ("index", index),
                    ("name", info.get("name", "")),
                    ("statUp", stat_up),
                    ("statDown", stat_down),
                    ("pkcalcId", to_id(info.get("name", ""))),
                    ("pkcalcPlus", pkcalc_plus),
                    ("pkcalcMinus", pkcalc_minus),
                    ("neutral", bool(pkcalc_plus and pkcalc_plus == pkcalc_minus)),
                ]
            )
        )

    if len(source_natures) != 25:
        failures.append(f"expected 25 natures, found {len(source_natures)}")
    unexpected = sorted(set(natures_info) - set(nature_constants))
    for constant in unexpected:
        failures.append(f"unexpected gNaturesInfo entry without constant: {constant}")

    source_report = OrderedDict(
        [
            ("schemaVersion", 1),
            ("category", "natures"),
            (
                "sources",
                OrderedDict(
                    [
                        ("pokemonConstants", relpath(POKEMON_CONSTANTS)),
                        ("pokemonData", relpath(POKEMON_C)),
                    ]
                ),
            ),
            (
                "repoSourceFields",
                ["constant", "index", "name", "statUp", "statDown"],
            ),
            (
                "pkcalcFieldMapping",
                OrderedDict(
                    [
                        ("constant", "not exported; source traceability only"),
                        ("index", "not exported; source ordering only"),
                        ("name", "name"),
                        ("statUp", "plus"),
                        ("statDown", "minus"),
                    ]
                ),
            ),
            ("statMapping", PKCALC_STAT_BY_REPO_STAT),
            ("natures", source_natures),
            ("failures", failures),
        ]
    )

    natures_by_id = OrderedDict(
        (
            nature["pkcalcId"],
            OrderedDict(
                [
                    ("kind", "Nature"),
                    ("id", nature["pkcalcId"]),
                    ("name", nature["name"]),
                    ("plus", nature["pkcalcPlus"]),
                    ("minus", nature["pkcalcMinus"]),
                ]
            ),
        )
        for nature in source_natures
        if nature["pkcalcId"] and nature["pkcalcPlus"] and nature["pkcalcMinus"]
    )
    calc_natures = OrderedDict(
        (nature["name"], [nature["pkcalcPlus"], nature["pkcalcMinus"]])
        for nature in source_natures
        if nature["name"] and nature["pkcalcPlus"] and nature["pkcalcMinus"]
    )
    return source_report, natures_by_id, calc_natures


def build_ability_artifacts() -> tuple[OrderedDict, OrderedDict]:
    ability_constants = parse_numeric_defines(ABILITY_CONSTANTS, "ABILITY_")
    abilities_info = parse_abilities_info(ABILITY_DATA)
    failures: list[str] = []
    source_abilities = []
    seen_ids: dict[str, str] = {}

    for constant, index in sorted(ability_constants.items(), key=lambda item: item[1]):
        if constant in {"ABILITY_NONE"} or constant.startswith("ABILITIES_COUNT"):
            continue
        info = abilities_info.get(constant)
        if info is None:
            failures.append(f"missing gAbilitiesInfo entry for {constant}")
            continue
        name = info.get("name", "")
        if not name or name == "-------":
            failures.append(f"{constant} missing user-facing ability name")
            continue
        pkcalc_id = constant_to_pkcalc_id(constant, "ABILITY_")
        if pkcalc_id in seen_ids:
            failures.append(
                f"{constant} generated duplicate PKCalc id {pkcalc_id} already used by {seen_ids[pkcalc_id]}"
            )
        seen_ids[pkcalc_id] = constant
        source_abilities.append(
            OrderedDict(
                [
                    ("constant", constant),
                    ("index", index),
                    ("name", name),
                    ("pkcalcId", pkcalc_id),
                    ("description", info.get("description", "")),
                    ("aiRating", info.get("aiRating")),
                    (
                        "flags",
                        OrderedDict(
                            (field, bool(info.get(field, False)))
                            for field in ABILITY_TRACE_FIELDS
                            if field != "description" and field != "aiRating"
                        ),
                    ),
                ]
            )
        )

    if not source_abilities:
        failures.append("expected at least one ability identity candidate")
    unexpected = sorted(set(abilities_info) - set(ability_constants))
    for constant in unexpected:
        failures.append(f"unexpected gAbilitiesInfo entry without constant: {constant}")

    source_report = OrderedDict(
        [
            ("schemaVersion", 1),
            ("category", "abilities"),
            (
                "sources",
                OrderedDict(
                    [
                        ("abilityConstants", relpath(ABILITY_CONSTANTS)),
                        ("abilityData", relpath(ABILITY_DATA)),
                    ]
                ),
            ),
            (
                "repoSourceFields",
                ["constant", "index", "name", "description", "aiRating", "flags"],
            ),
            (
                "pkcalcFieldMapping",
                OrderedDict(
                    [
                        ("constant", "id, after removing ABILITY_ and punctuation"),
                        ("index", "not exported; source ordering only"),
                        ("name", "name"),
                        ("description", "not exported; source traceability only"),
                        ("aiRating", "not exported; source traceability only"),
                        ("flags", "not exported; behavior semantics are deferred"),
                    ]
                ),
            ),
            ("identityCandidateCount", len(source_abilities)),
            ("abilities", source_abilities),
            ("failures", failures),
        ]
    )
    abilities_by_id = OrderedDict(
        (
            ability["pkcalcId"],
            OrderedDict(
                [
                    ("kind", "Ability"),
                    ("id", ability["pkcalcId"]),
                    ("name", ability["name"]),
                ]
            ),
        )
        for ability in source_abilities
        if ability["pkcalcId"] and ability["name"]
    )
    return source_report, abilities_by_id


def build_held_item_artifacts() -> tuple[OrderedDict, OrderedDict]:
    item_constants = parse_numeric_defines(ITEM_CONSTANTS, "ITEM_")
    item_info = parse_items_info(ITEM_DATA)
    hold_effects = parse_hold_effect_constants(HOLD_EFFECT_CONSTANTS)
    failures: list[str] = []
    source_items = []
    seen_ids: dict[str, str] = {}

    for constant, index in sorted(item_constants.items(), key=lambda item: item[1]):
        if constant in {"ITEM_NONE", "ITEM_LIST_END", "ITEM_FIELD_ARROW"}:
            continue
        info = item_info.get(constant)
        if info is None:
            continue
        hold_effect = info.get("holdEffect", "HOLD_EFFECT_NONE")
        if not hold_effect or hold_effect == "HOLD_EFFECT_NONE":
            continue
        name = info.get("name", "")
        if not name or name == "????????":
            failures.append(f"{constant} has hold effect {hold_effect} but no user-facing item name")
            continue
        if hold_effect not in hold_effects:
            failures.append(f"{constant} has unknown holdEffect {hold_effect}")
        pkcalc_id = constant_to_pkcalc_id(constant, "ITEM_")
        if pkcalc_id in seen_ids:
            failures.append(
                f"{constant} generated duplicate PKCalc id {pkcalc_id} already used by {seen_ids[pkcalc_id]}"
            )
        seen_ids[pkcalc_id] = constant
        source_items.append(
            OrderedDict(
                [
                    ("constant", constant),
                    ("index", index),
                    ("name", name),
                    ("pkcalcId", pkcalc_id),
                    ("holdEffect", hold_effect),
                    ("holdEffectParam", info.get("holdEffectParam")),
                    ("pocket", info.get("pocket", "")),
                    ("sortType", info.get("sortType", "")),
                    ("type", info.get("type", "")),
                ]
            )
        )

    if not source_items:
        failures.append("expected at least one held-item identity candidate")

    source_report = OrderedDict(
        [
            ("schemaVersion", 1),
            ("category", "heldItems"),
            (
                "sources",
                OrderedDict(
                    [
                        ("itemConstants", relpath(ITEM_CONSTANTS)),
                        ("itemData", relpath(ITEM_DATA)),
                        ("holdEffectConstants", relpath(HOLD_EFFECT_CONSTANTS)),
                    ]
                ),
            ),
            (
                "repoSourceFields",
                [
                    "constant",
                    "index",
                    "name",
                    "holdEffect",
                    "holdEffectParam",
                    "pocket",
                    "sortType",
                    "type",
                ],
            ),
            (
                "pkcalcFieldMapping",
                OrderedDict(
                    [
                        ("constant", "id, after removing ITEM_ and punctuation"),
                        ("index", "not exported; source ordering only"),
                        ("name", "name"),
                        ("holdEffect", "not exported; behavior semantics are deferred"),
                        ("holdEffectParam", "not exported; behavior semantics are deferred"),
                        ("pocket", "not exported; source traceability only"),
                        ("sortType", "not exported; source traceability only"),
                        ("type", "not exported; source traceability only"),
                    ]
                ),
            ),
            ("selectionRule", "Items with a non-HOLD_EFFECT_NONE .holdEffect field"),
            ("identityCandidateCount", len(source_items)),
            ("heldItems", source_items),
            ("failures", failures),
        ]
    )
    held_items_by_id = OrderedDict(
        (
            item["pkcalcId"],
            OrderedDict(
                [
                    ("kind", "Item"),
                    ("id", item["pkcalcId"]),
                    ("name", item["name"]),
                ]
            ),
        )
        for item in source_items
        if item["pkcalcId"] and item["name"]
    )
    return source_report, held_items_by_id


def build_move_artifacts() -> tuple[OrderedDict, OrderedDict, OrderedDict]:
    move_constants = parse_numeric_defines(MOVE_CONSTANTS, "MOVE_")
    type_names = parse_type_names(TYPE_DATA)
    moves_info, preprocessor_command = parse_moves_info(MOVE_DATA)
    failures: list[str] = []
    source_moves = []
    incomplete_moves = []
    seen_ids: dict[str, str] = {}

    for constant, index in sorted(move_constants.items(), key=lambda item: item[1]):
        if constant in {"MOVE_NONE", "MOVE_UNAVAILABLE"}:
            continue
        info = moves_info.get(index)
        if info is None:
            failures.append(f"missing gMovesInfo entry for {constant} index {index}")
            continue

        name = info.get("name", "")
        pkcalc_id = constant_to_pkcalc_id(constant, "MOVE_")
        type_constant = info.get("type", "")
        type_name = type_names.get(type_constant, "")
        category_constant = info.get("category", "")
        category_name = CATEGORY_BY_CONSTANT.get(category_constant, "")
        power_source = info.get("power")
        base_power = resolve_int_value(power_source)
        unresolved_fields = []
        if not name or name == "-":
            unresolved_fields.append("name")
        if not type_name:
            unresolved_fields.append("type")
        if not category_name:
            unresolved_fields.append("category")
        if base_power is None:
            unresolved_fields.append("basePower")

        if pkcalc_id in seen_ids:
            failures.append(
                f"{constant} generated duplicate PKCalc id {pkcalc_id} already used by {seen_ids[pkcalc_id]}"
            )
        seen_ids[pkcalc_id] = constant

        move = OrderedDict(
            [
                ("constant", constant),
                ("index", index),
                ("name", name),
                ("pkcalcId", pkcalc_id),
                ("typeConstant", type_constant),
                ("type", type_name),
                ("categoryConstant", category_constant),
                ("category", category_name),
                ("powerSource", power_source),
                ("basePower", base_power),
                ("effect", info.get("effect", "")),
                ("accuracySource", info.get("accuracy")),
                ("ppSource", info.get("pp")),
                ("target", info.get("target", "")),
                ("prioritySource", info.get("priority")),
                ("flagsPresent", info.get("flagsPresent", [])),
                ("additionalEffectsPresent", bool(info.get("additionalEffectsPresent", False))),
                ("metadataComplete", not unresolved_fields),
                ("unresolvedMetadataFields", unresolved_fields),
            ]
        )
        source_moves.append(move)
        if unresolved_fields:
            incomplete_moves.append(
                OrderedDict(
                    [
                        ("constant", constant),
                        ("pkcalcId", pkcalc_id),
                        ("unresolvedMetadataFields", unresolved_fields),
                    ]
                )
            )

    if not source_moves:
        failures.append("expected at least one move metadata candidate")
    if incomplete_moves:
        failures.append(f"{len(incomplete_moves)} move metadata candidates have unresolved basic fields")

    source_report = OrderedDict(
        [
            ("schemaVersion", 1),
            ("category", "moves"),
            (
                "sources",
                OrderedDict(
                    [
                        ("moveConstants", relpath(MOVE_CONSTANTS)),
                        ("moveData", relpath(MOVE_DATA)),
                        ("typeData", relpath(TYPE_DATA)),
                        ("configGeneral", relpath(CONFIG_GENERAL)),
                        ("configBattle", relpath(CONFIG_BATTLE)),
                    ]
                ),
            ),
            ("preprocessorCommand", preprocessor_command),
            (
                "repoSourceFields",
                [
                    "constant",
                    "index",
                    "name",
                    "type",
                    "category",
                    "power",
                    "effect",
                    "accuracy",
                    "pp",
                    "target",
                    "priority",
                    "flags",
                    "additionalEffects",
                ],
            ),
            (
                "pkcalcFieldMapping",
                OrderedDict(
                    [
                        ("constant", "id, after removing MOVE_ and punctuation"),
                        ("index", "not exported; source ordering only"),
                        ("name", "name"),
                        ("type", "type"),
                        ("category", "category"),
                        ("power", "basePower for MOVES_BY_ID and bp for calc.MOVES[4]"),
                        ("effect", "not exported; battle semantics are deferred"),
                        ("accuracy", "not exported; source traceability only"),
                        ("pp", "not exported; source traceability only"),
                        ("target", "not exported; battle semantics are deferred"),
                        ("priority", "not exported; battle semantics are deferred"),
                        ("flags", "not exported; battle semantics are deferred"),
                        ("additionalEffects", "not exported; battle semantics are deferred"),
                    ]
                ),
            ),
            ("selectionRule", "Direct numeric MOVE_* constants except MOVE_NONE and MOVE_UNAVAILABLE"),
            ("excludedConstants", ["MOVE_NONE", "MOVE_UNAVAILABLE"]),
            ("typeMapping", type_names),
            ("categoryMapping", CATEGORY_BY_CONSTANT),
            ("deferredSemanticFields", MOVE_DEFERRED_SEMANTIC_FIELDS),
            ("metadataCandidateCount", len(source_moves)),
            ("metadataCompleteCount", len(source_moves) - len(incomplete_moves)),
            ("metadataIncomplete", incomplete_moves),
            ("moves", source_moves),
            ("failures", failures),
        ]
    )
    complete_moves = [move for move in source_moves if move["metadataComplete"]]
    moves_by_id = OrderedDict(
        (
            move["pkcalcId"],
            OrderedDict(
                [
                    ("kind", "Move"),
                    ("id", move["pkcalcId"]),
                    ("name", move["name"]),
                    ("type", move["type"]),
                    ("category", move["category"]),
                    ("basePower", move["basePower"]),
                ]
            ),
        )
        for move in complete_moves
    )
    calc_moves = OrderedDict(
        (
            move["name"],
            OrderedDict(
                [
                    ("bp", move["basePower"]),
                    ("type", move["type"]),
                    ("category", move["category"]),
                ]
            ),
        )
        for move in complete_moves
    )
    return source_report, moves_by_id, calc_moves


def parse_numeric_defines(path: Path, prefix: str) -> OrderedDict:
    constants = OrderedDict()
    pattern = re.compile(rf"^#define\s+({prefix}[A-Z0-9_]+)\s+(\d+)\b", re.MULTILINE)
    for match in pattern.finditer(path.read_text()):
        constants[match.group(1)] = int(match.group(2))
    return constants


def parse_hold_effect_constants(path: Path) -> set[str]:
    text = strip_c_comments(path.read_text())
    match = re.search(r"enum\s+ItemHoldEffect\s*\{(?P<body>.*?)\n\};", text, flags=re.DOTALL)
    if not match:
        raise ValueError(f"Could not find enum ItemHoldEffect in {relpath(path)}")
    return set(re.findall(r"\b(HOLD_EFFECT_[A-Z0-9_]+)\b", match.group("body")))


def parse_natures_info(path: Path) -> OrderedDict:
    text = strip_c_comments(path.read_text())
    match = re.search(
        r"const\s+struct\s+NatureInfo\s+gNaturesInfo\[NUM_NATURES\]\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise ValueError(f"Could not find gNaturesInfo in {relpath(path)}")

    entries = OrderedDict()
    body = match.group("body")
    for constant, entry_body in parse_designated_entries(body, "NATURE_"):
        entries[constant] = OrderedDict(
            [
                ("name", parse_string_field(entry_body, "name")),
                ("statUp", parse_symbol_field(entry_body, "statUp")),
                ("statDown", parse_symbol_field(entry_body, "statDown")),
            ]
        )
    return entries


def parse_abilities_info(path: Path) -> OrderedDict:
    text = strip_c_comments(path.read_text())
    match = re.search(
        r"const\s+struct\s+Ability\s+gAbilitiesInfo\[ABILITIES_COUNT\]\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise ValueError(f"Could not find gAbilitiesInfo in {relpath(path)}")
    entries = OrderedDict()
    for constant, entry_body in parse_designated_entries(match.group("body"), "ABILITY_"):
        entries[constant] = OrderedDict(
            [
                ("name", parse_string_field(entry_body, "name")),
                ("description", parse_string_field(entry_body, "description")),
                ("aiRating", parse_int_field(entry_body, "aiRating")),
                *(
                    (field, parse_bool_field(entry_body, field))
                    for field in ABILITY_TRACE_FIELDS
                    if field != "description" and field != "aiRating"
                ),
            ]
        )
    return entries


def parse_items_info(path: Path) -> OrderedDict:
    text = strip_c_comments(path.read_text())
    match = re.search(
        r"const\s+struct\s+Item\s+gItemsInfo\[\]\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise ValueError(f"Could not find gItemsInfo in {relpath(path)}")
    entries = OrderedDict()
    for constant, entry_body in parse_designated_entries(match.group("body"), "ITEM_"):
        entries[constant] = OrderedDict(
            [
                ("name", parse_string_field(entry_body, "name")),
                ("holdEffect", parse_symbol_field(entry_body, "holdEffect") or "HOLD_EFFECT_NONE"),
                ("holdEffectParam", parse_value_field(entry_body, "holdEffectParam")),
                ("pocket", parse_symbol_field(entry_body, "pocket")),
                ("sortType", parse_symbol_field(entry_body, "sortType")),
                ("type", parse_symbol_field(entry_body, "type")),
            ]
        )
    return entries


def parse_type_names(path: Path) -> OrderedDict:
    text = strip_c_comments(path.read_text())
    match = re.search(
        r"const\s+struct\s+TypeInfo\s+gTypesInfo\[NUMBER_OF_MON_TYPES\]\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise ValueError(f"Could not find gTypesInfo in {relpath(path)}")
    type_names = OrderedDict()
    for constant, entry_body in parse_designated_entries(match.group("body"), "TYPE_"):
        name = parse_string_field(entry_body, "name")
        if name:
            type_names[constant] = name
    return type_names


def parse_moves_info(path: Path) -> tuple[OrderedDict, list[str]]:
    text, command = preprocess_move_data(path)
    match = re.search(
        r"const\s+struct\s+MoveInfo\s+gMovesInfo\[[^\]]+\]\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise ValueError(f"Could not find gMovesInfo in preprocessed {relpath(path)}")
    entries = OrderedDict()
    for index, entry_body in parse_numeric_designated_entries(match.group("body")):
        entries[index] = OrderedDict(
            [
                ("name", parse_string_field(entry_body, "name")),
                ("effect", parse_symbol_field(entry_body, "effect")),
                ("power", parse_value_field(entry_body, "power")),
                ("type", parse_symbol_value_field(entry_body, "type")),
                ("accuracy", parse_value_field(entry_body, "accuracy")),
                ("pp", parse_value_field(entry_body, "pp")),
                ("target", parse_value_field(entry_body, "target")),
                ("priority", parse_value_field(entry_body, "priority")),
                ("category", parse_symbol_value_field(entry_body, "category")),
                (
                    "flagsPresent",
                    [
                        field
                        for field in MOVE_FLAG_TRACE_FIELDS
                        if re.search(rf"\.{field}\s*=", entry_body)
                    ],
                ),
                (
                    "additionalEffectsPresent",
                    bool(re.search(r"\.additionalEffects\s*=|ADDITIONAL_EFFECTS\s*\(", entry_body)),
                ),
            ]
        )
    return entries, command


def preprocess_move_data(path: Path) -> tuple[str, list[str]]:
    command = [
        "gcc",
        "-E",
        "-P",
        "-Iinclude",
        "-Isrc",
        "-include",
        relpath(CONFIG_GENERAL),
        "-include",
        relpath(CONFIG_BATTLE),
        relpath(path),
    ]
    try:
        completed = subprocess.run(
            command,
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        raise RuntimeError(f"Could not preprocess {relpath(path)} with gcc: {error}") from error
    if completed.returncode != 0:
        raise RuntimeError(
            f"Could not preprocess {relpath(path)} with gcc:\n{completed.stderr.strip()}"
        )
    return completed.stdout, command


def parse_designated_entries(body: str, prefix: str) -> list[tuple[str, str]]:
    entry_pattern = re.compile(
        rf"\[({prefix}[A-Z0-9_]+)\]\s*=\s*\{{(?P<body>.*?)(?=\n\s*\[{prefix}[A-Z0-9_]+\]\s*=|\Z)",
        flags=re.DOTALL,
    )
    return [(match.group(1), match.group("body")) for match in entry_pattern.finditer(body)]


def parse_numeric_designated_entries(body: str) -> list[tuple[int, str]]:
    entry_pattern = re.compile(
        r"\[(\d+)\]\s*=\s*\{(?P<body>.*?)(?=\n\s*\[\d+\]\s*=|\Z)",
        flags=re.DOTALL,
    )
    return [(int(match.group(1)), match.group("body")) for match in entry_pattern.finditer(body)]


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*", "", text)


def parse_string_field(text: str, field: str) -> str:
    match = re.search(
        rf"\.{field}\s*=\s*(?:ITEM_NAME|ITEM_PLURAL_NAME|COMPOUND_STRING(?:_SIZE_LIMIT)?|_)\(\s*\"((?:[^\"\\]|\\.)*)\"",
        text,
        flags=re.DOTALL,
    )
    return unescape_c_string(match.group(1)) if match else ""


def parse_symbol_field(text: str, field: str) -> str:
    match = re.search(rf"\.{field}\s*=\s*([A-Z0-9_]+)", text)
    return match.group(1) if match else ""


def parse_int_field(text: str, field: str) -> int | None:
    match = re.search(rf"\.{field}\s*=\s*(-?\d+)", text)
    return int(match.group(1)) if match else None


def parse_bool_field(text: str, field: str) -> bool:
    match = re.search(rf"\.{field}\s*=\s*(TRUE|FALSE)", text)
    return match is not None and match.group(1) == "TRUE"


def parse_value_field(text: str, field: str) -> str | int | None:
    match = re.search(rf"\.{field}\s*=\s*([^,\n]+)", text)
    if not match:
        return None
    value = match.group(1).strip()
    return int(value) if re.fullmatch(r"-?\d+", value) else value


def parse_symbol_value_field(text: str, field: str) -> str:
    value = parse_value_field(text, field)
    return resolve_symbol_value(value) or ""


def resolve_int_value(value: object) -> int | None:
    if isinstance(value, int):
        return value
    if not isinstance(value, str):
        return None
    text = value.strip()
    if re.fullmatch(r"-?\d+", text):
        return int(text)
    ternary = re.fullmatch(
        r"\(?\s*(-?\d+)\s*(<=|>=|==|!=|<|>)\s*(-?\d+)\s*\)?\s*\?\s*(-?\d+)\s*:\s*(-?\d+)",
        text,
    )
    if not ternary:
        return None
    left = int(ternary.group(1))
    operator = ternary.group(2)
    right = int(ternary.group(3))
    true_value = int(ternary.group(4))
    false_value = int(ternary.group(5))
    result = {
        "<": left < right,
        "<=": left <= right,
        ">": left > right,
        ">=": left >= right,
        "==": left == right,
        "!=": left != right,
    }[operator]
    return true_value if result else false_value


def resolve_symbol_value(value: object) -> str | None:
    if isinstance(value, int):
        return None
    if not isinstance(value, str):
        return None
    text = value.strip()
    if re.fullmatch(r"[A-Z][A-Z0-9_]+", text):
        return text
    ternary = re.fullmatch(
        r"\(?\s*(-?\d+)\s*(<=|>=|==|!=|<|>)\s*(-?\d+)\s*\)?\s*\?\s*([A-Z][A-Z0-9_]+)\s*:\s*([A-Z][A-Z0-9_]+)",
        text,
    )
    if not ternary:
        return None
    left = int(ternary.group(1))
    operator = ternary.group(2)
    right = int(ternary.group(3))
    true_value = ternary.group(4)
    false_value = ternary.group(5)
    result = {
        "<": left < right,
        "<=": left <= right,
        ">": left > right,
        ">=": left >= right,
        "==": left == right,
        "!=": left != right,
    }[operator]
    return true_value if result else false_value


def write_outputs(
    output_dir: Path,
    source_natures: OrderedDict,
    natures_by_id: OrderedDict,
    calc_natures: OrderedDict,
    source_abilities: OrderedDict,
    abilities_by_id: OrderedDict,
    source_held_items: OrderedDict,
    held_items_by_id: OrderedDict,
    source_moves: OrderedDict,
    moves_by_id: OrderedDict,
    calc_moves: OrderedDict,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    write_json(output_dir / "source_natures.json", source_natures)
    write_json(output_dir / "pkcalc_natures_by_id.json", natures_by_id)
    write_json(output_dir / "pkcalc_calc_natures.json", calc_natures)
    write_js(
        output_dir / "pkcalc_natures.js",
        [
            ("NATURES_BY_ID_PK", natures_by_id),
            ("NATURES_PK", calc_natures),
        ],
    )
    write_json(output_dir / "source_abilities.json", source_abilities)
    write_json(output_dir / "pkcalc_abilities_by_id.json", abilities_by_id)
    write_js(output_dir / "pkcalc_abilities.js", [("ABILITIES_BY_ID_PK", abilities_by_id)])
    write_json(output_dir / "source_held_items.json", source_held_items)
    write_json(output_dir / "pkcalc_held_items_by_id.json", held_items_by_id)
    write_js(output_dir / "pkcalc_held_items.js", [("ITEMS_BY_ID_PK", held_items_by_id)])
    write_json(output_dir / "source_moves.json", source_moves)
    write_json(output_dir / "pkcalc_moves_by_id.json", moves_by_id)
    write_json(output_dir / "pkcalc_calc_moves.json", calc_moves)
    write_js(
        output_dir / "pkcalc_moves.js",
        [
            ("MOVES_BY_ID_PK", moves_by_id),
            ("MOVES_PK", calc_moves),
        ],
    )


def write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


def write_js(path: Path, constants: list[tuple[str, OrderedDict]]) -> None:
    chunks = []
    for name, value in constants:
        chunks.append(f"const {name} = {json.dumps(value, indent=4)};")
    path.write_text("\n\n".join(chunks) + "\n")


def to_id(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def constant_to_pkcalc_id(value: str, prefix: str) -> str:
    if value.startswith(prefix):
        value = value[len(prefix) :]
    return to_id(value)


def unescape_c_string(value: str) -> str:
    return (
        value.replace(r"\\", "\\")
        .replace(r"\"", '"')
        .replace(r"\n", "\n")
        .replace(r"\t", "\t")
    )


def relpath(path: Path) -> str:
    return str(path.relative_to(REPO_ROOT))


def relpath_or_abs(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


if __name__ == "__main__":
    main()
