import QtQuick 2.0
import QtQuick.Window 2.2

// Icon glyph used across the "Neon Pop" UI. Prefers a hand-authored SVG from
// qml/icons/glyphs/ (colors already baked in to match each icon's context —
// e.g. the "Фон" tool icon ships pre-tinted accentPurple) and falls back to
// the original hand-drawn Canvas vector for any `name` without a matching
// file, so nothing regresses for icons not yet supplied as SVG.
//
// NOTE: named GlyphIcon, not Icon — Sailfish.Silica already ships its own
// `Icon` type (source/color-based), and since QML resolves a name clash
// between an implicit same-directory file and an imported module in favor
// of the module, a file named Icon.qml here would be silently shadowed by
// Silica's Icon everywhere Sailfish.Silica is also imported (which is
// nearly every page) — causing "Cannot assign to non-existent property"
// for our custom properties (name/strokeColor) at runtime.
Item {
    id: root

    property string name: "close"
    property color strokeColor: "#f5f1ff"
    property real lineWidth: 2.2
    // SVG files ship a baked-in color chosen for the dark theme; set this to
    // force the hand-drawn Canvas fallback instead, when an instance needs
    // a different (dynamic) strokeColor — e.g. a dark glyph on a light chip.
    property bool preferCanvas: false

    width: 32
    height: 32

    readonly property var svgFiles: ({
        "undo": "undo.svg",
        "reset": "reset.svg",
        "save": "save.svg",
        "about": "about.svg",
        "chevronDown": "pulldown-chevron.svg",
        "image": "empty-select-photo.svg",
        "gallery": "empty-select-photo.svg",
        "layers": "background-tool.svg",
        "spark": "enhance-tool.svg",
        "palette": "style-tool.svg",
        "brush": "brush-eraser.svg",
        "check": "success.svg",
        "warning": "error.svg",
        "close": "close.svg"
    })
    readonly property string svgFile: preferCanvas ? "" : (svgFiles[root.name] || "")

    Image {
        anchors.fill: parent
        visible: root.svgFile !== ""
        source: root.svgFile !== "" ? Qt.resolvedUrl("../icons/glyphs/" + root.svgFile) : ""
        sourceSize: Qt.size(width * canvasIcon.renderScale, height * canvasIcon.renderScale)
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    Canvas {
        id: canvasIcon
        anchors.fill: parent
        visible: root.svgFile === ""

        // Canvas has no built-in HiDPI awareness — it rasterizes at the
        // item's logical size and gets upscaled, which reads as blurry/
        // low-quality on dense screens. Render the backing store at the
        // real device pixel ratio instead and let the 2D context scale
        // drawing commands back down.
        readonly property real renderScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0

        canvasSize: Qt.size(width * renderScale, height * renderScale)
        renderTarget: Canvas.FramebufferObject
        antialiasing: true

        Connections {
            target: root
            onNameChanged: canvasIcon.requestPaint()
            onStrokeColorChanged: canvasIcon.requestPaint()
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.scale(renderScale, renderScale)
            ctx.strokeStyle = root.strokeColor
            ctx.fillStyle = root.strokeColor
            ctx.lineWidth = root.lineWidth * (width / 32)
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            var w = width, h = height
            var cx = w / 2, cy = h / 2
            var name = root.name

            switch (name) {
            case "undo":
            case "reset": {
                // Circular arrow: sweep an arc, then attach an arrowhead built
                // from the tangent/normal at the arc's actual end point, so it
                // always lines up with the stroke instead of a hand-picked
                // triangle that may miss the endpoint.
                var isUndo = (name === "undo")
                var r = w * 0.28
                var startAngle = isUndo ? Math.PI * 0.20 : Math.PI * 1.30
                var ccw = isUndo
                var sweep = Math.PI * 1.55
                var endAngle = ccw ? startAngle - sweep : startAngle + sweep

                ctx.beginPath()
                ctx.arc(cx, cy, r, startAngle, endAngle, ccw)
                ctx.stroke()

                var dir = ccw ? -1 : 1
                var tx = -Math.sin(endAngle) * dir
                var ty = Math.cos(endAngle) * dir
                var nx = -ty
                var ny = tx
                var ex = cx + r * Math.cos(endAngle)
                var ey = cy + r * Math.sin(endAngle)
                var headLen = w * 0.30

                ctx.beginPath()
                ctx.moveTo(ex + tx * headLen * 0.6, ey + ty * headLen * 0.6)
                ctx.lineTo(ex - tx * headLen * 0.35 + nx * headLen * 0.5, ey - ty * headLen * 0.35 + ny * headLen * 0.5)
                ctx.lineTo(ex - tx * headLen * 0.35 - nx * headLen * 0.5, ey - ty * headLen * 0.35 - ny * headLen * 0.5)
                ctx.closePath()
                ctx.fill()
                break
            }

            case "save":
                ctx.beginPath()
                ctx.moveTo(w * 0.22, h * 0.18)
                ctx.lineTo(w * 0.78, h * 0.18)
                ctx.lineTo(w * 0.78, h * 0.82)
                ctx.lineTo(w * 0.22, h * 0.82)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.32)
                ctx.lineTo(cx, h * 0.62)
                ctx.moveTo(w * 0.36, h * 0.5)
                ctx.lineTo(cx, h * 0.64)
                ctx.lineTo(w * 0.64, h * 0.5)
                ctx.stroke()
                break

            case "about":
                ctx.beginPath()
                ctx.arc(cx, cy, w * 0.34, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(cx, h * 0.33, w * 0.03, 0, Math.PI * 2)
                ctx.fill()
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.46)
                ctx.lineTo(cx, h * 0.68)
                ctx.stroke()
                break

            case "image":
            case "gallery":
                ctx.beginPath()
                ctx.moveTo(w * 0.18, h * 0.22)
                ctx.lineTo(w * 0.82, h * 0.22)
                ctx.lineTo(w * 0.82, h * 0.78)
                ctx.lineTo(w * 0.18, h * 0.78)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(w * 0.34, h * 0.40, w * 0.06, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(w * 0.24, h * 0.70)
                ctx.lineTo(w * 0.42, h * 0.50)
                ctx.lineTo(w * 0.56, h * 0.62)
                ctx.lineTo(w * 0.68, h * 0.46)
                ctx.lineTo(w * 0.78, h * 0.70)
                ctx.stroke()
                break

            case "layers":
                for (var i = 0; i < 3; i++) {
                    var oy = h * (0.28 + i * 0.16)
                    ctx.beginPath()
                    ctx.moveTo(cx, oy - h * 0.10)
                    ctx.lineTo(w * 0.80, oy)
                    ctx.lineTo(cx, oy + h * 0.10)
                    ctx.lineTo(w * 0.20, oy)
                    ctx.closePath()
                    ctx.stroke()
                }
                break

            case "spark":
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.14)
                ctx.lineTo(cx + w * 0.07, cy - h * 0.05)
                ctx.lineTo(w * 0.86, cy)
                ctx.lineTo(cx + w * 0.07, cy + h * 0.05)
                ctx.lineTo(cx, h * 0.86)
                ctx.lineTo(cx - w * 0.07, cy + h * 0.05)
                ctx.lineTo(w * 0.14, cy)
                ctx.lineTo(cx - w * 0.07, cy - h * 0.05)
                ctx.closePath()
                ctx.stroke()
                break

            case "palette":
                ctx.beginPath()
                ctx.arc(cx, cy, w * 0.34, Math.PI * 0.15, Math.PI * 1.95, false)
                ctx.stroke()
                var dots = [[0.40, 0.36], [0.58, 0.32], [0.68, 0.50], [0.44, 0.60]]
                for (var d = 0; d < dots.length; d++) {
                    ctx.beginPath()
                    ctx.arc(w * dots[d][0], h * dots[d][1], w * 0.035, 0, Math.PI * 2)
                    ctx.fill()
                }
                break

            case "brush":
                ctx.beginPath()
                ctx.moveTo(w * 0.72, h * 0.16)
                ctx.lineTo(w * 0.84, h * 0.28)
                ctx.lineTo(w * 0.44, h * 0.68)
                ctx.lineTo(w * 0.32, h * 0.56)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(w * 0.34, h * 0.58)
                ctx.lineTo(w * 0.24, h * 0.86)
                ctx.lineTo(w * 0.44, h * 0.70)
                ctx.stroke()
                break

            case "eraser":
                ctx.save()
                ctx.translate(cx, cy)
                ctx.rotate(-Math.PI * 0.2)
                ctx.beginPath()
                ctx.moveTo(-w * 0.28, -h * 0.14)
                ctx.lineTo(w * 0.10, -h * 0.14)
                ctx.lineTo(w * 0.10, h * 0.14)
                ctx.lineTo(-w * 0.28, h * 0.14)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(-w * 0.02, -h * 0.14)
                ctx.lineTo(-w * 0.02, h * 0.14)
                ctx.stroke()
                ctx.restore()
                break

            case "close":
                ctx.beginPath()
                ctx.moveTo(w * 0.26, h * 0.26)
                ctx.lineTo(w * 0.74, h * 0.74)
                ctx.moveTo(w * 0.74, h * 0.26)
                ctx.lineTo(w * 0.26, h * 0.74)
                ctx.stroke()
                break

            case "check":
                ctx.beginPath()
                ctx.moveTo(w * 0.20, h * 0.52)
                ctx.lineTo(w * 0.42, h * 0.72)
                ctx.lineTo(w * 0.80, h * 0.30)
                ctx.stroke()
                break

            case "chevronDown":
                ctx.beginPath()
                ctx.moveTo(w * 0.24, h * 0.36)
                ctx.lineTo(cx, h * 0.62)
                ctx.lineTo(w * 0.76, h * 0.36)
                ctx.stroke()
                break

            case "warning":
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.16)
                ctx.lineTo(w * 0.86, h * 0.82)
                ctx.lineTo(w * 0.14, h * 0.82)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.40)
                ctx.lineTo(cx, h * 0.62)
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(cx, h * 0.72, w * 0.025, 0, Math.PI * 2)
                ctx.fill()
                break

            case "back":
                ctx.beginPath()
                ctx.moveTo(w * 0.62, h * 0.22)
                ctx.lineTo(w * 0.32, cy)
                ctx.lineTo(w * 0.62, h * 0.78)
                ctx.stroke()
                break

            case "plus":
                ctx.beginPath()
                ctx.moveTo(cx, h * 0.22)
                ctx.lineTo(cx, h * 0.78)
                ctx.moveTo(w * 0.22, cy)
                ctx.lineTo(w * 0.78, cy)
                ctx.stroke()
                break

            default:
                ctx.beginPath()
                ctx.arc(cx, cy, w * 0.3, 0, Math.PI * 2)
                ctx.stroke()
            }
        }
    }
}
