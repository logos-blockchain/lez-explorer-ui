import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme
import "../components"

Item {
    id: page

    property var explorer
    readonly property var backend: explorer ? explorer.backend : null
    property bool statusIsError: false
    // Armed by the first "Delete Cache" tap; a second tap actually wipes.
    property bool confirmingReset: false
    // True once the user edits the raw JSON; their text then becomes the base
    // document that Save merges the form fields into.
    property bool jsonDirty: false
    property bool advancedOpen: false
    property bool seedingEditor: false

    Component.onCompleted: {
        if (!page.backend)
            return;
        var base = (page.backend.configText && page.backend.configText.length > 0)
                 ? page.backend.configText : page.backend.defaultConfig;
        page.populateFrom(base);
        page.seedEditor(base);
    }

    // Surface backend-side failures (write/start) in the status line.
    Connections {
        target: page.backend
        ignoreUnknownSignals: true
        function onError(message) {
            page.setStatus(message, true);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing.medium

        SectionHeader { title: "Indexer Settings" }

        LogosText {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "The indexer starts automatically with these settings. Change a field "
                  + "and press Save to persist the config and restart the indexer."
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
        }

        FormField {
            id: addrField
            Layout.fillWidth: true
            label: "Bedrock Address"
            placeholder: "http://localhost:8080"
            errorText: text.trim() === "" || /^https?:\/\/.+/.test(text.trim())
                       ? "" : "Must start with http:// or https://"
        }

        FormField {
            id: channelField
            Layout.fillWidth: true
            label: "Channel ID"
            mono: true
            placeholder: "64 hex characters"
            errorText: text.trim() === "" || /^[0-9a-fA-F]{64}$/.test(text.trim())
                       ? "" : "Must be exactly 64 hex characters"
        }

        FormField {
            id: intervalField
            Layout.fillWidth: true
            label: "Polling Interval"
            placeholder: "1s"
            errorText: text.trim() === "" || /^\d+(ms|s|m|h)$/.test(text.trim())
                       ? "" : "Must look like 500ms, 1s, 2m or 1h"
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            LogosCheckbox {
                id: chainResetToggle
                text: "Allow chain reset"
            }
            LogosText {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "Wipe the local index and re-sync when the channel serves a different chain."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
        }

        // Advanced ▸ raw JSON — escape hatch for auth / cross_zone /
        // bridge_lock_holdings. Collapsed by default.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.small

            LogosText {
                text: (page.advancedOpen ? "▾ " : "▸ ") + "Advanced: raw config JSON"
                color: Theme.palette.textSecondary
                font.pixelSize: Theme.typography.secondaryText
            }
            TapHandler { onTapped: page.advancedOpen = !page.advancedOpen }
        }

        Rectangle {
            visible: page.advancedOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.spacing.radiusLarge
            color: Theme.palette.backgroundElevated
            border.width: 1
            border.color: editor.activeFocus ? Theme.palette.borderInteractive
                                             : Theme.palette.borderSecondary
            clip: true

            TextArea {
                id: editor
                anchors.fill: parent
                anchors.margins: Theme.spacing.medium
                font.family: "Menlo, Monaco, Courier New, monospace"
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.text
                wrapMode: TextArea.Wrap
                background: Item {}
                onTextChanged: if (!page.seedingEditor) page.jsonDirty = true
            }
        }

        // Filler so the buttons stay at the bottom while Advanced is collapsed.
        Item {
            visible: !page.advancedOpen
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.medium

            LogosText {
                id: statusLabel
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: page.statusIsError ? Theme.palette.error : Theme.palette.success
                font.pixelSize: Theme.typography.secondaryText
            }

            // Delete the indexer's RocksDB cache and re-sync (two-tap confirm).
            Rectangle {
                Layout.preferredHeight: 38
                Layout.preferredWidth: 150
                radius: Theme.spacing.radiusMedium
                color: resetCacheHover.hovered ? Qt.lighter(Theme.palette.error, 1.1) : Theme.palette.error

                HoverHandler { id: resetCacheHover }
                TapHandler { onTapped: page.resetCache() }

                LogosText {
                    anchors.centerIn: parent
                    text: page.confirmingReset ? "Confirm delete" : "Delete Cache"
                    color: Theme.palette.background
                    font.pixelSize: Theme.typography.secondaryText
                    font.weight: Theme.typography.weightBold
                }
            }

            // Reset form + JSON to the built-in template.
            Rectangle {
                Layout.preferredHeight: 38
                Layout.preferredWidth: 150
                radius: Theme.spacing.radiusMedium
                color: resetHover.hovered ? Theme.palette.backgroundSecondary : Theme.palette.backgroundElevated
                border.width: 1
                border.color: Theme.palette.borderSecondary

                HoverHandler { id: resetHover }
                TapHandler { onTapped: page.resetToDefault() }

                LogosText {
                    anchors.centerIn: parent
                    text: "Reset to default"
                    color: Theme.palette.textSecondary
                    font.pixelSize: Theme.typography.secondaryText
                }
            }

            // Save + (re)start.
            Rectangle {
                Layout.preferredHeight: 38
                Layout.preferredWidth: 150
                radius: Theme.spacing.radiusMedium
                color: saveHover.hovered ? Qt.lighter(Theme.palette.primary, 1.1) : Theme.palette.primary

                HoverHandler { id: saveHover }
                TapHandler { onTapped: page.save() }

                LogosText {
                    anchors.centerIn: parent
                    text: "Save & Start"
                    color: Theme.palette.background
                    font.pixelSize: Theme.typography.secondaryText
                    font.weight: Theme.typography.weightBold
                }
            }
        }
    }

    function setStatus(message, isError) {
        page.statusIsError = isError;
        statusLabel.text = message;
    }

    // Set editor text without tripping the user-edit dirty flag.
    function seedEditor(text) {
        page.seedingEditor = true;
        editor.text = text;
        page.seedingEditor = false;
        page.jsonDirty = false;
    }

    // Fill the form fields from a config JSON string (missing keys => empty).
    function populateFrom(json) {
        var obj = {};
        try { obj = JSON.parse(json); } catch (e) { obj = {}; }
        addrField.text = (obj.bedrock_config && obj.bedrock_config.addr) ? obj.bedrock_config.addr : "";
        channelField.text = obj.channel_id || "";
        intervalField.text = obj.consensus_info_polling_interval || "1s";
        chainResetToggle.checked = obj.allow_chain_reset === true;
    }

    // "" when all form fields are valid, else the first problem.
    function validateFields() {
        if (!/^https?:\/\/.+/.test(addrField.text.trim()))
            return "Bedrock address must start with http:// or https://.";
        if (!/^[0-9a-fA-F]{64}$/.test(channelField.text.trim()))
            return "Channel ID must be exactly 64 hex characters.";
        if (!/^\d+(ms|s|m|h)$/.test(intervalField.text.trim()))
            return "Polling interval must look like 500ms, 1s, 2m or 1h.";
        return "";
    }

    function resetToDefault() {
        if (!page.backend)
            return;
        page.confirmingReset = false;
        page.populateFrom(page.backend.defaultConfig);
        page.seedEditor(page.backend.defaultConfig);
        page.setStatus("", false);
    }

    // Delete the indexer's RocksDB cache and re-sync. First tap arms; a second
    // tap confirms (it's destructive — wipes the local index).
    function resetCache() {
        if (!page.backend)
            return;
        // Nothing to reset to until a config has been saved; don't arm the
        // destructive confirm — point the user at Save first.
        if (!page.backend.configText || page.backend.configText.length === 0) {
            page.confirmingReset = false;
            page.setStatus("Save an indexer config first, then you can delete the cache.", true);
            return;
        }
        if (!page.confirmingReset) {
            page.confirmingReset = true;
            page.setStatus("Click “Confirm delete” to wipe the local index and re-sync from scratch.", true);
            return;
        }
        page.confirmingReset = false;
        page.setStatus("Deleting cache…", false);
        logos.watch(page.backend.resetIndexerCache(),
            function (ok) {
                if (ok) {
                    page.setStatus("Cache deleted. Indexer restarting…", false);
                    page.explorer.goHome();
                }
                // On failure the backend emits error() (shown by onError above).
            },
            function (err) {
                page.setStatus("Error: " + err, true);
            });
    }

    // Merge the form-managed keys into the base JSON (the user-edited raw
    // JSON if dirty, else the saved config) and save+restart. Unknown keys
    // (auth, cross_zone, bridge_lock_holdings, ...) are preserved.
    function save() {
        if (!page.backend)
            return;
        // A save cancels a pending cache-delete confirmation.
        page.confirmingReset = false;

        var problem = page.validateFields();
        if (problem !== "") {
            page.setStatus(problem, true);
            return;
        }

        var base = page.jsonDirty ? editor.text
                 : ((page.backend.configText && page.backend.configText.length > 0)
                    ? page.backend.configText : page.backend.defaultConfig);
        var obj;
        try {
            obj = JSON.parse(base);
        } catch (e) {
            page.setStatus("Invalid JSON in the advanced editor: " + e, true);
            return;
        }
        if (typeof obj !== "object" || obj === null || Array.isArray(obj)) {
            page.setStatus("Config must be a JSON object.", true);
            return;
        }

        if (typeof obj.bedrock_config !== "object" || obj.bedrock_config === null || Array.isArray(obj.bedrock_config))
            obj.bedrock_config = {};
        obj.bedrock_config.addr = addrField.text.trim();
        obj.channel_id = channelField.text.trim().toLowerCase();
        obj.allow_chain_reset = chainResetToggle.checked;
        obj.consensus_info_polling_interval = intervalField.text.trim();

        var out = JSON.stringify(obj, null, 4);

        // Edits that cancel out (or a Save with nothing touched) shouldn't
        // bounce the indexer — compare semantically so formatting differences
        // with the stored JSON don't force a restart.
        var current = page.backend.configText;
        if (current && current.length > 0) {
            try {
                if (JSON.stringify(JSON.parse(current)) === JSON.stringify(obj)) {
                    page.seedEditor(out);
                    page.setStatus("", false);
                    page.explorer.goHome();
                    return;
                }
            } catch (e) {
                // Unparseable stored config — treat as changed and save normally.
            }
        }

        page.setStatus("Saving…", false);
        logos.watch(page.backend.applyConfigJson(out),
            function (ok) {
                if (ok) {
                    page.seedEditor(out);
                    page.setStatus("Saved. Indexer starting…", false);
                    page.explorer.goHome();
                }
                // On failure the backend emits error() with a specific reason
                // (shown by onError above) — don't clobber it with a generic line.
            },
            function (err) {
                page.setStatus("Error: " + err, true);
            });
    }
}
