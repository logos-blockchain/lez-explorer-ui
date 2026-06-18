import QtQuick
import Logos.Theme

// Themed square icon button. `source` is an SVG url; emits clicked().
Rectangle {
    id: root

    property url source
    property bool enabled: true
    property int size: 40
    signal clicked()

    implicitWidth: size
    implicitHeight: size
    radius: Theme.spacing.radiusMedium
    opacity: root.enabled ? 1.0 : 0.4
    color: hover.hovered && root.enabled ? Theme.palette.backgroundSecondary : Theme.palette.backgroundElevated
    border.width: 1
    border.color: Theme.palette.borderSecondary

    HoverHandler { id: hover; enabled: root.enabled }
    TapHandler { enabled: root.enabled; onTapped: root.clicked() }

    Image {
        anchors.centerIn: parent
        source: root.source
        sourceSize.width: Math.round(root.size * 0.5)
        sourceSize.height: Math.round(root.size * 0.5)
        width: Math.round(root.size * 0.5)
        height: Math.round(root.size * 0.5)
        fillMode: Image.PreserveAspectFit
    }
}
