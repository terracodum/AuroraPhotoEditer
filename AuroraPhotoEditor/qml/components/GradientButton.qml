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
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
