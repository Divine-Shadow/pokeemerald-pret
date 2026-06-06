#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const REPO_ROOT = path.resolve(__dirname, "../..");

function parseArgs(argv) {
  let outputDir = path.join(REPO_ROOT, "build/tracker_export");

  for (let i = 2; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--output-dir") {
      i++;
      if (i >= argv.length) {
        throw new Error("--output-dir requires a value");
      }
      outputDir = argv[i];
    } else if (arg === "--help" || arg === "-h") {
      console.log("Usage: smoke_tracker_export_playwright.cjs [--output-dir DIR]");
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }

  if (!path.isAbsolute(outputDir)) {
    outputDir = path.join(REPO_ROOT, outputDir);
  }

  return { outputDir };
}

function assertExists(filePath) {
  if (!fs.existsSync(filePath)) {
    throw new Error(`Missing generated tracker artifact: ${filePath}`);
  }
}

async function main() {
  const { outputDir } = parseArgs(process.argv);
  const pkcalcDir = path.join(outputDir, "pkcalc");

  const adapterFiles = ["sets.js", "party_order.js", "locations.js"];
  for (const fileName of adapterFiles) {
    assertExists(path.join(pkcalcDir, fileName));
  }

  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1024, height: 768 } });
  const pageErrors = [];
  const consoleErrors = [];

  page.on("pageerror", (error) => pageErrors.push(error.message));
  page.on("console", (message) => {
    if (message.type() === "error") {
      consoleErrors.push(message.text());
    }
  });

  await page.setContent(
    "<!doctype html><html><head><title>Tracker Export Smoke</title></head><body><main id=\"result\">loading</main></body></html>"
  );

  for (const fileName of adapterFiles) {
    await page.addScriptTag({ path: path.join(pkcalcDir, fileName) });
  }

  const result = await page.evaluate(() => {
    const setdex = typeof SETDEX_PK !== "undefined" ? SETDEX_PK : {};
    const partyOrder =
      typeof PARTY_ORDER_PK !== "undefined" ? PARTY_ORDER_PK : {};
    const locations = typeof LOCATIONS !== "undefined" ? LOCATIONS : {};
    const sawyerLabel = "Hiker Sawyer [TRAINER_SAWYER_1]";
    const geodude = setdex.Geodude?.[sawyerLabel];
    const route101 = locations.route101;
    const grass = route101?.encounters?.filter((enc) => enc.method === "grass") || [];

    const checks = {
      setdexLoaded: typeof SETDEX_PK === "object" && Object.keys(setdex).length > 200,
      partyOrderLoaded:
        typeof PARTY_ORDER_PK === "object" && Object.keys(partyOrder).length > 800,
      locationsLoaded:
        typeof LOCATIONS === "object" && Object.keys(locations).length > 200,
      sawyerGeodude:
        geodude?.level === 21 &&
        geodude?.item === "Berry Juice" &&
        geodude?.ability === "Sturdy" &&
        geodude?.nature === "Adamant" &&
        geodude?.moves?.includes("Stealth Rock") &&
        geodude?.moves?.includes("Rock Blast") &&
        geodude?.moves?.includes("Earthquake") &&
        geodude?.moves?.includes("Sucker Punch"),
      sawyerParty: partyOrder[sawyerLabel]?.[0] === "Geodude",
      route101Coord:
        route101?.coords?.some(([x, y]) => x === 4 && y === 10) === true,
      route101Grass:
        grass.length === 12 &&
        grass[0]?.species === "wurmple" &&
        grass.some((enc) => enc.species === "poochyena"),
    };

    const payload = {
      checks,
      counts: {
        setdexSpecies: Object.keys(setdex).length,
        partyOrder: Object.keys(partyOrder).length,
        locations: Object.keys(locations).length,
        route101Encounters: route101?.encounters?.length || 0,
      },
    };

    document.body.dataset.status = Object.values(checks).every(Boolean)
      ? "ok"
      : "fail";
    document.getElementById("result").textContent = JSON.stringify(checks);
    return payload;
  });

  const status = await page.locator("body").getAttribute("data-status");
  const renderedText = await page.locator("#result").innerText();
  await browser.close();

  const failed = Object.entries(result.checks)
    .filter(([, passed]) => !passed)
    .map(([name]) => name);

  if (pageErrors.length || consoleErrors.length || status !== "ok" || failed.length) {
    console.error(
      JSON.stringify(
        { status, pageErrors, consoleErrors, failed, ...result, renderedText },
        null,
        2
      )
    );
    process.exit(1);
  }

  console.log(JSON.stringify({ status, ...result, renderedText }, null, 2));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
