import QtQuick 2.0
import "../"

// Toast/banner used for: oversized-or-corrupt photo on load, "previous
// operation canceled" on tool switch during isProcessing, and project-saved
// confirmation (README "Toast / баннер ошибки"). Call show(message, kind)
// from the page; auto-hides after NeonTheme.toastDurationMs.
Item {
    id: root

    property string message: ""
    property string kind: "error" // "error" | "success" | "info"

    width: parent ? parent.width - 2 * NeonTheme.paddingXLarge : NeonTheme.px(600)
    height: content.height + NeonTheme.paddingMedium * 2
    visible: opacity > 0
    opacity: 0

    function show(msg, toastKind) {
        message = msg
        kind = toastKind || "error"
        opacity = 1
        hideTimer.restart()
    }

    function hide() {
        opacity = 0
    }

    Behavior on opacity { NumberAnimation { duration: 220 } }

    Timer {
        id: hideTimer
        interval: NeonTheme.toastDurationMs
        onTriggered: root.hide()
    }

    Rectangle {
        anchors.fill: parent
        radius: NeonTheme.radiusSmall
        color: NeonTheme.bgChip
        border.width: 1
        border.color: root.kind === "error" ? NeonTheme.errorColor
                      : root.kind === "success" ? NeonTheme.successColor
                      : NeonTheme.accentPurple
    }

    Row {
        id: content
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: NeonTheme.paddingMedium
        spacing: NeonTheme.paddingSmall

        GlyphIcon {
            name: root.kind === "success" ? "check" : "warning"
            width: NeonTheme.px(34)
            height: NeonTheme.px(34)
            strokeColor: root.kind === "error" ? NeonTheme.errorColor
                         : root.kind === "success" ? NeonTheme.successColor
                         : NeonTheme.accentPurple
        }

        Text {
            width: parent.width - NeonTheme.px(34) - NeonTheme.px(34) - content.spacing * 2
            text: root.message
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontBody
            font.weight: NeonTheme.fontWeightMedium
            font.pixelSize: NeonTheme.fontSizeCaption
            wrapMode: Text.WordWrap
        }

        GlyphIcon {
            name: "close"
            width: NeonTheme.px(34)
            height: NeonTheme.px(34)
            strokeColor: NeonTheme.textTertiary

            MouseArea {
                anchors.fill: parent
                anchors.margins: -NeonTheme.paddingTiny
                onClicked: root.hide()
            }
        }
    }
}
