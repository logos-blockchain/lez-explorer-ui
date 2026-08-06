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

    // Ingestion status (from backend.syncStatus) drives the health/sync banner.
    readonly property var sync: backend ? backend.syncStatus : null
    readonly property string syncState: (sync && sync.state) ? sync.state : "Starting"
    readonly property string syncError: (sync && sync.error) ? sync.error : ""

    // A Stopped state with an error means auto-start failed (Task 1 injects
    // the reason); plain Stopped means the indexer simply isn't running.
    readonly property bool startFailed: syncState === "Stopped" && syncError !== ""

    // Human-readable line for the banner: the current phase (or the error), so
    // the user can tell catching-up from a real failure on first launch.
    function statusLine() {
        if (!backend)
            return "";
        var height = backend.chainHeight > 0 ? backend.chainHeight : "—";
        if (page.syncState === "Error")
            return page.syncError !== "" ? page.syncError : "Indexer error";
        if (page.syncState === "Stalled")
            return "Indexer stalled" + (page.syncError !== "" ? ": " + page.syncError : "")
                 + " · block " + height;
        if (page.syncState === "CaughtUp")
            return backend.chainHeight > 0 ? "Up to date · block " + height
                                           : "Up to date · no blocks indexed";
        if (page.syncState === "Syncing")
            return "Syncing… · block " + height;
        if (page.syncState === "Stopped")
            return page.startFailed ? "Indexer failed to start: " + page.syncError : "Indexer not running";
        return "Starting indexer…";
    }

    // Message for the empty block list, tailored to the current phase. Settings
    // lives only in the top-right gear, so we point there rather than add a button.
    function emptyStateText() {
        if (page.syncState === "Error")
            return (page.syncError !== "" ? "Indexer error: " + page.syncError : "Indexer error.")
                 + "\nOpen Settings to reconfigure and restart.";
        if (page.syncState === "Stalled")
            return (page.syncError !== "" ? "Indexer stalled: " + page.syncError : "Indexer stalled on an invalid block.")
                 + "\nIndexed blocks up to the stall are still browsable.";
        if (page.startFailed)
            return "Indexer failed to start: " + page.syncError
                 + "\nOpen Settings to fix the configuration.";
        if (page.syncState === "Syncing" || page.syncState === "Starting")
            return "Indexer is syncing, please wait…";
        if (page.syncState === "CaughtUp")
            return "No blocks indexed yet.";
        return "No indexer running.\nOpen Settings to configure and start it.";
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing.medium

        // Health / sync bar.
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
                    color: page.syncState === "Error" || page.syncState === "Stalled" || page.startFailed ? Theme.palette.error
                         : page.syncState === "CaughtUp" ? Theme.palette.success
                         : Theme.palette.warning
                }
                LogosText {
                    text: page.statusLine()
                    color: page.syncState === "Error" || page.syncState === "Stalled" || page.startFailed ? Theme.palette.error : Theme.palette.textMuted
                    font.pixelSize: Theme.typography.secondaryText
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    elide: Text.ElideRight
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

            // Empty state — phase-aware message; Settings lives in the top-right gear.
            LogosText {
                anchors.centerIn: parent
                visible: blockList.count === 0
                width: parent.width * 0.8
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: page.emptyStateText()
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
        }
    }
}
