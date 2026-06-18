import QtQuick
import Logos.Theme

// Bedrock status pill: Finalized (success), Safe (warning), Pending (muted).
Badge {
    property string status: "Pending"
    text: status
    accent: status === "Finalized" ? Theme.palette.success
          : status === "Safe"      ? Theme.palette.warning
                                    : Theme.palette.textMuted
}
