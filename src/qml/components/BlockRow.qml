import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Clickable summary row for a block map ({ blockId, hash, txCount,
// timestampText, status }). Used on the home feed and in search results.
Rectangle {
    id: root

    property var block: ({})
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

            LogosText {
                text: "Block #" + (root.block.blockId !== undefined ? root.block.blockId : "?")
                color: Theme.palette.text
                font.pixelSize: Theme.typography.primaryText
                font.weight: Theme.typography.weightBold
            }
            MonoText {
                text: root.block.hash || ""
                color: Theme.palette.textSecondary
                wrapMode: Text.NoWrap
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
            LogosText {
                text: (root.block.txCount !== undefined ? root.block.txCount : 0) + " txs  ·  "
                      + (root.block.timestampText || "")
                color: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
            }
        }

        StatusBadge {
            status: root.block.status || "Pending"
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
