#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const REPO_ROOT = path.resolve(__dirname, "../..");
const DEFAULT_PKCALC_URL =
  process.env.PKCALC_URL || "https://pkcalc.anastarawneh.com/";

function parseArgs(argv) {
  let outputDir = path.join(REPO_ROOT, "build/tracker_export");
  let adapterRoot = "";
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
    } else if (arg === "--url") {
      i++;
      if (i >= argv.length) {
        throw new Error("--url requires a value");
      }
      url = argv[i];
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: site_smoke_tracker_export_playwright.cjs [--output-dir DIR] [--adapter-root DIR] [--url URL]"
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

  return { adapterRoot, outputDir, url };
}

function assertExists(filePath) {
  if (!fs.existsSync(filePath)) {
    throw new Error(`Missing generated tracker artifact: ${filePath}`);
  }
}

function generatedFiles(outputDir, adapterRoot) {
  if (adapterRoot) {
    return {
      party: path.join(adapterRoot, "js/data/party_order.js"),
      sets: path.join(adapterRoot, "js/data/sets.js"),
      locations: path.join(adapterRoot, "js/data/dex/locations.js"),
    };
  }

  const pkcalcDir = path.join(outputDir, "pkcalc");
  return {
    party: path.join(pkcalcDir, "party_order.js"),
    sets: path.join(pkcalcDir, "sets.js"),
    locations: path.join(pkcalcDir, "locations.js"),
  };
}

async function main() {
  const { adapterRoot, outputDir, url } = parseArgs(process.argv);
  const files = generatedFiles(outputDir, adapterRoot);
  for (const filePath of Object.values(files)) {
    assertExists(filePath);
  }

  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({ serviceWorkers: "block" });
  const page = await context.newPage();
  const pageErrors = [];
  const consoleErrors = [];
  const failedRequests = [];
  const intercepted = { party: 0, sets: 0, locations: 0 };
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

  await page.route(/\/js\/data\/party_order\.js\??.*$/, (route) => {
    intercepted.party++;
    route.fulfill({ path: files.party, contentType: "text/javascript" });
  });
  await page.route(/\/js\/data\/sets\.js\??.*$/, (route) => {
    intercepted.sets++;
    route.fulfill({ path: files.sets, contentType: "text/javascript" });
  });
  await page.route(/\/js\/data\/dex\/locations\.js\??.*$/, (route) => {
    intercepted.locations++;
    route.fulfill({ path: files.locations, contentType: "text/javascript" });
  });

  await page.goto(url, { waitUntil: "load", timeout: 90000 });
  await page.waitForFunction(
    () =>
      typeof getSetOptions === "function" &&
      typeof loadDexEntry === "function" &&
      typeof SETDEX_PK === "object" &&
      typeof PARTY_ORDER_PK === "object" &&
      typeof LOCATIONS === "object",
    { timeout: 60000 }
  );

  const result = await page.evaluate(() => {
    $(".gen").val("4").trigger("change");

    const sawyerLabel = "Hiker Sawyer [TRAINER_SAWYER_1]";
    const options = getSetOptions();
    const sawyerOption = options.find(
      (option) => option.id === `Geodude (${sawyerLabel})`
    );

    loadDexEntry("location/route101");
    const locationName =
      document.querySelector(".dex-info.location .name")?.textContent || "";
    const renderedRows = [
      ...document.querySelectorAll(".dex-info.location .location-species"),
    ].map((element) => ({
      name: element.querySelector(".name")?.textContent || "",
      level: element.querySelector(".level")?.textContent || "",
      method: element.querySelector(".method")?.getAttribute("title") || "",
    }));

    const checks = {
      pkcalcSetdexPath: SETDEX[4] === SETDEX_PK,
      pkcalcPartyOrderPath: partyOrder === PARTY_ORDER_PK,
      pkcalcGetSetOptionsSawyer:
        !!sawyerOption &&
        sawyerOption.pokemon === "Geodude" &&
        sawyerOption.set === sawyerLabel,
      pkcalcLocationsPath:
        LOCATIONS.route101?.coords?.some(([x, y]) => x === 4 && y === 10) ===
        true,
      pkcalcLoadDexEntryRoute101:
        locationName === "Route 101" &&
        renderedRows.length === 12 &&
        renderedRows.some((row) => row.name === "Wurmple") &&
        renderedRows.some((row) => row.name === "Poochyena"),
    };

    return {
      checks,
      counts: {
        setOptions: options.length,
        renderedRows: renderedRows.length,
      },
      locationName,
      sawyerOption,
      renderedRows: renderedRows.slice(0, 3),
    };
  });

  await browser.close();

  const failedChecks = Object.entries(result.checks)
    .filter(([, passed]) => !passed)
    .map(([name]) => name);
  const missedInterceptions = Object.entries(intercepted)
    .filter(([, count]) => count === 0)
    .map(([name]) => name);
  const status =
    pageErrors.length ||
    consoleErrors.length ||
    failedRequests.length ||
    failedChecks.length ||
    missedInterceptions.length
      ? "fail"
      : "ok";

  const payload = {
    status,
    appUrl: url,
    adapterRoot: adapterRoot || path.join(outputDir, "pkcalc"),
    intercepted,
    pageErrors,
    consoleErrors,
    failedRequests,
    failedChecks,
    missedInterceptions,
    ...result,
  };

  const output = JSON.stringify(payload, null, 2);
  if (status === "ok") {
    console.log(output);
  } else {
    console.error(output);
    process.exit(1);
  }
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
