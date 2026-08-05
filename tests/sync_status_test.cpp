// Unit tests for parseSyncStatus against the indexer FFI's status JSON.
//
// Fixtures mirror `IndexerStatus` in logos-execution-zone (lez/indexer/core/
// src/status.rs) as of PR #581: PascalCase `state` variants and snake_case
// fields (`last_error`, `indexed_block_id`, `stall_reason`). State names are
// passed through verbatim; only the UI-invented "Stopped" is synthesized.
//
// Build & run (see Justfile `test-backend`): needs only Qt6Core.

#include <cstdlib>
#include <iostream>

#include "sync_status.h"

namespace {
    int failures = 0;

    void check(bool ok, const std::string& what) {
        if (!ok) {
            ++failures;
            std::cerr << "FAIL: " << what << "\n";
        }
    }

    void checkEq(const QString& got, const QString& want, const std::string& what) {
        check(got == want, what + " (got \"" + got.toStdString() + "\", want \"" + want.toStdString() + "\")");
    }
} // namespace

int main() {
    // Empty payload = indexer not running.
    {
        const SyncStatus s = parseSyncStatus(QString());
        checkEq(s.state, "Stopped", "empty json maps to Stopped");
        check(s.blockId == 0, "empty json has no block id");
    }

    // Malformed payload degrades to stopped rather than crashing the banner.
    {
        const SyncStatus s = parseSyncStatus(QStringLiteral("not json"));
        checkEq(s.state, "Stopped", "malformed json maps to Stopped");
    }

    // The steady state: caught up with an indexed tip.
    {
        const SyncStatus s = parseSyncStatus(
            QStringLiteral(R"({"state":"CaughtUp","last_error":null,"indexed_block_id":42,"stall_reason":null})")
        );
        checkEq(s.state, "CaughtUp", "CaughtUp passes through");
        check(s.blockId == 42, "indexed_block_id is read");
        checkEq(s.error, "", "null last_error is empty");
    }

    // Boot / catch-up phases.
    {
        const SyncStatus s = parseSyncStatus(
            QStringLiteral(R"({"state":"Starting","last_error":null,"indexed_block_id":null,"stall_reason":null})")
        );
        checkEq(s.state, "Starting", "Starting passes through");
        check(s.blockId == 0, "null indexed_block_id is 0");
    }
    {
        const SyncStatus s = parseSyncStatus(
            QStringLiteral(R"({"state":"Syncing","last_error":null,"indexed_block_id":7,"stall_reason":null})")
        );
        checkEq(s.state, "Syncing", "Syncing passes through");
        check(s.blockId == 7, "syncing carries the tip");
    }

    // Failure carries the reason.
    {
        const SyncStatus s = parseSyncStatus(QStringLiteral(
            R"({"state":"Error","last_error":"L1 node unreachable","indexed_block_id":9,"stall_reason":null})"
        ));
        checkEq(s.state, "Error", "Error passes through");
        checkEq(s.error, "L1 node unreachable", "last_error is read");
    }

    // New in PR #581: parked on a bad block, tip frozen.
    {
        const SyncStatus s = parseSyncStatus(QStringLiteral(
            R"({"state":"Stalled","last_error":"block 10 does not chain","indexed_block_id":9,)"
            R"("stall_reason":{"orphans_since":3}})"
        ));
        checkEq(s.state, "Stalled", "Stalled passes through");
        checkEq(s.error, "block 10 does not chain", "stall carries last_error");
        check(s.blockId == 9, "stalled keeps the frozen tip");
    }

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "all checks passed\n";
    return EXIT_SUCCESS;
}
