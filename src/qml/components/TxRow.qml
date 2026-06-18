import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Clickable summary row for a transaction map ({ hash, type, ... }). Used in
// the block-detail tx list, account history, and search results.
Rectangle {
    id: root

    property var tx: ({})
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

        MonoText {
            text: root.tx.hash || ""
            color: Theme.palette.text
            wrapMode: Text.NoWrap
            elide: Text.ElideMiddle
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }

        TxTypeBadge {
            type: root.tx.type || "Public"
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
