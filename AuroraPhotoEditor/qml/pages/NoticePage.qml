import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"
import "../components"

// Success/error result screen (README screen 10). `success` controls the
// icon + accent color; MainPage passes it when pushing (export result).
Page {
    id: noticePage
    allowedOrientations: Orientation.All

    property string message: "Success"
    property bool success: true

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * NeonTheme.paddingXXLarge
        spacing: NeonTheme.paddingLarge

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: NeonTheme.px(160)
            height: NeonTheme.px(160)
            radius: width / 2
            color: noticePage.success ? NeonTheme.successColor : NeonTheme.errorColor

            GlyphIcon {
                anchors.centerIn: parent
                name: noticePage.success ? "check" : "warning"
                width: parent.width * 0.45
                height: parent.height * 0.45
                strokeColor: NeonTheme.bgBase
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            text: noticePage.message
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontBody
            font.weight: NeonTheme.fontWeightBold
            font.pixelSize: NeonTheme.fontSizeBodyLarge
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        GradientButton {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, NeonTheme.px(420))
            text: qsTr("OK")
            onClicked: pageStack.pop()
        }
    }
}
