import QtQuick 2.0
import QtQuick.Window 2.2

// True horizontal (left-to-right) linear-gradient rounded rectangle.
// QtQuick's own `Gradient` type only reliably paints vertically on this
// project's Qt5 target (`Gradient.orientation` is a Qt 5.12+ property,
// unavailable here — see the Font.Weight comment in NeonTheme.qml for the
// same version constraint), so horizontal fills are drawn on a Canvas
// instead via CanvasRenderingContext2D.createLinearGradient, which has no
// such limitation.
Canvas {
    id: canvas

    // Array of {position: 0..1, color: "#rrggbb"} — same shape as
    // QtQuick's GradientStop, just as plain JS objects, so any number of
    // stops works (2 for a CTA button, N for a hue spectrum strip).
    property var stops: [
        { position: 0.0, color: "#8b5cf6" },
        { position: 1.0, color: "#ec4899" }
    ]
    property real radius: 0

    readonly property real renderScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    canvasSize: Qt.size(Math.max(width, 1) * renderScale, Math.max(height, 1) * renderScale)
    renderTarget: Canvas.FramebufferObject
    antialiasing: true

    onStopsChanged: requestPaint()
    onRadiusChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.scale(renderScale, renderScale)

        var w = width, h = height
        if (w <= 0 || h <= 0 || stops.length === 0)
            return

        var grad = ctx.createLinearGradient(0, 0, w, 0)
        for (var i = 0; i < stops.length; i++) {
            grad.addColorStop(stops[i].position, stops[i].color)
        }

        var r = Math.min(radius, w / 2, h / 2)
        ctx.beginPath()
        ctx.moveTo(r, 0)
        ctx.lineTo(w - r, 0)
        ctx.quadraticCurveTo(w, 0, w, r)
        ctx.lineTo(w, h - r)
        ctx.quadraticCurveTo(w, h, w - r, h)
        ctx.lineTo(r, h)
        ctx.quadraticCurveTo(0, h, 0, h - r)
        ctx.lineTo(0, r)
        ctx.quadraticCurveTo(0, 0, r, 0)
        ctx.closePath()
        ctx.fillStyle = grad
        ctx.fill()
    }
}
