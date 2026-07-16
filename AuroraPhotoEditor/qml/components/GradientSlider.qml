import QtQuick 2.0
import "../"

// Left-aligned slider with a gradient-filled track (same gradient as
// GradientButton) and a plain white thumb — replaces Silica's stock
// Slider, whose track/handle colors can't be restyled on this Qt version.
Item {
    id: root

    property real minimumValue: 0
    property real maximumValue: 100
    property real value: 0
    property int stepSize: 1

    signal moved(real newValue)

    width: parent ? parent.width : NeonTheme.px(400)
    height: NeonTheme.px(48)

    readonly property real ratio: maximumValue > minimumValue
        ? Math.max(0, Math.min(1, (value - minimumValue) / (maximumValue - minimumValue)))
        : 0

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: NeonTheme.px(12)
        radius: height / 2
        color: NeonTheme.bgChip
    }

    GradientRect {
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(track.height, root.ratio * track.width)
        height: track.height
        radius: height / 2
        stops: [
            { position: 0.0, color: NeonTheme.gradientStart },
            { position: 1.0, color: NeonTheme.gradientEnd }
        ]
    }

    Rectangle {
        id: thumb
        width: NeonTheme.px(32)
        height: width
        radius: width / 2
        color: "#ffffff"
        anchors.verticalCenter: parent.verticalCenter
        x: root.ratio * (root.width - width)
    }

    MouseArea {
        anchors.fill: parent

        function updateFromX(mx) {
            var r = Math.max(0, Math.min(1, mx / root.width))
            var v = root.minimumValue + r * (root.maximumValue - root.minimumValue)
            if (root.stepSize > 0)
                v = Math.round(v / root.stepSize) * root.stepSize
            v = Math.max(root.minimumValue, Math.min(root.maximumValue, v))
            root.value = v
            root.moved(v)
        }

        onPressed: updateFromX(mouse.x)
        onPositionChanged: if (pressed) updateFromX(mouse.x)
    }
}
