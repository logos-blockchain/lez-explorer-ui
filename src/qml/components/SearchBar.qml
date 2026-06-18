import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Search input + button. Emits searchRequested(query) on Enter or button click.
RowLayout {
    id: root

    signal searchRequested(string query)

    spacing: Theme.spacing.small

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        radius: Theme.spacing.radiusMedium
        color: Theme.palette.backgroundElevated
        border.width: 1
        border.color: field.activeFocus ? Theme.palette.borderInteractive : Theme.palette.borderSecondary

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing.medium
            anchors.rightMargin: Theme.spacing.medium
            spacing: Theme.spacing.small

            Image {
                source: Qt.resolvedUrl("../icons/search.svg")
                sourceSize.width: 18
                sourceSize.height: 18
                width: 18
                height: 18
                fillMode: Image.PreserveAspectFit
                Layout.alignment: Qt.AlignVCenter
            }

            TextField {
                id: field
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                placeholderText: "Search by block id / block hash / tx hash / account id…"
                color: Theme.palette.text
                placeholderTextColor: Theme.palette.textMuted
                font.pixelSize: Theme.typography.secondaryText
                background: Item {}
                onAccepted: root.searchRequested(field.text)
            }
        }
    }

    Rectangle {
        Layout.preferredHeight: 40
        Layout.preferredWidth: 92
        radius: Theme.spacing.radiusMedium
        color: btnHover.hovered ? Qt.lighter(Theme.palette.primary, 1.1) : Theme.palette.primary

        HoverHandler { id: btnHover }
        TapHandler { onTapped: root.searchRequested(field.text) }

        LogosText {
            anchors.centerIn: parent
            text: "Search"
            color: Theme.palette.background
            font.pixelSize: Theme.typography.secondaryText
            font.weight: Theme.typography.weightBold
        }
    }
}
