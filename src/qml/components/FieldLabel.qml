import QtQuick
import QtQuick.Layouts
import Logos.Controls
import Logos.Theme

// The fixed-width label used at the start of a detail-card row. Pairs with
// InfoRow (plain values) or a custom delegate built inline in a page.
LogosText {
    color: Theme.palette.textSecondary
    font.pixelSize: Theme.typography.secondaryText
    Layout.preferredWidth: 150
    Layout.alignment: Qt.AlignTop
}
