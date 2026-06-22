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
    property string accountId: ""

    property var account: ({})
    property bool loaded: false
    property var txs: []
    property bool txsLoaded: false

    Component.onCompleted: page.fetch()

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true

        ColumnLayout {
            id: col
            width: parent.width
            spacing: Theme.spacing.medium

            SectionHeader { title: "Account Details" }

            LogosText {
                visible: !page.loaded
                text: "Loading…"
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
            LogosText {
                visible: page.loaded && Object.keys(page.account).length <= 1
                text: "Account not found."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }

            Card {
                Layout.fillWidth: true
                visible: page.loaded && Object.keys(page.account).length > 1

                InfoRow { label: "Account ID"; value: page.account.accountId || ""; mono: true; copyable: true }
                InfoRow {
                    label: "Balance"
                    value: page.account.balance || "0"
                    valueColor: Theme.palette.success
                }
                InfoRow { label: "Program Owner"; value: page.account.programOwner || ""; mono: true; copyable: true }
                InfoRow { label: "Nonce"; value: page.account.nonce || "0" }
                InfoRow {
                    label: "Data Size"
                    value: (page.account.dataSizeBytes !== undefined ? page.account.dataSizeBytes : 0) + " bytes"
                }
            }

            SectionHeader {
                visible: page.loaded && Object.keys(page.account).length > 1
                title: "Transaction History"
            }

            LogosText {
                visible: page.txsLoaded && page.txs.length === 0 && Object.keys(page.account).length > 1
                text: "No transactions found."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }

            Repeater {
                model: page.txs
                delegate: TxRow {
                    Layout.fillWidth: true
                    tx: modelData
                    onClicked: page.explorer.navigateTx(modelData.hash)
                }
            }

            Item { Layout.preferredHeight: Theme.spacing.large }
        }
    }

    function fetch() {
        if (!page.backend)
            return;
        logos.watch(page.backend.getAccount(page.accountId),
            function (result) {
                page.account = result || ({});
                page.loaded = true;
            },
            function (err) {
                page.account = ({});
                page.loaded = true;
            });
        logos.watch(page.backend.getTransactionsByAccount(page.accountId, 0, 10),
            function (result) {
                page.txs = result || [];
                page.txsLoaded = true;
            },
            function (err) {
                page.txs = [];
                page.txsLoaded = true;
            });
    }
}
