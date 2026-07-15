import QtQuick 2.0
import "../"

// Processing overlay: 96px ring (faint track + rotating accentGreen arc)
// plus the current operation's label (README screen 3, "isProcessing").
Item {
    id: root

    property string operationLabel: qsTr("Processing")
    property bool running: false

    anchors.fill: parent
    visible: running
    opacity: running ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
        opacity: 0.55
    }

    Column {
        anchors.centerIn: parent
        spacing: NeonTheme.paddingMedium

        Item {
            id: ring
            width: NeonTheme.px(96)
            height: NeonTheme.px(96)
            anchors.horizontalCenter: parent.horizontalCenter

            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.lineWidth = width * 0.08
                    ctx.strokeStyle = "rgba(255,255,255,0.15)"
                    ctx.beginPath()
                    ctx.arc(width / 2, height / 2, width / 2 - ctx.lineWidth, 0, Math.PI * 2)
                    ctx.stroke()
                }
            }

            Canvas {
                id: arc
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.lineWidth = width * 0.08
                    ctx.lineCap = "round"
                    ctx.strokeStyle = NeonTheme.accentGreen
                    ctx.beginPath()
                    ctx.arc(width / 2, height / 2, width / 2 - ctx.lineWidth, -Math.PI / 2, -Math.PI / 2 + Math.PI * 0.6)
                    ctx.stroke()
                }

                RotationAnimation on rotation {
                    running: root.running
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 900
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.operationLabel
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontDisplay
            font.weight: NeonTheme.fontWeightBold
            font.pixelSize: NeonTheme.fontSizeButton
        }
    }
}
