import QtQuick 2.0
import "../"

// Primary CTA "pill" button: gradient fill, radius 999 (see NeonTheme /
// README Design Tokens - "Основной градиент действия"). The fill is a
// GradientRect (Canvas-based) rather than a plain `Rectangle.gradient`, so
// it paints left-to-right instead of top-to-bottom (see GradientRect.qml).
Item {
    id: root

    property string text: ""
    // NOTE: `enabled` is Item's own built-in property — not redeclared here
    // (QML forbids shadowing a base-type property). Disabling it also
    // auto-disables the MouseArea below and everything else underneath.
    signal clicked

    width: parent ? parent.width : NeonTheme.px(400)
    height: NeonTheme.ctaHeight
    opacity: enabled ? 1.0 : 0.4
    scale: mouseArea.pressed ? 0.97 : 1.0

    Behavior on scale {
        NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
    }

    GradientRect {
        anchors.fill: parent
        radius: NeonTheme.radiusPill
        stops: [
            { position: 0.0, color: NeonTheme.gradientStart },
            { position: 1.0, color: NeonTheme.gradientEnd }
        ]
    }

    Text {
        anchors.centerIn: parent
        text: root.text
        color: NeonTheme.textPrimary
        font.family: NeonTheme.fontDisplay
        font.weight: NeonTheme.fontWeightBold
        font.pixelSize: NeonTheme.fontSizeButton
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onPressed: Haptics.press()
        onClicked: {
            Haptics.release()
            root.clicked()
        }
    }
}
