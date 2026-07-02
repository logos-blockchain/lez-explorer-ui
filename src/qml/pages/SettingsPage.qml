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

    Component.onCompleted: {
        // Seed the editor with the saved config, falling back to the template.
        if (page.backend) {
            var current = page.backend.configText;
            editor.text = (current && current.length > 0) ? current : page.backend.defaultConfig;
        }
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
            text: "Edit the indexer configuration below and press Save. The config is "
                  + "persisted and the indexer is (re)started from it — no file paths needed. "
                  + "It is reloaded automatically the next time you open the explorer."
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
        }

        // JSON editor. The TextArea is anchored to fill its container directly
        // (NOT wrapped in a ScrollView — there it doesn't inherit the viewport
        // size and collapses to an invisible implicit size). It scrolls its
        // viewport to follow the cursor; that's enough for a config-sized doc.
        Rectangle {
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
            }
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

            // Reset to the built-in template.
            Rectangle {
                Layout.preferredHeight: 38
                Layout.preferredWidth: 150
                radius: Theme.spacing.radiusMedium
                color: resetHover.hovered ? Theme.palette.backgroundSecondary : Theme.palette.backgroundElevated
                border.width: 1
                border.color: Theme.palette.borderSecondary

                HoverHandler { id: resetHover }
                TapHandler { onTapped: if (page.backend) { editor.text = page.backend.defaultConfig; page.setStatus("", false); } }

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

    function save() {
        if (!page.backend)
            return;
        // Validate locally first for an immediate, precise error.
        try {
            JSON.parse(editor.text);
        } catch (e) {
            page.setStatus("Invalid JSON: " + e, true);
            return;
        }
        page.setStatus("Saving…", false);
        logos.watch(page.backend.applyConfigJson(editor.text),
            function (ok) {
                if (ok) {
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
