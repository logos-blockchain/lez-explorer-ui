import QtQuick
import Logos.Theme

// Transaction-type pill, with a friendly label per kind.
Badge {
    property string type: "Public"
    text: type === "PrivacyPreserving" ? "Privacy-Preserving" : type
    accent: type === "PrivacyPreserving" ? Theme.palette.primary : Theme.palette.info
}
