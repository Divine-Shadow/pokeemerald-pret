#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const REPO_ROOT = path.resolve(__dirname, "../..");
const DEFAULT_OUTPUT_DIR = path.join(REPO_ROOT, "build/tracker_export");
const DEFAULT_CONTRACT_PATH = path.join(
  __dirname,
  "pkcalc_compat_contract.json"
);
const DEFAULT_PKCALC_URL =
  process.env.PKCALC_URL || "https://pkcalc.anastarawneh.com/";
const REQUEST_KEYS = ["party", "sets", "locations"];
const REQUIRED_CATEGORIES = [
  "species",
  "moves",
  "abilities",
  "items",
  "natures",
  "encounterSpecies",
];
const POLL_MS = 500;

function parseArgs(argv) {
  let outputDir = DEFAULT_OUTPUT_DIR;
  let adapterRoot = "";
  let referenceReport = "";
  let report = "";
  let contractPath = DEFAULT_CONTRACT_PATH;
  let url = DEFAULT_PKCALC_URL;

  for (let i = 2; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--output-dir") {
      outputDir = requiredArg(argv, ++i, arg);
    } else if (arg === "--adapter-root") {
      adapterRoot = requiredArg(argv, ++i, arg);
    } else if (arg === "--reference-report") {
      referenceReport = requiredArg(argv, ++i, arg);
    } else if (arg === "--report") {
      report = requiredArg(argv, ++i, arg);
    } else if (arg === "--contract") {
      contractPath = requiredArg(argv, ++i, arg);
    } else if (arg === "--url") {
      url = requiredArg(argv, ++i, arg);
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: live_reference_tracker_export_playwright.cjs [--output-dir DIR] [--adapter-root DIR] [--reference-report FILE] [--report FILE] [--contract FILE] [--url URL]"
      );
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }

  outputDir = resolveFromRoot(outputDir);
  if (adapterRoot) {
    adapterRoot = resolveFromRoot(adapterRoot);
  }
  contractPath = resolveFromRoot(contractPath);
  referenceReport = referenceReport
    ? resolveFromRoot(referenceReport)
    : path.join(outputDir, "reference_report.json");
  report = report
    ? resolveFromRoot(report)
    : path.join(outputDir, "live_reference_report.json");

  return { adapterRoot, contractPath, outputDir, referenceReport, report, url };
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

function assertExists(filePath) {
  if (!fs.existsSync(filePath)) {
    throw new Error(`Missing generated tracker artifact: ${filePath}`);
  }
}

function requireObject(errors, value, name) {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    errors.push(`${name} must be an object`);
    return false;
  }
  return true;
}

function requireString(errors, value, name) {
  if (!value || typeof value !== "string") {
    errors.push(`${name} must be a non-empty string`);
  }
}

function loadContract(contractPath) {
  const contract = JSON.parse(fs.readFileSync(contractPath, "utf8"));
  const errors = [];

  if (contract.schemaVersion !== 1) {
    errors.push("schemaVersion must be 1");
  }
  if (requireObject(errors, contract.requestPaths, "requestPaths")) {
    for (const key of REQUEST_KEYS) {
      requireString(errors, contract.requestPaths[key], `requestPaths.${key}`);
      if (
        typeof contract.requestPaths[key] === "string" &&
        !contract.requestPaths[key].startsWith("/")
      ) {
        errors.push(`requestPaths.${key} must start with /`);
      }
    }
  }
  if (requireObject(errors, contract.constants, "constants")) {
    for (const name of ["SETDEX_PK", "PARTY_ORDER_PK", "LOCATIONS"]) {
      requireString(errors, contract.constants[name], `constants.${name}`);
    }
  }
  if (requireObject(errors, contract.appGlobals, "appGlobals")) {
    for (const name of ["$", "getSetOptions", "loadDexEntry", "SETDEX"]) {
      requireString(errors, contract.appGlobals[name], `appGlobals.${name}`);
    }
  }
  if (requireObject(errors, contract.dataRoutes, "dataRoutes")) {
    const routes = contract.dataRoutes;
    if (routes.setdexSource !== "SETDEX") {
      errors.push("dataRoutes.setdexSource must be SETDEX");
    }
    if (!Number.isInteger(routes.setdexGenerationIndex)) {
      errors.push("dataRoutes.setdexGenerationIndex must be an integer");
    }
    if (routes.setdexConstant !== "SETDEX_PK") {
      errors.push("dataRoutes.setdexConstant must be SETDEX_PK");
    }
    if (routes.partyOrderSource !== "partyOrder") {
      errors.push("dataRoutes.partyOrderSource must be partyOrder");
    }
    if (routes.partyOrderConstant !== "PARTY_ORDER_PK") {
      errors.push("dataRoutes.partyOrderConstant must be PARTY_ORDER_PK");
    }
  }

  if (errors.length) {
    throw new Error(
      `Invalid PKCalc compatibility contract ${contractPath}:\n- ${errors.join(
        "\n- "
      )}`
    );
  }
  return contract;
}

