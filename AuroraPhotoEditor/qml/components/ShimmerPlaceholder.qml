import QtQuick 2.0
import "../"

// Skeleton shown in place of the photo while it's being picked/decoded,
// instead of a blank area. Cheap: one static card + one translucent band
// sweeping across it via a plain NumberAnimation on x (compositor-only,
// no Canvas repaint per frame). The animation's `running` is tied to
// `root.visible`, so it costs nothing once a photo is actually showing.
Item {
    id: root

    property bool running: false
    visible: running
    clip: true

    Rectangle {
        anchors.fill: parent
        radius: NeonTheme.radiusLarge
        color: NeonTheme.bgCard
    }

    GradientRect {
        id: sweep
        width: parent.width * 0.6
        height: parent.height
        x: -width
        stops: [
            { position: 0.0, color: Qt.rgba(1, 1, 1, 0.0) },
            { position: 0.5, color: Qt.rgba(1, 1, 1, 0.09) },
            { position: 1.0, color: Qt.rgba(1, 1, 1, 0.0) }
        ]

        NumberAnimation on x {
            running: root.visible
            loops: Animation.Infinite
            from: -sweep.width
            to: root.width
            duration: 1100
            easing.type: Easing.InOutQuad
        }
    }
}
