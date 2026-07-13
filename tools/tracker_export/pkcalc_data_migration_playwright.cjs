#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const REPO_ROOT = path.resolve(__dirname, "../..");
const DEFAULT_INPUT_DIR = path.join(
  REPO_ROOT,
  "build/tracker_export/data_migration"
);
const DEFAULT_PKCALC_URL =
  process.env.PKCALC_URL || "https://pkcalc.anastarawneh.com/";
const REQUIRED_NATURE_FIELDS = ["kind", "id", "name", "plus", "minus"];
const REQUIRED_ABILITY_FIELDS = ["kind", "id", "name"];
const REQUIRED_ITEM_IDENTITY_FIELDS = ["kind", "id", "name"];
const REQUIRED_MOVE_METADATA_FIELDS = ["kind", "id", "name", "type", "category", "basePower"];
const REQUIRED_CALC_MOVE_FIELDS = ["bp", "type", "category"];
const CALC_GENERATION_INDEX = 4;

function parseArgs(argv) {
  let inputDir = DEFAULT_INPUT_DIR;
  let mappingReport = "";
  let validationReport = "";
  let abilityGapReport = "";
  let heldItemGapReport = "";
  let moveGapReport = "";
  let url = DEFAULT_PKCALC_URL;

  for (let i = 2; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--input-dir") {
      inputDir = requiredArg(argv, ++i, arg);
    } else if (arg === "--mapping-report") {
      mappingReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--validation-report") {
      validationReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--ability-gap-report") {
      abilityGapReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--held-item-gap-report") {
      heldItemGapReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--move-gap-report") {
      moveGapReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--url") {
      url = requiredArg(argv, ++i, arg);
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: pkcalc_data_migration_playwright.cjs [--input-dir DIR] [--mapping-report FILE] [--validation-report FILE] [--ability-gap-report FILE] [--held-item-gap-report FILE] [--move-gap-report FILE] [--url URL]"
      );
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }

  inputDir = resolveFromRoot(inputDir);
  mappingReport = mappingReport
    ? resolveFromRoot(mappingReport)
    : path.join(inputDir, "pkcalc_catalog_mapping_report.json");
  validationReport = validationReport
    ? resolveFromRoot(validationReport)
    : path.join(inputDir, "natures_migration_validation_report.json");
  abilityGapReport = abilityGapReport
    ? resolveFromRoot(abilityGapReport)
    : path.join(inputDir, "ability_identity_gap_report.json");
  heldItemGapReport = heldItemGapReport
    ? resolveFromRoot(heldItemGapReport)
    : path.join(inputDir, "held_item_identity_gap_report.json");
  moveGapReport = moveGapReport
    ? resolveFromRoot(moveGapReport)
    : path.join(inputDir, "move_metadata_gap_report.json");

  return {
    inputDir,
    mappingReport,
    validationReport,
    abilityGapReport,
    heldItemGapReport,
    moveGapReport,
    url,
  };
}

function requiredArg(argv, index, flag) {
  if (index >= argv.length) {
    throw new Error(`${flag} requires a value`);
  }
  return argv[index];
}

function resolveFromRoot(value) {
  if (path.isAbsolute(value)) {
    return value;
  }
  return path.join(REPO_ROOT, value);
}