function loadReferenceReport(referenceReport) {
  const report = JSON.parse(fs.readFileSync(referenceReport, "utf8"));
  const errors = [];

  if (report.schemaVersion !== 1) {
    errors.push("schemaVersion must be 1");
  }
  if (report.failures?.length) {
    errors.push("reference_report.json has failures; run tracker-export-reference-check");
  }
  if (requireObject(errors, report.categories, "categories")) {
    for (const category of REQUIRED_CATEGORIES) {
      const data = report.categories[category];
      if (!requireObject(errors, data, `categories.${category}`)) {
        continue;
      }
      if (!Number.isInteger(data.count)) {
        errors.push(`categories.${category}.count must be an integer`);
      }
      requireObject(errors, data.values, `categories.${category}.values`);
    }
  }

  if (errors.length) {
    throw new Error(
      `Invalid tracker reference report ${referenceReport}:\n- ${errors.join(
        "\n- "
      )}`
    );
  }
  return report;
}

function referencePayload(report) {
  return Object.fromEntries(
    REQUIRED_CATEGORIES.map((category) => [
      category,
      Object.entries(report.categories[category].values).map(
        ([value, details]) => ({
          value,
          normalized: details.normalized || normalizeReference(value),
          occurrences: details.occurrences || 0,
        })
      ),
    ])
  );
}

function normalizeReference(value) {
  return String(value || "")
    .toLowerCase()
    .replace(/[^a-z0-9]/g, "");
}

function stripLeadingSlashes(value) {
  return value.replace(/^\/+/, "");
}

function generatedFiles(outputDir, adapterRoot, contract) {
  if (adapterRoot) {
    return Object.fromEntries(
      REQUEST_KEYS.map((key) => [
        key,
        path.join(adapterRoot, stripLeadingSlashes(contract.requestPaths[key])),
      ])
    );
  }

  const pkcalcDir = path.join(outputDir, "pkcalc");
  return {
    party: path.join(pkcalcDir, "party_order.js"),
    sets: path.join(pkcalcDir, "sets.js"),
    locations: path.join(pkcalcDir, "locations.js"),
  };
}

