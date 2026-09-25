import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// The single status surface for ingestion: phase, height, and — when the node
// is unreachable — a way to act on it.
RowLayout {
    id: root

    // Indexer state, verbatim from the backend's syncStatus.
    property string syncState: ""
    property string syncError: ""
    property int chainHeight: 0
    // Set by the owner: syncError names a transport failure AND the indexer is
    // not currently making progress.
    property bool nodeUnreachable: false

    signal openBlockchainAppRequested()

    objectName: "explorerIndexerStatus"
    Layout.fillWidth: true
    spacing: Theme.spacing.medium

    readonly property string heightText: root.chainHeight > 0 ? root.chainHeight : "—"
    readonly property bool startFailed: root.syncState === "Stopped" && root.syncError !== ""
    readonly property bool failed:
        root.nodeUnreachable || root.syncState === "Error" || root.syncState === "Stalled"
        || root.startFailed

    function statusLine() {
        if (root.nodeUnreachable)
            return "The explorer can't index new blocks until it's running.";
        if (root.syncState === "Error")
            return root.syncError !== "" ? root.syncError : "Indexer error";
        if (root.syncState === "Stalled")
            return "Indexer stalled" + (root.syncError !== "" ? ": " + root.syncError : "")
                 + " · block " + root.heightText;
        if (root.syncState === "CaughtUp")
            return root.chainHeight > 0 ? "Up to date · block " + root.heightText
                                        : "Up to date · no blocks indexed";
        if (root.syncState === "Syncing")
            return "Syncing… · block " + root.heightText;
        if (root.syncState === "Stopped")
            return root.startFailed ? "Indexer failed to start: " + root.syncError
                                    : "Indexer not running";
        return "Starting indexer…";
    }

    LogosNotice {
        Layout.fillWidth: true
        shown: true
        severity: root.failed ? LogosNotice.Error
                : root.syncState === "CaughtUp" ? LogosNotice.Success
                : LogosNotice.Info
        title: root.nodeUnreachable ? qsTr("Can't reach the blockchain node") : ""
        message: root.statusLine()
    }

    LogosButton {
        objectName: "explorerOpenBlockchainAppButton"
        visible: root.nodeUnreachable
        text: qsTr("Check node status")
        variant: LogosButton.Variant.Primary
        Layout.alignment: Qt.AlignVCenter
        onClicked: root.openBlockchainAppRequested()
    }
}
