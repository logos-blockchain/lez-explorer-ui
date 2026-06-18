import QtQuick
import QtQuick.Layouts
import Logos.Theme

// A themed surface panel that sizes to its content. Children are stacked
// vertically (they become items of the inner ColumnLayout).
Rectangle {
    id: root

    property int padding: Theme.spacing.large
    property int spacing: Theme.spacing.medium
    default property alias content: inner.data

    implicitHeight: inner.implicitHeight + padding * 2
    implicitWidth: inner.implicitWidth + padding * 2

    color: Theme.palette.backgroundElevated
    border.color: Theme.palette.borderSecondary
    border.width: 1
    radius: Theme.spacing.radiusLarge

    ColumnLayout {
        id: inner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.padding
        spacing: root.spacing
    }
}