function escapeRegExp(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function requestPathPattern(requestPath) {
  return new RegExp(`${escapeRegExp(requestPath)}(?:[?#].*)?$`);
}

function summarizeRequestPaths(contract, intercepted) {
  return Object.fromEntries(
    REQUEST_KEYS.map((key) => {
      const count = intercepted[key] || 0;
      return [
        key,
        {
          path: contract.requestPaths[key],
          intercepted: count,
          expected: `intercept ${contract.requestPaths[key]}`,
          actual: `${count} interception${count === 1 ? "" : "s"}`,
          passed: count > 0,
        },
      ];
    })
  );
}

function collectSectionFailures(prefix, checks) {
  const failures = [];
  for (const [name, check] of Object.entries(checks || {})) {
    if (!check.passed) {
      const detail =
        "expected" in check && "actual" in check
          ? ` expected ${JSON.stringify(check.expected)}, saw ${JSON.stringify(
              check.actual
            )}`
          : "";
      failures.push(`${prefix}.${name}${detail}`);
    }
  }
  return failures;
}

function collectPreflightFailures(preflight) {
  const failures = [];
  if (preflight.evaluationError) {
    failures.push(`page.evaluate preflight failed: ${preflight.evaluationError}`);
  }
  failures.push(
    ...collectSectionFailures("requestPaths", preflight.requestPaths),
    ...collectSectionFailures("constants", preflight.constants),
    ...collectSectionFailures("appGlobals", preflight.appGlobals)
  );
  return failures;
}

async function readPreflightState(page, contract, intercepted) {
  let pageState;
  try {
    pageState = await page.evaluate((contractInPage) => {
      function safeType(readType) {
        try {
          return readType();
        } catch (error) {
          return `error: ${error.message}`;
        }
      }

      const symbolTypes = {
        $: safeType(() => typeof $),
        getSetOptions: safeType(() => typeof getSetOptions),
        loadDexEntry: safeType(() => typeof loadDexEntry),
        SETDEX: safeType(() => typeof SETDEX),
        SETDEX_PK: safeType(() => typeof SETDEX_PK),
        PARTY_ORDER_PK: safeType(() => typeof PARTY_ORDER_PK),
        LOCATIONS: safeType(() => typeof LOCATIONS),
        partyOrder: safeType(() => typeof partyOrder),
      };

      function summarizeExpected(expectedTypes) {
        return Object.fromEntries(
          Object.entries(expectedTypes).map(([name, expected]) => [
            name,
            {
              expected,
              actual: symbolTypes[name] || "unsupported-symbol",
              passed: symbolTypes[name] === expected,
            },
          ])
        );
      }

      return {
        readyState: document.readyState,
        symbolTypes,
        constants: summarizeExpected(contractInPage.constants),
        appGlobals: summarizeExpected(contractInPage.appGlobals),
      };
    }, contract);
  } catch (error) {
    pageState = {
      readyState: "unknown",
      symbolTypes: {},
      constants: {},
      appGlobals: {},
      evaluationError: error.message,
    };
  }

  return {
    ...pageState,
    requestPaths: summarizeRequestPaths(contract, intercepted),
  };
}

async function waitForContractPreflight(page, contract, intercepted, timeoutMs) {
  const startedAt = Date.now();
  let preflight = await readPreflightState(page, contract, intercepted);
  let failedAssumptions = collectPreflightFailures(preflight);

  while (failedAssumptions.length && Date.now() - startedAt < timeoutMs) {
    await page.waitForTimeout(POLL_MS);
    preflight = await readPreflightState(page, contract, intercepted);
    failedAssumptions = collectPreflightFailures(preflight);
  }

  return {
    ...preflight,
    failedAssumptions,
    waitedMs: Date.now() - startedAt,
    passed: failedAssumptions.length === 0,
  };
}

async function runLiveReferenceAudit(page, contract, references) {
  return page.evaluate(
    ({ contractInPage, referencesInPage }) => {
      function normalize(value) {
        return String(value || "")
          .toLowerCase()
          .replace(/[^a-z0-9]/g, "");
      }

      function readExpr(expr) {
        try {
          return { ok: true, value: Function(`return (${expr})`)() };
        } catch (error) {
          return { ok: false, error: error.message };
        }
      }

      function addName(names, value) {
        if (typeof value !== "string" || !value) {
          return;
        }
        names.exact.add(value);
        names.normalized.add(normalize(value));
      }

      function collectCatalogNames(value, names = makeNameSet(), depth = 0) {
        if (value == null || depth > 3) {
          return names;
        }
        if (typeof value === "string") {
          addName(names, value);
          return names;
        }
        if (Array.isArray(value)) {
          for (const item of value) {
            collectCatalogNames(item, names, depth + 1);
          }
          return names;
        }
        if (typeof value === "object") {
          for (const [key, item] of Object.entries(value)) {
            addName(names, key);
            if (item && typeof item === "object") {
              addName(names, item.id);
              addName(names, item.name);
              addName(names, item.displayName);
              addName(names, item.baseSpecies);
            } else {
              addName(names, item);
            }
          }
        }
        return names;
      }

      function makeNameSet() {
        return { exact: new Set(), normalized: new Set() };
      }

      function sortedSample(set, count) {
        return [...set].sort().slice(0, count);
      }

      function missingReferences(refs, names) {
        return refs.filter(
          (ref) =>
            !names.exact.has(ref.value) && !names.normalized.has(ref.normalized)
        );
      }

      function summarizeCandidate(candidate, refs) {
        const read = readExpr(candidate.expression);
        if (!read.ok) {
          return {
            ...candidate,
            ok: false,
            error: read.error,
            referenceCount: refs.length,
            resolvedCount: 0,
            missingCount: refs.length,
            missing: refs.slice(0, 40),
          };
        }

        const names = collectCatalogNames(read.value);
        const missing = missingReferences(refs, names);
        const type = Array.isArray(read.value) ? "array" : typeof read.value;
        const rawCount =
          read.value && typeof read.value === "object"
            ? Object.keys(read.value).length
            : null;
        return {
          ...candidate,
          ok: true,
          type,
          rawCount,
          exactNameCount: names.exact.size,
          normalizedNameCount: names.normalized.size,
          referenceCount: refs.length,
          resolvedCount: refs.length - missing.length,
          missingCount: missing.length,
          missing: missing.slice(0, 40),
          sampleNames: sortedSample(names.exact, 12),
          sampleNormalizedNames: sortedSample(names.normalized, 12),
        };
      }

      function auditCategory(category, candidates) {
        const refs = referencesInPage[category] || [];
        const candidateResults = candidates.map((candidate) =>
          summarizeCandidate(candidate, refs)
        );
        const selected =
          candidateResults.find((candidate) => candidate.ok && candidate.missingCount === 0) ||
          candidateResults
            .filter((candidate) => candidate.ok)
            .sort((a, b) => a.missingCount - b.missingCount)[0] ||
          candidateResults[0];
        const passed = !!selected?.ok && selected.missingCount === 0;
        return {
          status: passed ? "checked" : "fail",
          mode: "exhaustive",
          matching: "exact display name or normalized identifier",
          selected: selected
            ? {
                expression: selected.expression,
                source: selected.source,
                referenceCount: selected.referenceCount,
                resolvedCount: selected.resolvedCount,
                missingCount: selected.missingCount,
              }
            : null,
          candidates: candidateResults,
          unresolved: selected?.missing || refs.slice(0, 40),
          unresolvedCount: selected?.missingCount ?? refs.length,
          count: refs.length,
        };
      }

      function buildSpeciesNameById() {
        const read = readExpr("SPECIES_BY_ID");
        const names = new Map();
        if (!read.ok || !Array.isArray(read.value)) {
          return names;
        }
        for (const generation of read.value) {
          if (!generation || typeof generation !== "object") {
            continue;
          }
          for (const [key, speciesData] of Object.entries(generation)) {
            const normalizedKey = normalize(key);
            const displayName =
              speciesData && typeof speciesData === "object"
                ? speciesData.name || key
                : key;
            if (!names.has(normalizedKey)) {
              names.set(normalizedKey, displayName);
            }
            if (speciesData?.id && !names.has(normalize(speciesData.id))) {
              names.set(normalize(speciesData.id), displayName);
            }
          }
        }
        return names;
      }

      function counter(values) {
        const counts = new Map();
        for (const value of values) {
          counts.set(value, (counts.get(value) || 0) + 1);
        }
        return counts;
      }

      function diffCounters(expected, actual) {
        const missing = [];
        const unexpected = [];
        const keys = new Set([...expected.keys(), ...actual.keys()]);
        for (const key of [...keys].sort()) {
          const expectedCount = expected.get(key) || 0;
          const actualCount = actual.get(key) || 0;
          if (expectedCount > actualCount) {
            missing.push({ normalized: key, count: expectedCount - actualCount });
          } else if (actualCount > expectedCount) {
            unexpected.push({ normalized: key, count: actualCount - expectedCount });
          }
        }
        return { missing, unexpected };
      }

      function auditLocationRendering() {
        const speciesNameById = buildSpeciesNameById();
        const locations = Object.entries(LOCATIONS || {}).filter(
          ([, location]) =>
            Array.isArray(location?.encounters) && location.encounters.length > 0
        );
        const failures = [];
        const samples = [];
        let renderedRows = 0;
        let expectedRows = 0;
        let routeErrors = 0;

        for (const [locationId, location] of locations) {
          const expectedNames = location.encounters.map((encounter) => {
            const speciesId = normalize(encounter.species);
            return normalize(speciesNameById.get(speciesId) || encounter.species);
          });
          expectedRows += expectedNames.length;
          try {
            loadDexEntry(`location/${locationId}`);
          } catch (error) {
            routeErrors++;
            failures.push({
              locationId,
              reason: "loadDexEntry threw",
              error: error.message,
            });
            continue;
          }

          const rows = [
            ...document.querySelectorAll(".dex-info.location .location-species"),
          ].map((element) => ({
            name: element.querySelector(".name")?.textContent || "",
            level: element.querySelector(".level")?.textContent || "",
            method: element.querySelector(".method")?.getAttribute("title") || "",
          }));
          renderedRows += rows.length;
          const actualNames = rows.map((row) => normalize(row.name));
          const expectedCounts = counter(expectedNames);
          const actualCounts = counter(actualNames);
          const diff = diffCounters(expectedCounts, actualCounts);
          const passed =
            rows.length === location.encounters.length &&
            diff.missing.length === 0 &&
            diff.unexpected.length === 0;

          if (!passed) {
            failures.push({
              locationId,
              reason: "rendered encounter rows did not match LOCATIONS encounters",
              expectedRows: location.encounters.length,
              actualRows: rows.length,
              missing: diff.missing.slice(0, 20),
              unexpected: diff.unexpected.slice(0, 20),
            });
          }
          if (samples.length < 5) {
            samples.push({
              locationId,
              expectedRows: location.encounters.length,
              actualRows: rows.length,
              species: rows.slice(0, 5).map((row) => row.name),
            });
          }
        }

        return {
          status: failures.length ? "fail" : "checked",
          mode: "exhaustive",
          route: "loadDexEntry('location/<locationId>')",
          locationsWithEncounters: locations.length,
          expectedRows,
          renderedRows,
          routeErrors,
          failures: failures.slice(0, 40),
          failureCount: failures.length,
          samples,
        };
      }

      const generation = contractInPage.dataRoutes.setdexGenerationIndex;
      const categoryCandidates = {
        species: [
          {
            expression: "species",
            source: "live PKCalc Dex species display catalog",
          },
          {
            expression: "SPECIES_BY_ID",
            source: "live PKCalc Dex species identifier catalog",
          },
          {
            expression: `SETDEX[${generation}]`,
            source: "routed generated trainer set species keys",
          },
        ],
        moves: [
          {
            expression: "MOVES_BY_ID",
            source: "live PKCalc Dex move identifier catalog",
          },
          { expression: "moves", source: "live PKCalc calculator move catalog" },
          {
            expression: `calc.MOVES[${generation}]`,
            source: "live PKCalc generation-specific calculator moves",
          },
        ],
        abilities: [
          {
            expression: "ABILITIES_BY_ID",
            source: "live PKCalc Dex ability identifier catalog",
          },
          {
            expression: "abilities",
            source: "live PKCalc calculator ability catalog",
          },
          {
            expression: `calc.ABILITIES[${generation}]`,
            source: "live PKCalc generation-specific calculator abilities",
          },
        ],
        items: [
          {
            expression: "ITEMS_BY_ID",
            source: "live PKCalc Dex item identifier catalog",
          },
          { expression: "items", source: "live PKCalc calculator item catalog" },
          {
            expression: `calc.ITEMS[${generation}]`,
            source: "live PKCalc generation-specific calculator items",
          },
        ],
        natures: [
          {
            expression: "NATURES_BY_ID",
            source: "live PKCalc Dex nature identifier catalog",
          },
          {
            expression: "calc.NATURES",
            source: "live PKCalc calculator nature catalog",
          },
        ],
        encounterSpecies: [
          {
            expression: "SPECIES_BY_ID",
            source: "live PKCalc Dex species identifier catalog for location species ids",
          },
          {
            expression: "species",
            source: "live PKCalc Dex species display catalog",
          },
        ],
      };

      const categories = Object.fromEntries(
        Object.entries(categoryCandidates).map(([category, candidates]) => [
          category,
          auditCategory(category, candidates),
        ])
      );
      const encounterRoute = auditLocationRendering();
      return {
        categories,
        encounterRoute,
        resolutionPolicy: {
          mode: "exhaustive",
          explanation:
            "Every generated reference from reference_report.json is checked against live PKCalc Dex catalogs using exact or normalized names. Encounter locations are additionally rendered through loadDexEntry for every routed location that has encounters.",
          categoryCandidates,
        },
      };
    },
    { contractInPage: contract, referencesInPage: references }
  );
}

function categoryFailures(categories) {
  const failures = [];
  for (const [category, result] of Object.entries(categories)) {
    if (result.status !== "checked") {
      const names = (result.unresolved || [])
        .map((ref) => ref.value || ref.normalized || JSON.stringify(ref))
        .slice(0, 20);
      failures.push(
        `live reference ${category} unresolved ${result.unresolvedCount} name(s): ${names.join(
          ", "
        )}`
      );
    }
  }
  return failures;
}

function writeJson(filePath, payload) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(payload, null, 2)}\n`);
}

function relpathOrAbs(filePath) {
  const rel = path.relative(REPO_ROOT, filePath);
  return rel.startsWith("..") || path.isAbsolute(rel) ? filePath : rel;
}

function finish(payload, reportPath) {
  writeJson(reportPath, payload);

  if (payload.status === "ok") {
    console.log(`Wrote ${relpathOrAbs(reportPath)}`);
    console.log(
      `Live reference audit passed: ${Object.entries(payload.categories)
        .map(([category, result]) => `${category}: ${result.count}`)
        .join(", ")}`
    );
    console.log(
      `Encounter route audit passed: ${payload.encounterRoute.locationsWithEncounters} locations, ${payload.encounterRoute.renderedRows} rows`
    );
    return;
  }

  console.error(`Wrote ${relpathOrAbs(reportPath)}`);
  console.error("Live reference audit failed:");
  for (const failure of payload.failures) {
    console.error(`- ${failure}`);
  }
  process.exit(1);
}

async function main() {
  const { adapterRoot, contractPath, outputDir, referenceReport, report, url } =
    parseArgs(process.argv);
  const contract = loadContract(contractPath);
  const referenceReportData = loadReferenceReport(referenceReport);
  const references = referencePayload(referenceReportData);
  const files = generatedFiles(outputDir, adapterRoot, contract);
  for (const filePath of Object.values(files)) {
    assertExists(filePath);
  }

  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({ serviceWorkers: "block" });
  const page = await context.newPage();
  const pageErrors = [];
  const consoleErrors = [];
  const failedRequests = [];
  const intercepted = Object.fromEntries(REQUEST_KEYS.map((key) => [key, 0]));
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

  for (const key of REQUEST_KEYS) {
    await page.route(requestPathPattern(contract.requestPaths[key]), (route) => {
      intercepted[key]++;
      route.fulfill({ path: files[key], contentType: "text/javascript" });
    });
  }

  let navigationError = "";
  try {
    await page.goto(url, {
      waitUntil: "load",
      timeout: contract.timeouts?.navigationMs || 90000,
    });
  } catch (error) {
    navigationError = error.message;
  }

  const preflight = await waitForContractPreflight(
    page,
    contract,
    intercepted,
    contract.timeouts?.contractReadyMs || 60000
  );

  let auditResult = {
    categories: {},
    encounterRoute: {
      status: "fail",
      mode: "exhaustive",
      failures: [],
      failureCount: 0,
    },
    resolutionPolicy: {
      mode: "exhaustive",
      explanation: "",
      categoryCandidates: {},
    },
  };
  let auditError = "";
  if (!navigationError && preflight.passed) {
    try {
      auditResult = await runLiveReferenceAudit(page, contract, references);
    } catch (error) {
      auditError = error.message;
    }
  }

  await browser.close();

  const missedInterceptions = Object.entries(intercepted)
    .filter(([, count]) => count === 0)
    .map(([name]) => name);
  const failures = [
    ...(navigationError ? [`navigation.load failed: ${navigationError}`] : []),
    ...preflight.failedAssumptions,
    ...(auditError ? [`live reference evaluation failed: ${auditError}`] : []),
    ...categoryFailures(auditResult.categories),
    ...(auditResult.encounterRoute.status === "checked"
      ? []
      : [
          `encounter route rendering failed ${auditResult.encounterRoute.failureCount || 0} location(s)`,
        ]),
    ...pageErrors.map((error) => `page error: ${error}`),
    ...consoleErrors.map((error) => `console error: ${error}`),
    ...failedRequests.map(
      (request) =>
        `failed request: ${request.type} ${request.url} ${request.error || ""}`.trim()
    ),
    ...missedInterceptions.map((name) => `missed interception: ${name}`),
  ];

  const payload = {
    schemaVersion: 1,
    status: failures.length ? "fail" : "ok",
    appUrl: url,
    adapterRoot: adapterRoot || path.join(outputDir, "pkcalc"),
    contractPath,
    referenceReportPath: referenceReport,
    generatedAt: new Date().toISOString().replace(/\.\d{3}Z$/, "Z"),
    resolutionPolicy: auditResult.resolutionPolicy,
    categories: auditResult.categories,
    encounterRoute: auditResult.encounterRoute,
    contractChecks: {
      requestPaths: preflight.requestPaths,
      constants: preflight.constants,
      appGlobals: preflight.appGlobals,
    },
    intercepted,
    missedInterceptions,
    preflight: {
      readyState: preflight.readyState,
      symbolTypes: preflight.symbolTypes,
      waitedMs: preflight.waitedMs,
    },
    pageErrors,
    consoleErrors,
    failedRequests,
    failures,
  };

  finish(payload, report);
}

main().catch((error) => {
  console.error(error.stack || error.message);
  process.exit(1);
});
