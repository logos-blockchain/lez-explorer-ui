import QtQuick
import Logos.Controls
import Logos.Theme

LogosText {
    property string title: ""
    text: title
    color: Theme.palette.text
    font.pixelSize: Theme.typography.titleText
    font.weight: Theme.typography.weightBold
}
