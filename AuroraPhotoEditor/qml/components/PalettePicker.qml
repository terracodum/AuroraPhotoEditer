import QtQuick 2.0
import "../"

// Compact HSV color picker: saturation/value square + hue strip + a live
// preview swatch with hex readout. `color` is bindable both ways — set it
// to preselect a color, read it (or listen for colorPicked) to get the
// user's choice.
Item {
    id: root

    property color color: "#ffffff"
    signal colorPicked(color pickedColor)

    property real hue: 0
    property real sat: 0
    property real val: 1
    property bool _syncing: false

    width: parent ? parent.width : NeonTheme.px(400)
    height: svSquare.height + NeonTheme.paddingMedium + hueStrip.height +
            NeonTheme.paddingMedium + preview.height

    function _applyHsv() {
        _syncing = true
        root.color = Qt.hsva(hue, sat, val, 1.0)
        root.colorPicked(root.color)
        _syncing = false
    }

    function _syncFromColor(c) {
        // Defensive against `c` (or its hsv* accessors) briefly being
        // undefined during binding evaluation — was producing a real
        // "Cannot assign [undefined] to double" warning on some launches.
        if (c === undefined || c === null)
            return
        var h = c.hsvHue
        hue = (h === undefined || h < 0) ? hue : h
        sat = c.hsvSaturation !== undefined ? c.hsvSaturation : sat
        val = c.hsvValue !== undefined ? c.hsvValue : val
    }

    onColorChanged: if (!_syncing) _syncFromColor(color)
    Component.onCompleted: _syncFromColor(color)

    // Saturation (x) / Value (y) square for the current hue.
    Item {
        id: svSquare
        width: parent.width
        height: NeonTheme.px(200)

        GradientRect {
            anchors.fill: parent
            radius: NeonTheme.radiusMedium
            stops: [
                { position: 0.0, color: "#ffffff" },
                { position: 1.0, color: Qt.hsva(root.hue, 1.0, 1.0, 1.0) }
            ]
        }
        Rectangle {
            anchors.fill: parent
            radius: NeonTheme.radiusMedium
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "#000000" }
            }
        }
        Rectangle {
            width: NeonTheme.px(30)
            height: width
            radius: width / 2
            color: "transparent"
            border.width: 3
            border.color: "#ffffff"
            x: root.sat * (svSquare.width - width)
            y: (1 - root.val) * (svSquare.height - height)
        }

        MouseArea {
            anchors.fill: parent

            function update(mx, my) {
                root.sat = Math.max(0, Math.min(1, mx / width))
                root.val = 1 - Math.max(0, Math.min(1, my / height))
                root._applyHsv()
            }
            onPressed: update(mouse.x, mouse.y)
            onPositionChanged: if (pressed) update(mouse.x, mouse.y)
        }
    }

    // Hue strip (full rainbow, horizontal).
    Item {
        id: hueStrip
        anchors.top: svSquare.bottom
        anchors.topMargin: NeonTheme.paddingMedium
        width: parent.width
        height: NeonTheme.px(40)

        GradientRect {
            anchors.fill: parent
            radius: height / 2
            stops: [
                { position: 0.0, color: "#ff0000" },
                { position: 0.166, color: "#ffff00" },
                { position: 0.333, color: "#00ff00" },
                { position: 0.5, color: "#00ffff" },
                { position: 0.666, color: "#0000ff" },
                { position: 0.833, color: "#ff00ff" },
                { position: 1.0, color: "#ff0000" }
            ]
        }
        Rectangle {
            height: hueStrip.height * 1.4
            width: height
            radius: width / 2
            color: "transparent"
            border.width: 3
            border.color: "#ffffff"
            y: (hueStrip.height - height) / 2
            x: root.hue * (hueStrip.width - width)
        }

        MouseArea {
            anchors.fill: parent

            function update(mx) {
                root.hue = Math.max(0, Math.min(1, mx / width))
                root._applyHsv()
            }
            onPressed: update(mouse.x)
            onPositionChanged: if (pressed) update(mouse.x)
        }
    }

    Row {
        id: preview
        anchors.top: hueStrip.bottom
        anchors.topMargin: NeonTheme.paddingMedium
        spacing: NeonTheme.paddingSmall

        Rectangle {
            width: NeonTheme.px(44)
            height: width
            radius: width / 2
            color: root.color
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.25)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.color.toString().toUpperCase()
            color: NeonTheme.textSecondary
            font.family: NeonTheme.fontBody
            font.pixelSize: NeonTheme.fontSizeCaption
        }
    }
}
