import { resolve } from "node:path";

// CI sets LOGOS_QT_MCP automatically; for interactive use:
//   nix build .#test-framework -o result-mcp
const root = process.env.LOGOS_QT_MCP || new URL("../result-mcp", import.meta.url).pathname;
const { test, run } = await import(resolve(root, "test-framework/framework.mjs"));

// Smoke tests: the view loads and the home page renders. They do NOT assert a
// live indexer connection — with no indexer ingesting, the backend stays in
// "Connecting" (chain tip is 0), which is the correct empty state. They only
// verify the QML view comes up and the backend is wired (readiness overlay
// clears, revealing the navigation bar + home page).

test("lez_explorer_ui: loads the view", async (app) => {
  await app.waitFor(
    async () => { await app.expectTexts(["LEZ Explorer"]); },
    { timeout: 30000, interval: 500, description: "the explorer view to load" }
  );
});

test("lez_explorer_ui: renders the home page", async (app) => {
  await app.waitFor(
    async () => { await app.expectTexts(["Recent Blocks", "Indexer config"]); },
    { timeout: 15000, interval: 500, description: "the home page to render" }
  );
});

run();
