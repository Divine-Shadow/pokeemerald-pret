#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const REPO_ROOT = path.resolve(__dirname, "../..");
const DEFAULT_CONTRACT_PATH = path.join(
  __dirname,
  "pkcalc_compat_contract.json"
);
const DEFAULT_PKCALC_URL =
  process.env.PKCALC_URL || "https://pkcalc.anastarawneh.com/";
const REQUEST_KEYS = ["party", "sets", "locations"];
const POLL_MS = 500;

function parseArgs(argv) {
  let outputDir = path.join(REPO_ROOT, "build/tracker_export");
  let adapterRoot = "";
  let contractPath = DEFAULT_CONTRACT_PATH;
  let url = DEFAULT_PKCALC_URL;

  for (let i = 2; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--output-dir") {
      i++;
      if (i >= argv.length) {
        throw new Error("--output-dir requires a value");
      }
      outputDir = argv[i];
    } else if (arg === "--adapter-root") {
      i++;
      if (i >= argv.length) {
        throw new Error("--adapter-root requires a value");
      }
      adapterRoot = argv[i];
    } else if (arg === "--contract") {
      i++;
      if (i >= argv.length) {
        throw new Error("--contract requires a value");
      }
      contractPath = argv[i];
    } else if (arg === "--url") {
      i++;
      if (i >= argv.length) {
        throw new Error("--url requires a value");
      }
      url = argv[i];
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: site_smoke_tracker_export_playwright.cjs [--output-dir DIR] [--adapter-root DIR] [--contract FILE] [--url URL]"
      );
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }

  if (!path.isAbsolute(outputDir)) {
    outputDir = path.join(REPO_ROOT, outputDir);
  }
  if (adapterRoot && !path.isAbsolute(adapterRoot)) {
    adapterRoot = path.join(REPO_ROOT, adapterRoot);
  }
  if (!path.isAbsolute(contractPath)) {
    contractPath = path.join(REPO_ROOT, contractPath);
  }

  return { adapterRoot, contractPath, outputDir, url };
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
  if (requireObject(errors, contract.smokeInputs, "smokeInputs")) {
    const smoke = contract.smokeInputs;
    for (const name of [
      "generationSelector",
      "generationValue",
      "trainerLabel",
      "setOptionId",
      "setSpecies",
      "locationEntry",
      "locationId",
      "locationName",
    ]) {
      requireString(errors, smoke[name], `smokeInputs.${name}`);
    }
    if (
      !Array.isArray(smoke.locationCoord) ||
      smoke.locationCoord.length !== 2 ||
      !smoke.locationCoord.every(Number.isInteger)
    ) {
      errors.push("smokeInputs.locationCoord must be a two-integer array");
    }
    if (!Number.isInteger(smoke.expectedRenderedRows)) {
      errors.push("smokeInputs.expectedRenderedRows must be an integer");
    }
    if (
      !Array.isArray(smoke.expectedRenderedSpecies) ||
      !smoke.expectedRenderedSpecies.every((value) => typeof value === "string")
    ) {
      errors.push("smokeInputs.expectedRenderedSpecies must be a string array");
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

function collectNestedFailures(groups) {
  const failures = [];
  for (const [prefix, checks] of Object.entries(groups || {})) {
    failures.push(...collectSectionFailures(prefix, checks));
  }
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

async function runSmokeAssertions(page, contract) {
  return page.evaluate((contractInPage) => {
    const smoke = contractInPage.smokeInputs;
    $(smoke.generationSelector).val(smoke.generationValue).trigger("change");

    const options = getSetOptions();
    const sawyerOption = options.find(
      (option) => option.id === smoke.setOptionId
    );

    loadDexEntry(smoke.locationEntry);
    const locationName =
      document.querySelector(".dex-info.location .name")?.textContent || "";
    const renderedRows = [
      ...document.querySelectorAll(".dex-info.location .location-species"),
    ].map((element) => ({
      name: element.querySelector(".name")?.textContent || "",
      level: element.querySelector(".level")?.textContent || "",
      method: element.querySelector(".method")?.getAttribute("title") || "",
    }));

    const [coordX, coordY] = smoke.locationCoord;
    const speciesPresent = Object.fromEntries(
      smoke.expectedRenderedSpecies.map((species) => [
        species,
        renderedRows.some((row) => row.name === species),
      ])
    );
    const allSpeciesPresent = Object.values(speciesPresent).every(Boolean);
    const partyOrderType = typeof partyOrder;
    const setdexGeneration = contractInPage.dataRoutes.setdexGenerationIndex;

    const dataRoutes = {
      setdexGeneration: {
        expression: `SETDEX[${setdexGeneration}] === SETDEX_PK`,
        expected: true,
        actual: SETDEX?.[setdexGeneration] === SETDEX_PK,
        passed: SETDEX?.[setdexGeneration] === SETDEX_PK,
      },
      partyOrder: {
        expression: "partyOrder === PARTY_ORDER_PK",
        expected: true,
        actual: partyOrderType !== "undefined" && partyOrder === PARTY_ORDER_PK,
        actualType: partyOrderType,
        passed: partyOrderType !== "undefined" && partyOrder === PARTY_ORDER_PK,
      },
    };

    const uiSmoke = {
      getSetOptionsSawyer: {
        expected: {
          id: smoke.setOptionId,
          pokemon: smoke.setSpecies,
          set: smoke.trainerLabel,
        },
        actual: sawyerOption || null,
        passed:
          !!sawyerOption &&
          sawyerOption.pokemon === smoke.setSpecies &&
          sawyerOption.set === smoke.trainerLabel,
      },
      locationsCoord: {
        expected: {
          locationId: smoke.locationId,
          coord: smoke.locationCoord,
        },
        actual: LOCATIONS[smoke.locationId]?.coords || null,
        passed:
          LOCATIONS[smoke.locationId]?.coords?.some(
            ([x, y]) => x === coordX && y === coordY
          ) === true,
      },
      loadDexEntryLocation: {
        expected: {
          locationName: smoke.locationName,
          renderedRows: smoke.expectedRenderedRows,
          species: smoke.expectedRenderedSpecies,
        },
        actual: {
          locationName,
          renderedRows: renderedRows.length,
          speciesPresent,
        },
        passed:
          locationName === smoke.locationName &&
          renderedRows.length === smoke.expectedRenderedRows &&
          allSpeciesPresent,
      },
    };

    return {
      contractChecks: {
        dataRoutes,
        uiSmoke,
      },
      checks: {
        pkcalcSetdexPath: dataRoutes.setdexGeneration.passed,
        pkcalcPartyOrderPath: dataRoutes.partyOrder.passed,
        pkcalcGetSetOptionsSawyer: uiSmoke.getSetOptionsSawyer.passed,
        pkcalcLocationsPath: uiSmoke.locationsCoord.passed,
        pkcalcLoadDexEntryRoute101: uiSmoke.loadDexEntryLocation.passed,
      },
      counts: {
        setOptions: options.length,
        renderedRows: renderedRows.length,
      },
      locationName,
      sawyerOption,
      renderedRows: renderedRows.slice(0, 3),
    };
  }, contract);
}

function emitPayload(payload) {
  const output = JSON.stringify(payload, null, 2);
  if (payload.status === "ok") {
    console.log(output);
    return;
  }
  console.error(output);
  process.exit(1);
}

async function main() {
  const { adapterRoot, contractPath, outputDir, url } = parseArgs(process.argv);
  const contract = loadContract(contractPath);
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

  if (navigationError || !preflight.passed) {
    await browser.close();
    const failedAssumptions = [
      ...(navigationError ? [`navigation.load failed: ${navigationError}`] : []),
      ...preflight.failedAssumptions,
    ];
    emitPayload({
      status: "fail",
      appUrl: url,
      adapterRoot: adapterRoot || path.join(outputDir, "pkcalc"),
      contractPath,
      contractChecks: {
        requestPaths: preflight.requestPaths,
        constants: preflight.constants,
        appGlobals: preflight.appGlobals,
      },
      failedAssumptions,
      intercepted,
      pageErrors,
      consoleErrors,
      failedRequests,
      missedInterceptions: Object.entries(intercepted)
        .filter(([, count]) => count === 0)
        .map(([name]) => name),
      preflight: {
        readyState: preflight.readyState,
        symbolTypes: preflight.symbolTypes,
        waitedMs: preflight.waitedMs,
      },
    });
  }

  let result;
  let assertionError = "";
  try {
    result = await runSmokeAssertions(page, contract);
  } catch (error) {
    assertionError = error.message;
    result = {
      contractChecks: { dataRoutes: {}, uiSmoke: {} },
      checks: {},
      counts: {},
      locationName: "",
      sawyerOption: null,
      renderedRows: [],
    };
  }

  await browser.close();

  const failedChecks = Object.entries(result.checks)
    .filter(([, passed]) => !passed)
    .map(([name]) => name);
  const missedInterceptions = Object.entries(intercepted)
    .filter(([, count]) => count === 0)
    .map(([name]) => name);
  const failedAssumptions = [
    ...(assertionError ? [`smoke assertions failed: ${assertionError}`] : []),
    ...collectNestedFailures(result.contractChecks),
  ];
  const status =
    pageErrors.length ||
    consoleErrors.length ||
    failedRequests.length ||
    failedChecks.length ||
    missedInterceptions.length ||
    failedAssumptions.length
      ? "fail"
      : "ok";

  emitPayload({
    status,
    appUrl: url,
    adapterRoot: adapterRoot || path.join(outputDir, "pkcalc"),
    contractPath,
    failedAssumptions,
    intercepted,
    pageErrors,
    consoleErrors,
    failedRequests,
    failedChecks,
    missedInterceptions,
    preflight: {
      readyState: preflight.readyState,
      symbolTypes: preflight.symbolTypes,
      waitedMs: preflight.waitedMs,
    },
    ...result,
    contractChecks: {
      requestPaths: preflight.requestPaths,
      constants: preflight.constants,
      appGlobals: preflight.appGlobals,
      ...result.contractChecks,
    },
  });
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
