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
    property var results: ({ blocks: [], transactions: [], accounts: [] })
    property string query: ""

    readonly property var blocks: results && results.blocks ? results.blocks : []
    readonly property var transactions: results && results.transactions ? results.transactions : []
    readonly property var accounts: results && results.accounts ? results.accounts : []
    readonly property int total: blocks.length + transactions.length + accounts.length

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.implicitHeight
        clip: true

        ColumnLayout {
            id: col
            width: parent.width
            spacing: Theme.spacing.medium

            SectionHeader { title: "Search Results" }

            LogosText {
                visible: page.total === 0
                text: page.query.length > 0
                      ? "No results for \"" + page.query + "\"."
                      : "No results."
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }

            SectionHeader {
                visible: page.blocks.length > 0
                title: "Blocks (" + page.blocks.length + ")"
                font.pixelSize: Theme.typography.subtitleText
            }
            Repeater {
                model: page.blocks
                delegate: BlockRow {
                    Layout.fillWidth: true
                    block: modelData
                    onClicked: page.explorer.navigateBlock(modelData.blockId)
                }
            }

            SectionHeader {
                visible: page.transactions.length > 0
                title: "Transactions (" + page.transactions.length + ")"
                font.pixelSize: Theme.typography.subtitleText
            }
            Repeater {
                model: page.transactions
                delegate: TxRow {
                    Layout.fillWidth: true
                    tx: modelData
                    onClicked: page.explorer.navigateTx(modelData.hash)
                }
            }

            SectionHeader {
                visible: page.accounts.length > 0
                title: "Accounts (" + page.accounts.length + ")"
                font.pixelSize: Theme.typography.subtitleText
            }
            Repeater {
                model: page.accounts
                delegate: AccountRow {
                    Layout.fillWidth: true
                    account: modelData
                    onClicked: page.explorer.navigateAccount(modelData.accountId)
                }
            }

            Item { Layout.preferredHeight: Theme.spacing.large }
        }
    }
}
