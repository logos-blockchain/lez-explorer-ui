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
            portField.text = "8779";
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

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.medium

            LogosText {
                text: "RPC port"
                color: Theme.palette.textSecondary
                font.pixelSize: Theme.typography.secondaryText
                Layout.alignment: Qt.AlignVCenter
            }
            Rectangle {
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                radius: Theme.spacing.radiusMedium
                color: Theme.palette.backgroundSecondary
                border.width: 1
                border.color: portField.activeFocus ? Theme.palette.borderInteractive
                                                     : Theme.palette.borderSecondary
                TextField {
                    id: portField
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacing.medium
                    anchors.rightMargin: Theme.spacing.medium
                    verticalAlignment: TextInput.AlignVCenter
                    text: "8779"
                    inputMethodHints: Qt.ImhDigitsOnly
                    color: Theme.palette.text
                    font.pixelSize: Theme.typography.secondaryText
                    background: Item {}
                }
            }
            Item { Layout.fillWidth: true }
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
        logos.watch(page.backend.applyConfigJson(editor.text, portField.text),
            function (ok) {
                if (ok) {
                    page.setStatus("Saved. Indexer starting…", false);
                    page.explorer.goHome();
                } else {
                    page.setStatus("Save failed — check the config and try again.", true);
                }
            },
            function (err) {
                page.setStatus("Error: " + err, true);
            });
    }
}
