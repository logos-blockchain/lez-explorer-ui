import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Top navigation: back / forward / home + title.
RowLayout {
    id: root

    property bool canGoBack: false
    property bool canGoForward: false
    signal backClicked()
    signal forwardClicked()
    signal homeClicked()
    signal settingsClicked()

    spacing: Theme.spacing.small

    IconButton {
        source: Qt.resolvedUrl("../icons/arrow-left.svg")
        enabled: root.canGoBack
        onClicked: root.backClicked()
    }
    IconButton {
        source: Qt.resolvedUrl("../icons/arrow-right.svg")
        enabled: root.canGoForward
        onClicked: root.forwardClicked()
    }
    IconButton {
        source: Qt.resolvedUrl("../icons/home.svg")
        onClicked: root.homeClicked()
    }

    LogosText {
        text: "LEZ Explorer"
        color: Theme.palette.primary
        font.pixelSize: Theme.typography.titleText
        font.weight: Theme.typography.weightBold
        Layout.leftMargin: Theme.spacing.small
        Layout.alignment: Qt.AlignVCenter
    }

    Item { Layout.fillWidth: true }

    IconButton {
        source: Qt.resolvedUrl("../icons/settings.svg")
        onClicked: root.settingsClicked()
    }
}