function loadJson(filePath) {
  if (!fs.existsSync(filePath)) {
    throw new Error(`Missing generated data-migration artifact: ${filePath}`);
  }
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function writeJson(filePath, payload) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(payload, null, 2)}\n`);
}

function normalize(value) {
  return String(value || "")
    .toLowerCase()
    .replace(/[^a-z0-9]/g, "");
}

function relpathOrAbs(filePath) {
  const rel = path.relative(REPO_ROOT, filePath);
  return rel.startsWith("..") || path.isAbsolute(rel) ? filePath : rel;
}

function loadGeneratedArtifacts(inputDir) {
  const sourceNaturesPath = path.join(inputDir, "source_natures.json");
  const naturesByIdPath = path.join(inputDir, "pkcalc_natures_by_id.json");
  const calcNaturesPath = path.join(inputDir, "pkcalc_calc_natures.json");
  const naturesJsPath = path.join(inputDir, "pkcalc_natures.js");
  const sourceAbilitiesPath = path.join(inputDir, "source_abilities.json");
  const abilitiesByIdPath = path.join(inputDir, "pkcalc_abilities_by_id.json");
  const abilitiesJsPath = path.join(inputDir, "pkcalc_abilities.js");
  const sourceHeldItemsPath = path.join(inputDir, "source_held_items.json");
  const heldItemsByIdPath = path.join(inputDir, "pkcalc_held_items_by_id.json");
  const heldItemsJsPath = path.join(inputDir, "pkcalc_held_items.js");
  const sourceMovesPath = path.join(inputDir, "source_moves.json");
  const movesByIdPath = path.join(inputDir, "pkcalc_moves_by_id.json");
  const calcMovesPath = path.join(inputDir, "pkcalc_calc_moves.json");
  const movesJsPath = path.join(inputDir, "pkcalc_moves.js");
  return {
    sourceNaturesPath,
    naturesByIdPath,
    calcNaturesPath,
    naturesJsPath,
    sourceAbilitiesPath,
    abilitiesByIdPath,
    abilitiesJsPath,
    sourceHeldItemsPath,
    heldItemsByIdPath,
    heldItemsJsPath,
    sourceMovesPath,
    movesByIdPath,
    calcMovesPath,
    movesJsPath,
    sourceNatures: loadJson(sourceNaturesPath),
    naturesById: loadJson(naturesByIdPath),
    calcNatures: loadJson(calcNaturesPath),
    sourceAbilities: loadJson(sourceAbilitiesPath),
    abilitiesById: loadJson(abilitiesByIdPath),
    sourceHeldItems: loadJson(sourceHeldItemsPath),
    heldItemsById: loadJson(heldItemsByIdPath),
    sourceMoves: loadJson(sourceMovesPath),
    movesById: loadJson(movesByIdPath),
    calcMoves: loadJson(calcMovesPath),
  };
}

function validateGeneratedAgainstSource(artifacts) {
  const failures = [];
  if (artifacts.sourceNatures.schemaVersion !== 1) {
    failures.push("source_natures.json schemaVersion must be 1");
  }
  if (artifacts.sourceNatures.category !== "natures") {
    failures.push("source_natures.json category must be natures");
  }
  if (artifacts.sourceNatures.failures?.length) {
    failures.push(...artifacts.sourceNatures.failures);
  }
  const natures = artifacts.sourceNatures.natures || [];
  if (natures.length !== 25) {
    failures.push(`expected 25 source natures, found ${natures.length}`);
  }

  for (const nature of natures) {
    const byId = artifacts.naturesById[nature.pkcalcId];
    const calc = artifacts.calcNatures[nature.name];
    if (!byId) {
      failures.push(`missing generated NATURES_BY_ID entry for ${nature.name}`);
      continue;
    }
    for (const [field, expected] of Object.entries({
      kind: "Nature",
      id: nature.pkcalcId,
      name: nature.name,
      plus: nature.pkcalcPlus,
      minus: nature.pkcalcMinus,
    })) {
      if (byId[field] !== expected) {
        failures.push(
          `generated NATURES_BY_ID.${nature.pkcalcId}.${field} expected ${expected}, saw ${byId[field]}`
        );
      }
    }
    if (!Array.isArray(calc) || calc[0] !== nature.pkcalcPlus || calc[1] !== nature.pkcalcMinus) {
      failures.push(
        `generated calc.NATURES.${nature.name} expected [${nature.pkcalcPlus}, ${nature.pkcalcMinus}]`
      );
    }
  }

  return failures;
}

function validateGeneratedIdentityAgainstSource({
  sourceReport,
  sourceKey,
  generatedById,
  expectedCategory,
  expectedKind,
}) {
  const failures = [];
  if (sourceReport.schemaVersion !== 1) {
    failures.push(`${expectedCategory} source report schemaVersion must be 1`);
  }
  if (sourceReport.category !== expectedCategory) {
    failures.push(`${expectedCategory} source report category must be ${expectedCategory}`);
  }
  if (sourceReport.failures?.length) {
    failures.push(...sourceReport.failures);
  }

  const entries = sourceReport[sourceKey] || [];
  if (!entries.length) {
    failures.push(`${expectedCategory} source report must contain identity candidates`);
  }
  if (Object.keys(generatedById || {}).length !== entries.length) {
    failures.push(
      `${expectedCategory} generated by-id count expected ${entries.length}, saw ${Object.keys(
        generatedById || {}
      ).length}`
    );
  }

  const seenIds = new Map();
  for (const entry of entries) {
    if (!entry.pkcalcId || !entry.name) {
      failures.push(`${expectedCategory} source entry ${entry.constant || "<unknown>"} missing id or name`);
      continue;
    }
    if (seenIds.has(entry.pkcalcId)) {
      failures.push(
        `${expectedCategory} duplicate id ${entry.pkcalcId} for ${entry.constant} and ${seenIds.get(
          entry.pkcalcId
        )}`
      );
    }
    seenIds.set(entry.pkcalcId, entry.constant);
    const generatedEntry = generatedById?.[entry.pkcalcId];
    if (!generatedEntry) {
      failures.push(`${expectedCategory} missing generated by-id entry ${entry.pkcalcId}`);
      continue;
    }
    for (const [field, expected] of Object.entries({
      kind: expectedKind,
      id: entry.pkcalcId,
      name: entry.name,
    })) {
      if (generatedEntry[field] !== expected) {
        failures.push(
          `${expectedCategory} generated ${entry.pkcalcId}.${field} expected ${expected}, saw ${generatedEntry[field]}`
        );
      }
    }
  }

  return failures;
}

function validateGeneratedMoveMetadataAgainstSource({ sourceReport, generatedById, generatedCalc }) {
  const failures = [];
  if (sourceReport.schemaVersion !== 1) {
    failures.push("moves source report schemaVersion must be 1");
  }
  if (sourceReport.category !== "moves") {
    failures.push("moves source report category must be moves");
  }
  if (sourceReport.failures?.length) {
    failures.push(...sourceReport.failures);
  }

  const entries = sourceReport.moves || [];
  const completeEntries = entries.filter((entry) => entry.metadataComplete);
  if (!entries.length) {
    failures.push("moves source report must contain metadata candidates");
  }
  if (sourceReport.metadataCandidateCount !== entries.length) {
    failures.push(
      `moves metadataCandidateCount expected ${entries.length}, saw ${sourceReport.metadataCandidateCount}`
    );
  }
  if (sourceReport.metadataCompleteCount !== completeEntries.length) {
    failures.push(
      `moves metadataCompleteCount expected ${completeEntries.length}, saw ${sourceReport.metadataCompleteCount}`
    );
  }
  if (completeEntries.length !== entries.length) {
    failures.push(`${entries.length - completeEntries.length} move metadata candidates are incomplete`);
  }
  if (Object.keys(generatedById || {}).length !== completeEntries.length) {
    failures.push(
      `moves generated by-id count expected ${completeEntries.length}, saw ${Object.keys(
        generatedById || {}
      ).length}`
    );
  }
  if (Object.keys(generatedCalc || {}).length !== completeEntries.length) {
    failures.push(
      `moves generated calc count expected ${completeEntries.length}, saw ${Object.keys(
        generatedCalc || {}
      ).length}`
    );
  }

  const seenIds = new Map();
  for (const entry of completeEntries) {
    if (!entry.pkcalcId || !entry.name || !entry.type || !entry.category || typeof entry.basePower !== "number") {
      failures.push(`${entry.constant || "<unknown move>"} missing basic move metadata`);
      continue;
    }
    if (seenIds.has(entry.pkcalcId)) {
      failures.push(
        `moves duplicate id ${entry.pkcalcId} for ${entry.constant} and ${seenIds.get(entry.pkcalcId)}`
      );
    }
    seenIds.set(entry.pkcalcId, entry.constant);

    const byId = generatedById?.[entry.pkcalcId];
    if (!byId) {
      failures.push(`moves missing generated by-id entry ${entry.pkcalcId}`);
    } else {
      const expectedById = {
        kind: "Move",
        id: entry.pkcalcId,
        name: entry.name,
        type: entry.type,
        category: entry.category,
        basePower: entry.basePower,
      };
      for (const field of REQUIRED_MOVE_METADATA_FIELDS) {
        if (byId[field] !== expectedById[field]) {
          failures.push(
            `moves generated ${entry.pkcalcId}.${field} expected ${expectedById[field]}, saw ${byId[field]}`
          );
        }
      }
    }

    const calc = generatedCalc?.[entry.name];
    if (!calc) {
      failures.push(`moves missing generated calc entry ${entry.name}`);
    } else {
      const expectedCalc = {
        bp: entry.basePower,
        type: entry.type,
        category: entry.category,
      };
      for (const field of REQUIRED_CALC_MOVE_FIELDS) {
        if (calc[field] !== expectedCalc[field]) {
          failures.push(
            `moves generated calc ${entry.name}.${field} expected ${expectedCalc[field]}, saw ${calc[field]}`
          );
        }
      }
    }
  }

  return failures;
}

function objectFieldShape(value) {
  if (!value || typeof value !== "object") {
    return [];
  }
  const sample = Array.isArray(value) ? value.find((item) => item && typeof item === "object") : value;
  if (!sample || typeof sample !== "object") {
    return [];
  }
  return Object.keys(sample).sort();
}

function compareValues(pathName, generated, live, failures, incompatibleValues) {
  if (JSON.stringify(generated) !== JSON.stringify(live)) {
    failures.push(`${pathName} does not match live PKCalc`);
    incompatibleValues.push({
      path: pathName,
      generated,
      live,
    });
  }
}

function compareById(generated, live) {
  const failures = [];
  const missingFields = [];
  const incompatibleValues = [];
  const generatedKeys = Object.keys(generated).sort();
  const liveKeys = Object.keys(live || {}).sort();
  compareValues("NATURES_BY_ID keys", generatedKeys, liveKeys, failures, incompatibleValues);

  for (const id of generatedKeys) {
    const generatedEntry = generated[id];
    const liveEntry = live?.[id];
    if (!liveEntry) {
      failures.push(`live NATURES_BY_ID missing ${id}`);
      incompatibleValues.push({ path: `NATURES_BY_ID.${id}`, generated: generatedEntry, live: null });
      continue;
    }
    for (const field of REQUIRED_NATURE_FIELDS) {
      if (!(field in liveEntry)) {
        missingFields.push(`NATURES_BY_ID.${id}.${field}`);
      }
      compareValues(
        `NATURES_BY_ID.${id}.${field}`,
        generatedEntry[field],
        liveEntry[field],
        failures,
        incompatibleValues
      );
    }
  }

  return { failures, missingFields, incompatibleValues };
}

function compareCalcNatures(generated, live) {
  const failures = [];
  const incompatibleValues = [];
  const generatedKeys = Object.keys(generated).sort();
  const liveKeys = Object.keys(live || {}).sort();
  compareValues("calc.NATURES keys", generatedKeys, liveKeys, failures, incompatibleValues);
  for (const name of generatedKeys) {
    compareValues(
      `calc.NATURES.${name}`,
      generated[name],
      live?.[name],
      failures,
      incompatibleValues
    );
  }
  return { failures, incompatibleValues };
}

function nameSets(values) {
  const exact = new Set();
  const normalized = new Set();
  for (const value of values || []) {
    if (typeof value !== "string" || !value) {
      continue;
    }
    exact.add(value);
    normalized.add(normalize(value));
  }
  return { exact, normalized };
}

function objectCatalogNameSets(catalog) {
  return nameSets(Object.keys(catalog || {}));
}

function buildNormalizedObjectIndex(catalog) {
  const index = new Map();
  for (const [name, entry] of Object.entries(catalog || {})) {
    const normalizedName = normalize(name);
    if (!index.has(normalizedName)) {
      index.set(normalizedName, { name, entry });
    }
  }
  return index;
}

function compareIdentityCatalog({
  category,
  generatedById,
  liveById,
  liveCalcNames,
  requiredFields,
  expectedKind,
}) {
  const failures = [];
  const missingFields = [];
  const incompatibleValues = [];
  const generatedEntries = Object.entries(generatedById || {}).sort(([left], [right]) =>
    left.localeCompare(right)
  );
  const liveEntries = Object.entries(liveById || {}).sort(([left], [right]) =>
    left.localeCompare(right)
  );
  const generatedIds = new Set(generatedEntries.map(([id]) => id));
  const liveIds = new Set(liveEntries.map(([id]) => id));
  const calcNames = nameSets(liveCalcNames || []);

  const missingFromLiveById = [];
  const missingFromCalc = [];
  const liveOnlyById = [];
  const liveCalcOnly = [];
  const sharedIds = [];
  const displayNameDifferences = [];

  for (const [id, generatedEntry] of generatedEntries) {
    for (const field of requiredFields) {
      if (!(field in generatedEntry)) {
        failures.push(`generated ${category}.${id}.${field} is missing`);
        missingFields.push(`generated.${id}.${field}`);
      }
    }

    const liveEntry = liveById?.[id];
    if (!liveEntry) {
      missingFromLiveById.push(generatedEntry);
    } else {
      sharedIds.push(id);
      for (const field of requiredFields) {
        if (!(field in liveEntry)) {
          failures.push(`live ${category}.${id}.${field} is missing`);
          missingFields.push(`live.${id}.${field}`);
        }
      }
      for (const [field, expected] of Object.entries({
        kind: expectedKind,
        id,
      })) {
        if (liveEntry[field] !== expected) {
          failures.push(`live ${category}.${id}.${field} expected ${expected}, saw ${liveEntry[field]}`);
          incompatibleValues.push({
            path: `${category}.${id}.${field}`,
            generated: expected,
            live: liveEntry[field],
          });
        }
      }
      if (normalize(liveEntry.name) !== normalize(generatedEntry.name)) {
        failures.push(
          `live ${category}.${id}.name normalized mismatch expected ${generatedEntry.name}, saw ${liveEntry.name}`
        );
        incompatibleValues.push({
          path: `${category}.${id}.name`,
          generated: generatedEntry.name,
          live: liveEntry.name,
        });
      } else if (liveEntry.name !== generatedEntry.name) {
        displayNameDifferences.push({
          id,
          generated: generatedEntry.name,
          live: liveEntry.name,
        });
      }
    }

    if (
      !calcNames.exact.has(generatedEntry.name) &&
      !calcNames.normalized.has(normalize(generatedEntry.name)) &&
      !calcNames.normalized.has(normalize(id))
    ) {
      missingFromCalc.push(generatedEntry);
    }
  }

  for (const [id, liveEntry] of liveEntries) {
    if (!generatedIds.has(id)) {
      liveOnlyById.push(liveEntry);
    }
  }
  const generatedCalcNames = nameSets(generatedEntries.map(([, entry]) => entry.name));
  for (const name of liveCalcNames || []) {
    if (
      typeof name === "string" &&
      !generatedCalcNames.exact.has(name) &&
      !generatedCalcNames.normalized.has(normalize(name))
    ) {
      liveCalcOnly.push(name);
    }
  }

  const gapCount =
    missingFromLiveById.length +
    missingFromCalc.length +
    liveOnlyById.length +
    liveCalcOnly.length;

  return {
    status: failures.length ? "fail" : gapCount ? "ok_with_gaps" : "ok",
    requiredFields,
    expectedKind,
    counts: {
      generatedById: generatedEntries.length,
      liveById: liveEntries.length,
      liveCalcNames: Array.isArray(liveCalcNames) ? liveCalcNames.length : 0,
      sharedIds: sharedIds.length,
      missingFromLiveById: missingFromLiveById.length,
      missingFromCalc: missingFromCalc.length,
      liveOnlyById: liveOnlyById.length,
      liveCalcOnly: liveCalcOnly.length,
      displayNameDifferences: displayNameDifferences.length,
      missingFields: missingFields.length,
      incompatibleValues: incompatibleValues.length,
    },
    sharedIds: sharedIds.slice(0, 80),
    missingFromLiveById: missingFromLiveById.slice(0, 120),
    customRepoOnly: missingFromLiveById.slice(0, 120),
    missingFromCalc: missingFromCalc.slice(0, 120),
    liveOnlyById: liveOnlyById.slice(0, 120),
    liveCalcOnly: liveCalcOnly.slice(0, 120),
    displayNameDifferences: displayNameDifferences.slice(0, 120),
    missingFields,
    incompatibleValues,
    incompatibleIds: incompatibleValues.filter((value) => value.path.endsWith(".id")),
    failures,
  };
}

function buildIdentityGapReport({
  category,
  sourceReport,
  sourceKey,
  generatedById,
  liveById,
  liveCalcNames,
  requiredFields,
  expectedKind,
  generatedAt,
  url,
  artifactPaths,
  liveCatalogs,
}) {
  const sourceFailures = validateGeneratedIdentityAgainstSource({
    sourceReport,
    sourceKey,
    generatedById,
    expectedCategory: sourceReport.category,
    expectedKind,
  });
  const comparison = compareIdentityCatalog({
    category,
    generatedById,
    liveById,
    liveCalcNames,
    requiredFields,
    expectedKind,
  });
  const failures = [...sourceFailures, ...comparison.failures];
  return {
    schemaVersion: 1,
    status: failures.length ? "fail" : comparison.status,
    appUrl: url,
    generatedAt,
    category,
    source: {
      sources: sourceReport.sources,
      fields: sourceReport.repoSourceFields,
      identityCandidateCount: sourceReport.identityCandidateCount,
      selectionRule: sourceReport.selectionRule || "All user-facing ability constants except ABILITY_NONE",
    },
    generatedArtifacts: artifactPaths,
    liveCatalogs,
    validationPolicy: {
      scope: "identity only",
      requiredFields,
      sharedIdComparison:
        "kind and id must match exactly; names must match after normalization, with exact display differences reported as nonfatal.",
      gapPolicy:
        "Repo-only and live-only entries are migration gaps, not fatal validation failures.",
    },
    counts: comparison.counts,
    sharedIds: comparison.sharedIds,
    missingFromLiveById: comparison.missingFromLiveById,
    customRepoOnly: comparison.customRepoOnly,
    missingFromCalc: comparison.missingFromCalc,
    liveOnlyById: comparison.liveOnlyById,
    liveCalcOnly: comparison.liveCalcOnly,
    displayNameDifferences: comparison.displayNameDifferences,
    missingFields: comparison.missingFields,
    incompatibleValues: comparison.incompatibleValues,
    incompatibleIds: comparison.incompatibleIds,
    failures,
  };
}

function compareMoveMetadataCatalog({ generatedById, generatedCalc, liveById, liveCalcMoves }) {
  const missingFields = [];
  const incompatibleValues = [];
  const generatedEntries = Object.entries(generatedById || {}).sort(([left], [right]) =>
    left.localeCompare(right)
  );
  const liveEntries = Object.entries(liveById || {}).sort(([left], [right]) =>
    left.localeCompare(right)
  );
  const generatedIds = new Set(generatedEntries.map(([id]) => id));
  const generatedNameSets = objectCatalogNameSets(generatedCalc || {});
  const liveCalcNameSets = objectCatalogNameSets(liveCalcMoves || {});
  const liveCalcIndex = buildNormalizedObjectIndex(liveCalcMoves || {});

  const missingFromLiveById = [];
  const missingFromCalc = [];
  const liveOnlyById = [];
  const liveCalcOnly = [];
  const sharedIds = [];
  const sharedCalcNames = [];
  const displayNameDifferences = [];

  for (const [id, generatedEntry] of generatedEntries) {
    for (const field of REQUIRED_MOVE_METADATA_FIELDS) {
      if (!(field in generatedEntry)) {
        missingFields.push(`generated.MOVES_BY_ID.${id}.${field}`);
      }
    }

    const liveEntry = liveById?.[id];
    if (!liveEntry) {
      missingFromLiveById.push(generatedEntry);
    } else {
      sharedIds.push(id);
      for (const field of REQUIRED_MOVE_METADATA_FIELDS) {
        if (!(field in liveEntry)) {
          missingFields.push(`live.MOVES_BY_ID.${id}.${field}`);
          continue;
        }
        if (field === "name") {
          if (normalize(liveEntry.name) !== normalize(generatedEntry.name)) {
            incompatibleValues.push({
              path: `MOVES_BY_ID.${id}.name`,
              generated: generatedEntry.name,
              live: liveEntry.name,
            });
          } else if (liveEntry.name !== generatedEntry.name) {
            displayNameDifferences.push({
              id,
              generated: generatedEntry.name,
              live: liveEntry.name,
            });
          }
        } else if (JSON.stringify(liveEntry[field]) !== JSON.stringify(generatedEntry[field])) {
          incompatibleValues.push({
            path: `MOVES_BY_ID.${id}.${field}`,
            generated: generatedEntry[field],
            live: liveEntry[field],
          });
        }
      }
    }

    const calcMatch =
      liveCalcMoves?.[generatedEntry.name] ||
      liveCalcIndex.get(normalize(generatedEntry.name))?.entry ||
      liveCalcIndex.get(normalize(id))?.entry;
    const calcName =
      liveCalcMoves?.[generatedEntry.name]
        ? generatedEntry.name
        : liveCalcIndex.get(normalize(generatedEntry.name))?.name ||
          liveCalcIndex.get(normalize(id))?.name ||
          "";
    if (!calcMatch) {
      missingFromCalc.push(generatedEntry);
    } else {
      sharedCalcNames.push(calcName);
      for (const field of REQUIRED_CALC_MOVE_FIELDS) {
        if (!(field in calcMatch)) {
          missingFields.push(`live.calc.MOVES[${CALC_GENERATION_INDEX}].${calcName}.${field}`);
          continue;
        }
      }
      const generatedCalcEntry = generatedCalc?.[generatedEntry.name] || {
        bp: generatedEntry.basePower,
        type: generatedEntry.type,
        category: generatedEntry.category,
      };
      for (const [generatedField, liveField] of [
        ["bp", "bp"],
        ["type", "type"],
        ["category", "category"],
      ]) {
        if (
          liveField in calcMatch &&
          JSON.stringify(calcMatch[liveField]) !== JSON.stringify(generatedCalcEntry[generatedField])
        ) {
          incompatibleValues.push({
            path: `calc.MOVES[${CALC_GENERATION_INDEX}].${calcName}.${liveField}`,
            generated: generatedCalcEntry[generatedField],
            live: calcMatch[liveField],
          });
        }
      }
    }
  }

  for (const [id, liveEntry] of liveEntries) {
    if (!generatedIds.has(id)) {
      liveOnlyById.push(liveEntry);
    }
  }
  for (const name of Object.keys(liveCalcMoves || {}).sort()) {
    if (
      !generatedNameSets.exact.has(name) &&
      !generatedNameSets.normalized.has(normalize(name))
    ) {
      liveCalcOnly.push(name);
    }
  }

  const gapCount =
    missingFromLiveById.length +
    missingFromCalc.length +
    liveOnlyById.length +
    liveCalcOnly.length +
    missingFields.length +
    incompatibleValues.length;

  return {
    status: gapCount ? "ok_with_gaps" : "ok",
    counts: {
      generatedById: generatedEntries.length,
      generatedCalc: Object.keys(generatedCalc || {}).length,
      liveById: liveEntries.length,
      liveCalcNames: Object.keys(liveCalcMoves || {}).length,
      sharedIds: sharedIds.length,
      sharedCalcNames: sharedCalcNames.length,
      missingFromLiveById: missingFromLiveById.length,
      missingFromCalc: missingFromCalc.length,
      liveOnlyById: liveOnlyById.length,
      liveCalcOnly: liveCalcOnly.length,
      displayNameDifferences: displayNameDifferences.length,
      missingFields: missingFields.length,
      incompatibleValues: incompatibleValues.length,
    },
    sharedIds: sharedIds.slice(0, 120),
    sharedCalcNames: sharedCalcNames.slice(0, 120),
    missingFromLiveById: missingFromLiveById.slice(0, 120),
    customRepoOnly: missingFromLiveById.slice(0, 120),
    missingFromCalc: missingFromCalc.slice(0, 120),
    liveOnlyById: liveOnlyById.slice(0, 120),
    liveCalcOnly: liveCalcOnly.slice(0, 120),
    displayNameDifferences: displayNameDifferences.slice(0, 120),
    missingFields,
    incompatibleValues,
    incompatibleIds: incompatibleValues.filter((value) => value.path.endsWith(".id")),
  };
}

function buildMoveMetadataGapReport({
  sourceReport,
  generatedById,
  generatedCalc,
  liveById,
  liveCalcMoves,
  generatedAt,
  url,
  artifactPaths,
  liveCatalogs,
}) {
  const sourceFailures = validateGeneratedMoveMetadataAgainstSource({
    sourceReport,
    generatedById,
    generatedCalc,
  });
  const comparison = compareMoveMetadataCatalog({
    generatedById,
    generatedCalc,
    liveById,
    liveCalcMoves,
  });
  return {
    schemaVersion: 1,
    status: sourceFailures.length ? "fail" : comparison.status,
    appUrl: url,
    generatedAt,
    category: "moves",
    source: {
      sources: sourceReport.sources,
      fields: sourceReport.repoSourceFields,
      metadataCandidateCount: sourceReport.metadataCandidateCount,
      metadataCompleteCount: sourceReport.metadataCompleteCount,
      selectionRule: sourceReport.selectionRule,
      excludedConstants: sourceReport.excludedConstants,
    },
    generatedArtifacts: artifactPaths,
    liveCatalogs,
    validationPolicy: {
      scope: "identity and basic metadata only",
      requiredByIdFields: REQUIRED_MOVE_METADATA_FIELDS,
      requiredCalcFields: REQUIRED_CALC_MOVE_FIELDS,
      sharedIdComparison:
        "kind, id, type, category, and basePower are compared when present; display names compare after normalization, with exact display differences reported as nonfatal.",
      valueGapPolicy:
        "Live and repo basic metadata differences are reported as incompatibleValues, not accepted as replacement-catalog parity.",
      gapPolicy:
        "Repo-only and live-only entries are migration gaps, not fatal validation failures.",
      deferredSemanticFields: sourceReport.deferredSemanticFields || [],
    },
    counts: comparison.counts,
    sharedIds: comparison.sharedIds,
    sharedCalcNames: comparison.sharedCalcNames,
    missingFromLiveById: comparison.missingFromLiveById,
    customRepoOnly: comparison.customRepoOnly,
    missingFromCalc: comparison.missingFromCalc,
    liveOnlyById: comparison.liveOnlyById,
    liveCalcOnly: comparison.liveCalcOnly,
    displayNameDifferences: comparison.displayNameDifferences,
    missingFields: comparison.missingFields,
    incompatibleValues: comparison.incompatibleValues,
    incompatibleIds: comparison.incompatibleIds,
    deferredSemanticFields: sourceReport.deferredSemanticFields || [],
    failures: sourceFailures,
  };
}

async function readLiveCatalogs(page) {
  return page.evaluate(() => {
    function readExpr(expr) {
      try {
        return { ok: true, value: Function(`return (${expr})`)() };
      } catch (error) {
        return { ok: false, error: error.message };
      }
    }

    function summarizeCatalog(name, expr) {
      const read = readExpr(expr);
      if (!read.ok) {
        return { name, expression: expr, ok: false, error: read.error };
      }
      const value = read.value;
      const containerType = Array.isArray(value) ? "array" : typeof value;
      let entryCount = null;
      let sampleKey = "";
      let samplePath = "";
      let sample = null;
      if (value && typeof value === "object") {
        const keys = Object.keys(value);
        entryCount = keys.length;
        const representative = findRepresentativeEntry(value);
        sampleKey = representative.key;
        samplePath = representative.path;
        sample = representative.value;
      }
      return {
        name,
        expression: expr,
        ok: true,
        containerType,
        entryCount,
        sampleKey,
        samplePath,
        sampleFields: sample && typeof sample === "object" && !Array.isArray(sample) ? Object.keys(sample).sort() : [],
        sampleType: Array.isArray(sample) ? "array" : typeof sample,
      };
    }

    function findRepresentativeEntry(value, path = [], depth = 0) {
      if (!value || typeof value !== "object" || depth > 4) {
        return { key: path[path.length - 1] || "", path: path.join("."), value };
      }
      if (isCatalogEntry(value)) {
        return { key: path[path.length - 1] || "", path: path.join("."), value };
      }

      const entries = Object.entries(value);
      for (const [key, child] of entries) {
        if (!child || typeof child !== "object" || Array.isArray(child)) {
          continue;
        }
        const result = findRepresentativeEntry(child, [...path, key], depth + 1);
        if (result.value && typeof result.value === "object" && isCatalogEntry(result.value)) {
          return result;
        }
      }
      for (const [key, child] of entries) {
        if (["string", "number", "boolean"].includes(typeof child)) {
          return { key, path: [...path, key].join("."), value: child };
        }
      }
      for (const [key, child] of entries) {
        if (!child || typeof child !== "object" || Array.isArray(child)) {
          continue;
        }
        const result = findRepresentativeEntry(child, [...path, key], depth + 1);
        if (["string", "number", "boolean"].includes(typeof result.value)) {
          return result;
        }
      }

      const [key, child] = entries[0] || ["", null];
      return { key, path: [...path, key].filter(Boolean).join("."), value: child };
    }

    function isCatalogEntry(value) {
      if (!value || typeof value !== "object" || Array.isArray(value)) {
        return false;
      }
      const has = (field) => Object.prototype.hasOwnProperty.call(value, field);
      return (
        has("kind") ||
        has("id") ||
        has("name") ||
        has("baseStats") ||
        has("bs") ||
        (has("bp") && has("category")) ||
        (has("desc") && has("name"))
      );
    }

    function collectIdentityCatalog(value, expectedKind, entries = {}, depth = 0) {
      if (value == null || depth > 5) {
        return entries;
      }
      if (Array.isArray(value)) {
        for (const child of value) {
          collectIdentityCatalog(child, expectedKind, entries, depth + 1);
        }
        return entries;
      }
      if (typeof value !== "object") {
        return entries;
      }

      const hasIdentity =
        typeof value.id === "string" &&
        typeof value.name === "string" &&
        (!expectedKind || value.kind === expectedKind);
      if (hasIdentity) {
        entries[value.id] = JSON.parse(JSON.stringify(value));
      }

      for (const child of Object.values(value)) {
        if (child && typeof child === "object") {
          collectIdentityCatalog(child, expectedKind, entries, depth + 1);
        }
      }
      return entries;
    }

    const catalogs = {
      species: [
        summarizeCatalog("SPECIES_BY_ID", "SPECIES_BY_ID"),
        summarizeCatalog("calc.SPECIES[4]", "calc.SPECIES[4]"),
      ],
      moves: [
        summarizeCatalog("MOVES_BY_ID", "MOVES_BY_ID"),
        summarizeCatalog("calc.MOVES[4]", "calc.MOVES[4]"),
      ],
      abilities: [
        summarizeCatalog("ABILITIES_BY_ID", "ABILITIES_BY_ID"),
        summarizeCatalog("calc.ABILITIES[4]", "calc.ABILITIES[4]"),
      ],
      items: [
        summarizeCatalog("ITEMS_BY_ID", "ITEMS_BY_ID"),
        summarizeCatalog("calc.ITEMS[4]", "calc.ITEMS[4]"),
      ],
      natures: [
        summarizeCatalog("NATURES_BY_ID", "NATURES_BY_ID"),
        summarizeCatalog("calc.NATURES", "calc.NATURES"),
      ],
    };
    return {
      catalogs,
      naturesById: JSON.parse(JSON.stringify(NATURES_BY_ID)),
      calcNatures: JSON.parse(JSON.stringify(calc.NATURES)),
      movesById: collectIdentityCatalog(MOVES_BY_ID, "Move"),
      calcMoves: JSON.parse(JSON.stringify(calc.MOVES?.[4] || {})),
      abilitiesById: collectIdentityCatalog(ABILITIES_BY_ID, "Ability"),
      itemsById: collectIdentityCatalog(ITEMS_BY_ID, "Item"),
      calcAbilities: JSON.parse(JSON.stringify(calc.ABILITIES?.[4] || [])),
      calcItems: JSON.parse(JSON.stringify(calc.ITEMS?.[4] || [])),
      symbolTypes: {
        NATURES_BY_ID: typeof NATURES_BY_ID,
        MOVES_BY_ID: typeof MOVES_BY_ID,
        ABILITIES_BY_ID: typeof ABILITIES_BY_ID,
        ITEMS_BY_ID: typeof ITEMS_BY_ID,
        calc: typeof calc,
        calcNATURES: typeof calc?.NATURES,
        calcMOVES: typeof calc?.MOVES,
        calcABILITIES: typeof calc?.ABILITIES,
        calcITEMS: typeof calc?.ITEMS,
        Nature: typeof Nature,
      },
    };
  });
}

function buildReports({ artifacts, live, url, pageErrors, consoleErrors, failedRequests }) {
  const natureGeneratedFailures = validateGeneratedAgainstSource(artifacts);
  const abilityGeneratedFailures = validateGeneratedIdentityAgainstSource({
    sourceReport: artifacts.sourceAbilities,
    sourceKey: "abilities",
    generatedById: artifacts.abilitiesById,
    expectedCategory: "abilities",
    expectedKind: "Ability",
  });
  const heldItemGeneratedFailures = validateGeneratedIdentityAgainstSource({
    sourceReport: artifacts.sourceHeldItems,
    sourceKey: "heldItems",
    generatedById: artifacts.heldItemsById,
    expectedCategory: "heldItems",
    expectedKind: "Item",
  });
  const moveGeneratedFailures = validateGeneratedMoveMetadataAgainstSource({
    sourceReport: artifacts.sourceMoves,
    generatedById: artifacts.movesById,
    generatedCalc: artifacts.calcMoves,
  });
  const byIdComparison = compareById(artifacts.naturesById, live.naturesById);
  const calcComparison = compareCalcNatures(artifacts.calcNatures, live.calcNatures);
  const requiredMissingFields = [
    ...byIdComparison.missingFields,
    ...REQUIRED_NATURE_FIELDS.filter(
      (field) => !objectFieldShape(Object.values(live.naturesById || {})[0]).includes(field)
    ).map((field) => `NATURES_BY_ID.<entry>.${field}`),
  ];
  const incompatibleValues = [
    ...byIdComparison.incompatibleValues,
    ...calcComparison.incompatibleValues,
  ];
  const generatedAt = new Date().toISOString().replace(/\.\d{3}Z$/, "Z");
  const natureArtifactPaths = {
    sourceNatures: relpathOrAbs(artifacts.sourceNaturesPath),
    naturesById: relpathOrAbs(artifacts.naturesByIdPath),
    calcNatures: relpathOrAbs(artifacts.calcNaturesPath),
    naturesJs: relpathOrAbs(artifacts.naturesJsPath),
  };
  const abilityArtifactPaths = {
    sourceAbilities: relpathOrAbs(artifacts.sourceAbilitiesPath),
    abilitiesById: relpathOrAbs(artifacts.abilitiesByIdPath),
    abilitiesJs: relpathOrAbs(artifacts.abilitiesJsPath),
  };
  const heldItemArtifactPaths = {
    sourceHeldItems: relpathOrAbs(artifacts.sourceHeldItemsPath),
    heldItemsById: relpathOrAbs(artifacts.heldItemsByIdPath),
    heldItemsJs: relpathOrAbs(artifacts.heldItemsJsPath),
  };
  const moveArtifactPaths = {
    sourceMoves: relpathOrAbs(artifacts.sourceMovesPath),
    movesById: relpathOrAbs(artifacts.movesByIdPath),
    calcMoves: relpathOrAbs(artifacts.calcMovesPath),
    movesJs: relpathOrAbs(artifacts.movesJsPath),
  };
  const abilityGapReport = buildIdentityGapReport({
    category: "abilities",
    sourceReport: artifacts.sourceAbilities,
    sourceKey: "abilities",
    generatedById: artifacts.abilitiesById,
    liveById: live.abilitiesById,
    liveCalcNames: live.calcAbilities,
    requiredFields: REQUIRED_ABILITY_FIELDS,
    expectedKind: "Ability",
    generatedAt,
    url,
    artifactPaths: abilityArtifactPaths,
    liveCatalogs: live.catalogs.abilities,
  });
  const heldItemGapReport = buildIdentityGapReport({
    category: "heldItems",
    sourceReport: artifacts.sourceHeldItems,
    sourceKey: "heldItems",
    generatedById: artifacts.heldItemsById,
    liveById: live.itemsById,
    liveCalcNames: live.calcItems,
    requiredFields: REQUIRED_ITEM_IDENTITY_FIELDS,
    expectedKind: "Item",
    generatedAt,
    url,
    artifactPaths: heldItemArtifactPaths,
    liveCatalogs: live.catalogs.items,
  });
  const moveGapReport = buildMoveMetadataGapReport({
    sourceReport: artifacts.sourceMoves,
    generatedById: artifacts.movesById,
    generatedCalc: artifacts.calcMoves,
    liveById: live.movesById,
    liveCalcMoves: live.calcMoves,
    generatedAt,
    url,
    artifactPaths: moveArtifactPaths,
    liveCatalogs: live.catalogs.moves,
  });
  const failures = [
    ...natureGeneratedFailures,
    ...abilityGeneratedFailures,
    ...heldItemGeneratedFailures,
    ...moveGeneratedFailures,
    ...abilityGapReport.failures,
    ...heldItemGapReport.failures,
    ...moveGapReport.failures,
    ...byIdComparison.failures,
    ...calcComparison.failures,
    ...pageErrors.map((error) => `page error: ${error}`),
    ...consoleErrors.map((error) => `console error: ${error}`),
    ...failedRequests.map(
      (request) =>
        `failed request: ${request.type} ${request.url} ${request.error || ""}`.trim()
    ),
  ];

  const selectedCategoryMapping = {
    status: failures.length ? "fail" : "ready",
    repoSourceFields: artifacts.sourceNatures.repoSourceFields,
    pkcalcFields: REQUIRED_NATURE_FIELDS,
    fieldMap: artifacts.sourceNatures.pkcalcFieldMapping,
    missingFields: requiredMissingFields,
    incompatibleValues,
    generatedArtifacts: natureArtifactPaths,
  };

  const mappingReport = {
    schemaVersion: 1,
    status: failures.length ? "fail" : "ok",
    appUrl: url,
    generatedAt,
    chosenMigrationCategory: "natures",
    identityProofCategories: ["abilities", "heldItems"],
    metadataProofCategories: ["moves"],
    selectionRationale:
      "Natures remain the replacement-catalog proof because they have deterministic stat fields. Abilities and held items are identity-only proofs with explicit gap reports. Moves are an identity/basic-metadata proof only; effects, flags, and damage-calc semantics remain deferred.",
    liveCatalogShapes: live.catalogs,
    repoSourceMappings: {
      natures: selectedCategoryMapping,
      abilities: {
        status: abilityGapReport.status,
        repoSources: Object.values(artifacts.sourceAbilities.sources),
        repoSourceFields: artifacts.sourceAbilities.repoSourceFields,
        pkcalcFields: REQUIRED_ABILITY_FIELDS,
        fieldMap: artifacts.sourceAbilities.pkcalcFieldMapping,
        liveCatalogs: live.catalogs.abilities,
        missingFields: abilityGapReport.missingFields,
        incompatibleValues: abilityGapReport.incompatibleValues,
        generatedArtifacts: abilityArtifactPaths,
        gapReport: "ability_identity_gap_report.json",
        gapCounts: abilityGapReport.counts,
        deferredSemantics: [
          "Ability descriptions and flags are source traceability only in this slice.",
          "Ability behavior and calculator parity are not asserted.",
        ],
      },
      heldItems: {
        status: heldItemGapReport.status,
        repoSources: Object.values(artifacts.sourceHeldItems.sources),
        repoSourceFields: artifacts.sourceHeldItems.repoSourceFields,
        pkcalcFields: REQUIRED_ITEM_IDENTITY_FIELDS,
        fieldMap: artifacts.sourceHeldItems.pkcalcFieldMapping,
        liveCatalogs: live.catalogs.items,
        missingFields: heldItemGapReport.missingFields,
        incompatibleValues: heldItemGapReport.incompatibleValues,
        generatedArtifacts: heldItemArtifactPaths,
        gapReport: "held_item_identity_gap_report.json",
        gapCounts: heldItemGapReport.counts,
        deferredSemantics: [
          "Hold effects and parameters are source traceability only in this slice.",
          "Item behavior, mega evolution behavior, and damage-calculator parity are not asserted.",
        ],
      },
      moves: {
        status: moveGapReport.status,
        repoSources: Object.values(artifacts.sourceMoves.sources),
        repoSourceFields: artifacts.sourceMoves.repoSourceFields,
        pkcalcFields: REQUIRED_MOVE_METADATA_FIELDS,
        pkcalcCalcFields: REQUIRED_CALC_MOVE_FIELDS,
        fieldMap: artifacts.sourceMoves.pkcalcFieldMapping,
        liveCatalogs: live.catalogs.moves,
        missingFields: moveGapReport.missingFields,
        incompatibleValues: moveGapReport.incompatibleValues,
        generatedArtifacts: moveArtifactPaths,
        gapReport: "move_metadata_gap_report.json",
        gapCounts: moveGapReport.counts,
        deferredSemantics: [
          "Move effects, flags, secondary effects, targets, recoil, and multi-hit behavior are source traceability only in this slice.",
          "Damage-calculator parity and default overlay replacement are not asserted.",
        ],
      },
    },
    pageDiagnostics: {
      symbolTypes: live.symbolTypes,
      pageErrors,
      consoleErrors,
      failedRequests,
    },
    failures,
  };

  const validationReport = {
    schemaVersion: 1,
    status: failures.length ? "fail" : "ok",
    appUrl: url,
    generatedAt,
    category: "natures",
    source: {
      count: artifacts.sourceNatures.natures.length,
      sources: artifacts.sourceNatures.sources,
      fields: artifacts.sourceNatures.repoSourceFields,
      statMapping: artifacts.sourceNatures.statMapping,
    },
    generatedArtifacts: natureArtifactPaths,
    liveExpectations: {
      naturesByIdFields: REQUIRED_NATURE_FIELDS,
      calcNaturesShape: "display name -> [plus, minus]",
    },
    checks: {
      sourceHasNoFailures: artifacts.sourceNatures.failures.length === 0,
      sourceCount: artifacts.sourceNatures.natures.length,
      generatedByIdCount: Object.keys(artifacts.naturesById).length,
      generatedCalcCount: Object.keys(artifacts.calcNatures).length,
      generatedMatchesSource: natureGeneratedFailures.length === 0,
      liveByIdMatchesGenerated: byIdComparison.failures.length === 0,
      liveCalcMatchesGenerated: calcComparison.failures.length === 0,
      missingFieldCount: requiredMissingFields.length,
      incompatibleValueCount: incompatibleValues.length,
      abilityIdentityStatus: abilityGapReport.status,
      heldItemIdentityStatus: heldItemGapReport.status,
      moveMetadataStatus: moveGapReport.status,
    },
    missingFields: requiredMissingFields,
    incompatibleValues,
    failures,
  };

  return { mappingReport, validationReport, abilityGapReport, heldItemGapReport, moveGapReport };
}

async function main() {
  const {
    inputDir,
    mappingReport,
    validationReport,
    abilityGapReport,
    heldItemGapReport,
    moveGapReport,
    url,
  } = parseArgs(process.argv);
  const artifacts = loadGeneratedArtifacts(inputDir);
  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({ serviceWorkers: "block" });
  const page = await context.newPage();
  const pageErrors = [];
  const consoleErrors = [];
  const failedRequests = [];
  const appOrigin = new URL(url).origin;

  page.on("pageerror", (error) => pageErrors.push(error.message));
  page.on("console", (message) => {
    if (message.type() === "error") {
      consoleErrors.push(message.text());
    }
  });
  page.on("requestfailed", (request) => {
    const requestUrl = request.url();
    const resourceType = request.resourceType();
    if (
      requestUrl.startsWith(appOrigin) &&
      ["document", "script", "stylesheet", "xhr", "fetch"].includes(resourceType)
    ) {
      failedRequests.push({
        url: requestUrl,
        type: resourceType,
        error: request.failure()?.errorText,
      });
    }
  });

  await page.goto(url, { waitUntil: "load", timeout: 90000 });
  await page.waitForFunction(
    () =>
      typeof calc === "object" &&
      typeof NATURES_BY_ID === "object" &&
      typeof MOVES_BY_ID === "object" &&
      typeof ABILITIES_BY_ID === "object" &&
      typeof ITEMS_BY_ID === "object" &&
      typeof SPECIES_BY_ID === "object",
    null,
    { timeout: 60000 }
  );
  const live = await readLiveCatalogs(page);
  await browser.close();

  const reports = buildReports({
    artifacts,
    live,
    url,
    pageErrors,
    consoleErrors,
    failedRequests,
  });
  writeJson(mappingReport, reports.mappingReport);
  writeJson(validationReport, reports.validationReport);
  writeJson(abilityGapReport, reports.abilityGapReport);
  writeJson(heldItemGapReport, reports.heldItemGapReport);
  writeJson(moveGapReport, reports.moveGapReport);

  console.log(`Wrote ${relpathOrAbs(mappingReport)}`);
  console.log(`Wrote ${relpathOrAbs(validationReport)}`);
  console.log(`Wrote ${relpathOrAbs(abilityGapReport)}`);
  console.log(`Wrote ${relpathOrAbs(heldItemGapReport)}`);
  console.log(`Wrote ${relpathOrAbs(moveGapReport)}`);
  if (reports.validationReport.status !== "ok") {
    console.error("PKCalc data migration validation failed:");
    for (const failure of reports.validationReport.failures) {
      console.error(`- ${failure}`);
    }
    process.exit(1);
  }
  console.log(
    "PKCalc data migration audit passed: chose natures, "
      + `${reports.validationReport.checks.sourceCount} source entries, `
      + `${reports.validationReport.checks.generatedByIdCount} NATURES_BY_ID entries; `
      + `ability identity ${reports.abilityGapReport.status}, `
      + `held-item identity ${reports.heldItemGapReport.status}, `
      + `move metadata ${reports.moveGapReport.status}`
  );
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exit(1);
});
