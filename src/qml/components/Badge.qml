import QtQuick
import Logos.Controls
import Logos.Theme

// A small coloured pill. `accent` tints both the text and a translucent fill.
Rectangle {
    id: root

    property string text: ""
    property color accent: Theme.palette.textSecondary

    implicitWidth: label.implicitWidth + Theme.spacing.medium * 2
    implicitHeight: label.implicitHeight + Theme.spacing.small
    radius: Theme.spacing.radiusSmall
    color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.15)
    border.width: 1
    border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4)

    LogosText {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.accent
        font.pixelSize: Theme.typography.badgeText
        font.weight: Theme.typography.weightBold
    }
}
