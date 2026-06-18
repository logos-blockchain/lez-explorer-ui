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
    readonly property bool connected: backend && backend.connectionStatus === "Connected"

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing.medium

        // Health bar.
        Card {
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing.small

                Rectangle {
                    width: 9
                    height: 9
                    radius: 4.5
                    Layout.alignment: Qt.AlignVCenter
                    color: page.connected ? Theme.palette.success : Theme.palette.warning
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
                    Layout.alignment: Qt.AlignVCenter
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredHeight: 34
                    Layout.preferredWidth: 100
                    radius: Theme.spacing.radiusMedium
                    color: settingsHover.hovered ? Theme.palette.backgroundSecondary : Theme.palette.backgroundElevated
                    border.width: 1
                    border.color: Theme.palette.borderSecondary

                    HoverHandler { id: settingsHover }
                    TapHandler { onTapped: if (page.explorer) page.explorer.navigateSettings() }

                    LogosText {
                        anchors.centerIn: parent
                        text: "Settings"
                        color: Theme.palette.textSecondary
                        font.pixelSize: Theme.typography.secondaryText
                    }
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
            ColumnLayout {
                anchors.centerIn: parent
                visible: blockList.count === 0
                width: parent.width * 0.8
                spacing: Theme.spacing.medium

                LogosText {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: page.connected
                          ? "No blocks indexed yet."
                          : "No indexer running yet. Open Settings to configure and start it."
                    color: Theme.palette.textMuted
                    font.pixelSize: Theme.typography.secondaryText
                }

                Rectangle {
                    visible: !page.connected
                    Layout.alignment: Qt.AlignHCenter
                    implicitHeight: 38
                    implicitWidth: 160
                    radius: Theme.spacing.radiusMedium
                    color: openHover.hovered ? Qt.lighter(Theme.palette.primary, 1.1) : Theme.palette.primary

                    HoverHandler { id: openHover }
                    TapHandler { onTapped: if (page.explorer) page.explorer.navigateSettings() }

                    LogosText {
                        anchors.centerIn: parent
                        text: "Open Settings"
                        color: Theme.palette.background
                        font.pixelSize: Theme.typography.secondaryText
                        font.weight: Theme.typography.weightBold
                    }
                }
            }
        }
    }
}
