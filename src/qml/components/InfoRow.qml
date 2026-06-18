import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// One "Label  value" row in a detail card. The value can be monospace. For
// non-text values (badges, clickable links) build a small RowLayout in the page
// with a FieldLabel instead.
RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property bool mono: false
    property color valueColor: Theme.palette.text

    spacing: Theme.spacing.medium
    Layout.fillWidth: true

    FieldLabel { text: root.label }

    LogosText {
        visible: !root.mono
        text: root.value
        color: root.valueColor
        font.pixelSize: Theme.typography.secondaryText
        wrapMode: Text.WrapAnywhere
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
    }

    MonoText {
        visible: root.mono
        text: root.value
        color: root.valueColor
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
    }
}
