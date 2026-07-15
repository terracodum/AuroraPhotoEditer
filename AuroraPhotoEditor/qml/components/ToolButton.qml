import QtQuick 2.0
import "../"

// Docked toolbar square button ("Фон" / "Улучшение" / "Стиль"), radius 28,
// each with its own accent color (README screen 3, DockedPanel).
Rectangle {
    id: root

    property string label: ""
    property string iconName: "layers"
    property color accent: NeonTheme.accentPurple
    // `enabled` is Item's own built-in property, intentionally not redeclared.
    signal clicked

    radius: NeonTheme.radiusMedium
    color: NeonTheme.bgCard
    opacity: enabled ? 1.0 : 0.4
    border.width: 2
    border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.5)

    Column {
        anchors.centerIn: parent
        spacing: NeonTheme.paddingTiny

        GlyphIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.iconName
            width: NeonTheme.px(40)
            height: NeonTheme.px(40)
            strokeColor: root.accent
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontBody
            font.weight: NeonTheme.fontWeightMedium
            font.pixelSize: NeonTheme.fontSizeCaption
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
