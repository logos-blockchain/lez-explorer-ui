import QtQuick
import Logos.Controls
import Logos.Theme

// Monospace, selectable text for hashes / ids / signatures.
LogosText {
    font.family: "Menlo, Monaco, Courier New, monospace"
    font.pixelSize: Theme.typography.secondaryText
    color: Theme.palette.text
    textFormat: Text.PlainText
    wrapMode: Text.WrapAnywhere
}
