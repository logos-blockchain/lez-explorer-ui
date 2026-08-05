import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// Labelled single-line input for the settings form. A non-empty `errorText`
// turns the border red and shows the message under the field.
ColumnLayout {
    id: field

    property alias label: labelText.text
    property alias text: input.text
    property alias placeholder: input.placeholderText
    property bool mono: false
    property string errorText: ""

    spacing: Theme.spacing.small

    FieldLabel { id: labelText }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 38
        radius: Theme.spacing.radiusMedium
        color: Theme.palette.backgroundElevated
        border.width: 1
        border.color: field.errorText !== "" ? Theme.palette.error
                    : input.activeFocus ? Theme.palette.borderInteractive
                                        : Theme.palette.borderSecondary

        TextField {
            id: input
            anchors.fill: parent
            anchors.leftMargin: Theme.spacing.medium
            anchors.rightMargin: Theme.spacing.medium
            verticalAlignment: TextInput.AlignVCenter
            font.family: field.mono ? "Menlo, Monaco, Courier New, monospace" : undefined
            font.pixelSize: Theme.typography.secondaryText
            color: Theme.palette.text
            background: Item {}
        }
    }

    LogosText {
        visible: field.errorText !== ""
        text: field.errorText
        color: Theme.palette.error
        font.pixelSize: Theme.typography.secondaryText
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }
}
