import { resolve } from "node:path";

// CI sets LOGOS_QT_MCP automatically; for interactive use:
//   nix build .#test-framework -o result-mcp
const root = process.env.LOGOS_QT_MCP || new URL("../result-mcp", import.meta.url).pathname;
const { test, run } = await import(resolve(root, "test-framework/framework.mjs"));

// Smoke tests: the view loads and the pages render. The backend now
// auto-starts the indexer from a seeded default config; with no bedrock
// running in the harness that start fails or errors, which is fine — these
// tests only assert the QML comes up and the backend is wired.

test("lez_explorer_ui: loads the view", async (app) => {
  await app.waitFor(
    async () => { await app.expectTexts(["LEZ Explorer"]); },
    { timeout: 30000, interval: 500, description: "the explorer view to load" }
  );
});

test("lez_explorer_ui: renders the home page", async (app) => {
  await app.waitFor(
    async () => { await app.expectTexts(["Recent Blocks", "Settings"]); },
    { timeout: 15000, interval: 500, description: "the home page to render" }
  );
});

test("lez_explorer_ui: settings shows the config form", async (app) => {
  await app.click("Settings");
  await app.waitFor(
    async () => { await app.expectTexts(["Bedrock Address", "Channel ID", "Save & Start"]); },
    { timeout: 15000, interval: 500, description: "the settings form to render" }
  );
});

run();
