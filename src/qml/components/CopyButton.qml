import QtQuick
import Logos.Theme

// Tiny copy-to-clipboard button. Self-contained: writes `value` to the system
// clipboard via a hidden TextEdit (which runs in the host GUI process, so it
// reaches the real desktop clipboard), then flips to a check mark briefly as
// confirmation.
Item {
    id: root

    property string value: ""
    property int size: 22
    implicitWidth: size
    implicitHeight: size

    // Off-screen helper used purely to push text onto the clipboard.
    TextEdit { id: clip; visible: false }

    Rectangle {
        anchors.fill: parent
        radius: Theme.spacing.radiusSmall
        color: hover.hovered ? Theme.palette.backgroundSecondary : "transparent"

        HoverHandler { id: hover }
        TapHandler {
            onTapped: {
                clip.text = root.value;
                clip.selectAll();
                clip.copy();
                clip.text = "";
                icon.source = icon.checkUrl;
                resetTimer.restart();
            }
        }

        Image {
            id: icon
            anchors.centerIn: parent
            property url copyUrl: Qt.resolvedUrl("../icons/copy.svg")
            property url checkUrl: Qt.resolvedUrl("../icons/check.svg")
            source: copyUrl
            sourceSize.width: Math.round(root.size * 0.72)
            sourceSize.height: Math.round(root.size * 0.72)
            width: Math.round(root.size * 0.72)
            height: Math.round(root.size * 0.72)
            fillMode: Image.PreserveAspectFit
        }

        Timer {
            id: resetTimer
            interval: 1200
            onTriggered: icon.source = icon.copyUrl
        }
    }
}
