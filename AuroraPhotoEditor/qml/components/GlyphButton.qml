import QtQuick 2.0
import "../"

// Header/toolbar icon button built on our Icon glyph set. Touch target is
// always NeonTheme.iconButtonSize (>= minTouchTarget) even though the
// glyph itself is drawn smaller, per README "Минимальный размер тач-таргета".
Item {
    id: root

    property string iconName: "close"
    // `enabled` is Item's own built-in property, intentionally not redeclared.
    property color iconColor: NeonTheme.textPrimary
    property color bgColor: NeonTheme.bgCard
    // SVG icons ship a fixed baked-in color; force the Canvas fallback for
    // instances that need a specific dynamic iconColor instead (e.g. a
    // black glyph on a colored button).
    property bool preferCanvas: false
    signal clicked

    width: NeonTheme.iconButtonSize
    height: NeonTheme.iconButtonSize
    opacity: enabled ? 1.0 : 0.35

    Rectangle {
        anchors.fill: parent
        // Rounded square, not a circle — radiusSmall stays well under half
        // the button size at any iconButtonSize we use.
        radius: NeonTheme.radiusSmall
        color: mouseArea.pressed ? Qt.darker(root.bgColor, 1.2) : root.bgColor
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.16)
    }

    GlyphIcon {
        anchors.centerIn: parent
        name: root.iconName
        preferCanvas: root.preferCanvas
        width: parent.width * 0.76
        height: parent.height * 0.76
        strokeColor: root.iconColor
        lineWidth: 4.4
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
