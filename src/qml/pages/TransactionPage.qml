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
                InfoRow { visible: !!page.tx.blockId; label: "Block"; value: page.tx.blockId ? "#" + page.tx.blockId : "" }
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
                    label: "Program Account"; value: page.tx.programAccountId || ""; mono: true; copyable: true
                }
                InfoRow {
                    visible: page.txType === "Public"
                    label: "Message"
                    value: page.tx.message ? page.tx.message : "(empty)"
                    mono: !!page.tx.message
                    copyable: !!page.tx.message
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
            }

            // What the transaction was charged.
            SectionHeader {
                visible: page.loaded && page.tx.receipt !== undefined
                title: "Receipt"
            }
            Card {
                Layout.fillWidth: true
                visible: page.loaded && page.tx.receipt !== undefined

                InfoRow {
                    label: "Outcome"
                    value: page.tx.receipt && page.tx.receipt.success ? "Applied" : "Reverted (fee still charged)"
                }
                InfoRow { label: "Execution Gas"; value: page.tx.receipt ? page.tx.receipt.gasExec : "" }
                InfoRow { label: "Storage Gas"; value: page.tx.receipt ? page.tx.receipt.gasStor : "" }
                InfoRow { label: "Base Fee"; value: page.tx.receipt ? page.tx.receipt.feeBase : "" }
                InfoRow { label: "Tip"; value: page.tx.receipt ? page.tx.receipt.tip : "" }
                InfoRow { label: "Refund"; value: page.tx.receipt ? page.tx.receipt.refund : "" }
            }

            // The declared caps, not the amount charged. Public txs always show
            // this card; privacy-preserving ones only when a public signer pays.
            SectionHeader {
                visible: page.loaded && (page.txType === "Public" || page.tx.fee !== undefined)
                title: "Fee Declaration"
            }
            Card {
                Layout.fillWidth: true
                visible: page.loaded && (page.txType === "Public" || page.tx.fee !== undefined)

                LogosText {
                    visible: page.tx.fee === undefined
                    text: "Fee-exempt (system transaction)"
                    color: Theme.palette.textMuted
                    font.pixelSize: Theme.typography.secondaryText
                }
                InfoRow {
                    visible: page.tx.fee !== undefined
                    label: "Payer"; value: page.tx.fee ? page.tx.fee.payer : ""; mono: true; copyable: true
                }
                InfoRow {
                    visible: page.tx.fee !== undefined
                    label: "Gas Limit"; value: page.tx.fee ? page.tx.fee.gasLimit : ""
                }
                InfoRow {
                    visible: page.tx.fee !== undefined
                    label: "Tip"; value: page.tx.fee ? page.tx.fee.tip : ""
                }
                InfoRow {
                    visible: page.tx.fee !== undefined
                    label: "Max Fee"; value: page.tx.fee ? page.tx.fee.maxFee : ""
                }
            }

            // Fee paid inside the proof: the amount and payer stay private; only
            // the tip and the refund target are public.
            SectionHeader {
                visible: page.loaded && page.tx.provenFee !== undefined
                title: "Fee In Proof"
            }
            Card {
                Layout.fillWidth: true
                visible: page.loaded && page.tx.provenFee !== undefined

                InfoRow { label: "Paid"; value: "In proof (private)" }
                InfoRow { label: "Tip"; value: page.tx.provenFee ? page.tx.provenFee.tip : "" }
                InfoRow {
                    visible: !!page.tx.provenFee && !!page.tx.provenFee.refundTo
                    label: "Refund To"; value: page.tx.provenFee ? page.tx.provenFee.refundTo : ""; mono: true; copyable: true
                }
                InfoRow {
                    label: "Fallback Action"; value: page.tx.provenFee && page.tx.provenFee.hasFallback ? "Yes" : "No"
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
