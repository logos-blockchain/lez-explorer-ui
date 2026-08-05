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
    property string txHash: ""

    property var tx: ({})
    property bool loaded: false
    readonly property string txType: tx.type || ""

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

            SectionHeader { title: "Transaction Details" }

            LogosText {
                visible: !page.loaded
                text: "Loading…"
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
            LogosText {
                visible: page.loaded && Object.keys(page.tx).length === 0
                text: "Transaction not found."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }

            Card {
                Layout.fillWidth: true
                visible: page.loaded && Object.keys(page.tx).length > 0

                InfoRow { label: "Hash"; value: page.tx.hash || ""; mono: true; copyable: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacing.medium
                    FieldLabel { text: "Type" }
                    TxTypeBadge { type: page.txType || "Public"; Layout.alignment: Qt.AlignTop }
                    Item { Layout.fillWidth: true }
                }

                // Public.
                InfoRow {
                    visible: page.txType === "Public"
                    label: "Program ID"; value: page.tx.programId || ""; mono: true; copyable: true
                }
                InfoRow {
                    visible: page.txType === "Public"
                    label: "Instruction Data"
                    value: (page.tx.instructionDataCount !== undefined ? page.tx.instructionDataCount : 0) + " words"
                }
                InfoRow {
                    visible: page.txType === "Public" || page.txType === "PrivacyPreserving"
                    label: "Signatures"
                    value: String(page.tx.signatureCount !== undefined ? page.tx.signatureCount : 0)
                }

                // Privacy-preserving. Each private action bundles a nullifier,
                // root, commitment and encrypted post-state.
                InfoRow {
                    visible: page.txType === "PrivacyPreserving"
                    label: "Private Actions"
                    value: String(page.tx.privateActionsCount !== undefined ? page.tx.privateActionsCount : 0)
                }
                InfoRow {
                    visible: page.txType === "PrivacyPreserving"
                    label: "Proof Size"
                    value: (page.tx.proofSizeBytes !== undefined ? page.tx.proofSizeBytes : 0) + " bytes"
                }
                InfoRow {
                    visible: page.txType === "PrivacyPreserving"
                    label: "Validity Window"
                    value: "[" + (page.tx.validityWindowStart !== undefined ? page.tx.validityWindowStart : 0)
                           + ", " + (page.tx.validityWindowEnd !== undefined ? page.tx.validityWindowEnd : 0) + ")"
                }

                // Program deployment.
                InfoRow {
                    visible: page.txType === "ProgramDeployment"
                    label: "Bytecode Size"
                    value: (page.tx.bytecodeSizeBytes !== undefined ? page.tx.bytecodeSizeBytes : 0) + " bytes"
                }
            }

            SectionHeader {
                visible: page.loaded && page.tx.accounts !== undefined && page.tx.accounts.length > 0
                title: "Accounts (" + (page.tx.accounts ? page.tx.accounts.length : 0) + ")"
            }

            Repeater {
                model: page.tx.accounts !== undefined ? page.tx.accounts : []
                delegate: AccountRow {
                    Layout.fillWidth: true
                    account: modelData
                    onClicked: page.explorer.navigateAccount(modelData.accountId)
                }
            }

            Item { Layout.preferredHeight: Theme.spacing.large }
        }
    }

    function fetch() {
        if (!page.backend)
            return;
        logos.watch(page.backend.getTransaction(page.txHash),
            function (result) {
                page.tx = result || ({});
                page.loaded = true;
            },
            function (err) {
                page.tx = ({});
                page.loaded = true;
            });
    }
}
