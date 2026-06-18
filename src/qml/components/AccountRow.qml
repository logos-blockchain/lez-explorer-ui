import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Clickable row for an account reference ({ accountId, nonce?, balance? }).
// Used for tx account lists and account search results.
Rectangle {
    id: root

    property var account: ({})
    signal clicked()

    height: layout.implicitHeight + Theme.spacing.medium * 2
    radius: Theme.spacing.radiusMedium
    color: hover.hovered ? Theme.palette.backgroundSecondary : "transparent"
    border.width: 1
    border.color: hover.hovered ? Theme.palette.borderInteractive : Theme.palette.borderSecondary

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked() }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing.medium
        spacing: Theme.spacing.medium

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.tiny

            MonoText {
                text: root.account.accountId || ""
                color: Theme.palette.text
                wrapMode: Text.NoWrap
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
            LogosText {
                visible: root.account.nonce !== undefined
                text: "Nonce: " + (root.account.nonce !== undefined ? root.account.nonce : "")
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
        }

        LogosText {
            visible: root.account.balance !== undefined
            text: (root.account.balance !== undefined ? root.account.balance : "")
            color: Theme.palette.success
            font.pixelSize: Theme.typography.secondaryText
            font.weight: Theme.typography.weightBold
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
