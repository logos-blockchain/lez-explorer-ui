import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme
import "../components"

Item {
    id: page

    // Injected by Main: the controller (navigation + backend access).
    property var explorer
    readonly property var backend: explorer ? explorer.backend : null

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing.medium

        // Indexer config + health.
        Card {
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing.medium

                LogosText {
                    text: "Indexer config"
                    color: Theme.palette.textSecondary
                    font.pixelSize: Theme.typography.secondaryText
                    Layout.alignment: Qt.AlignVCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: Theme.spacing.radiusMedium
                    color: Theme.palette.backgroundSecondary
                    border.width: 1
                    border.color: configField.activeFocus ? Theme.palette.borderInteractive
                                                           : Theme.palette.borderSecondary

                    TextField {
                        id: configField
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing.medium
                        anchors.rightMargin: Theme.spacing.medium
                        verticalAlignment: TextInput.AlignVCenter
                        text: page.backend ? page.backend.indexerConfig : ""
                        placeholderText: "absolute path to indexer_config.json (blank = already running)"
                        color: Theme.palette.text
                        placeholderTextColor: Theme.palette.textMuted
                        font.pixelSize: Theme.typography.secondaryText
                        background: Item {}
                        onAccepted: page.applyConfig()
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: 84
                    radius: Theme.spacing.radiusMedium
                    color: applyHover.hovered ? Qt.lighter(Theme.palette.primary, 1.1) : Theme.palette.primary

                    HoverHandler { id: applyHover }
                    TapHandler { onTapped: page.applyConfig() }

                    LogosText {
                        anchors.centerIn: parent
                        text: "Apply"
                        color: Theme.palette.background
                        font.pixelSize: Theme.typography.secondaryText
                        font.weight: Theme.typography.weightBold
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing.small

                Rectangle {
                    width: 9
                    height: 9
                    radius: 4.5
                    Layout.alignment: Qt.AlignVCenter
                    color: page.backend && page.backend.connectionStatus === "Connected"
                           ? Theme.palette.success
                           : Theme.palette.warning
                }
                LogosText {
                    text: {
                        if (!page.backend)
                            return "";
                        var height = page.backend.chainHeight > 0 ? page.backend.chainHeight : "—";
                        return "Chain height: " + height + "   ·   " + page.backend.connectionStatus;
                    }
                    color: Theme.palette.textMuted
                    font.pixelSize: Theme.typography.secondaryText
                }
            }
        }

        SectionHeader { title: "Recent Blocks" }

        ListView {
            id: blockList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spacing.small
            model: page.backend ? page.backend.recentBlocks : []

            delegate: BlockRow {
                width: ListView.view.width
                block: modelData
                onClicked: page.explorer.navigateBlock(modelData.blockId)
            }

            footer: Item {
                width: blockList.width
                height: loadMore.visible ? loadMore.implicitHeight + Theme.spacing.large : 0

                Rectangle {
                    id: loadMore
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: Theme.spacing.medium
                    visible: blockList.count > 0
                    implicitHeight: 36
                    implicitWidth: 140
                    radius: Theme.spacing.radiusMedium
                    color: moreHover.hovered ? Theme.palette.backgroundSecondary : Theme.palette.backgroundElevated
                    border.width: 1
                    border.color: Theme.palette.borderSecondary

                    HoverHandler { id: moreHover }
                    TapHandler { onTapped: if (page.backend) page.backend.loadMoreBlocks() }

                    LogosText {
                        anchors.centerIn: parent
                        text: "Load more"
                        color: Theme.palette.textSecondary
                        font.pixelSize: Theme.typography.secondaryText
                    }
                }
            }

            // Empty state.
            LogosText {
                anchors.centerIn: parent
                visible: blockList.count === 0
                width: parent.width * 0.8
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: page.backend && page.backend.connectionStatus === "Connected"
                      ? "No blocks indexed yet."
                      : "Waiting for the indexer… set the config path above if it isn't running."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
        }
    }

    function applyConfig() {
        if (!page.backend)
            return;
        // Empty port → backend keeps its default (8779).
        logos.watch(page.backend.applyIndexerConfig(configField.text, ""),
            function (ok) {}, function (err) {});
    }
}
