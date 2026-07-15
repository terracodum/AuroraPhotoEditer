import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"
import "../components"

Page {
    objectName: "aboutPage"
    allowedOrientations: Orientation.All

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    SilicaFlickable {
        objectName: "flickable"
        anchors.fill: parent
        contentHeight: layout.height + NeonTheme.paddingXLarge

        Column {
            id: layout
            objectName: "layout"
            width: parent.width
            spacing: NeonTheme.paddingLarge

            PageHeaderBar {
                title: qsTr("О приложении")
                showBack: true
                onBackClicked: pageStack.pop()
            }

            Column {
                width: parent.width
                spacing: NeonTheme.paddingSmall

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: NeonTheme.px(110)
                    height: NeonTheme.px(110)
                    radius: NeonTheme.radiusMedium
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: NeonTheme.gradientStart }
                        GradientStop { position: 1.0; color: NeonTheme.gradientEnd }
                    }

                    GlyphIcon {
                        anchors.centerIn: parent
                        name: "spark"
                        width: parent.width * 0.5
                        height: parent.height * 0.5
                        strokeColor: NeonTheme.textPrimary
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "AuroraPhotoEditor"
                    color: NeonTheme.textPrimary
                    font.family: NeonTheme.fontDisplay
                    font.weight: NeonTheme.fontWeightBold
                    font.pixelSize: NeonTheme.fontSizeCardTitle
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    // TODO: source from a build-time context property instead
                    // of hardcoding, once one is exposed to QML.
                    text: qsTr("Версия 0.1")
                    color: NeonTheme.textTertiary
                    font.family: NeonTheme.fontBody
                    font.pixelSize: NeonTheme.fontSizeCaption
                }
            }

            Label {
                objectName: "descriptionText"
                anchors { left: parent.left; right: parent.right; margins: NeonTheme.paddingXLarge }
                color: NeonTheme.textSecondary
                font.family: NeonTheme.fontBody
                font.pixelSize: NeonTheme.fontSizeBody
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                text: qsTr("#descriptionText")
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; margins: NeonTheme.paddingXLarge }
                height: 1
                color: NeonTheme.bgChip
            }

            Text {
                objectName: "licenseHeader"
                anchors { left: parent.left; right: parent.right; margins: NeonTheme.paddingXLarge }
                text: qsTr("3-Clause BSD License")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeBody
            }

            Label {
                objectName: "licenseText"
                anchors { left: parent.left; right: parent.right; margins: NeonTheme.paddingXLarge }
                color: NeonTheme.textTertiary
                font.family: NeonTheme.fontBody
                font.pixelSize: NeonTheme.fontSizeCaption
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                text: qsTr("#licenseText")
            }
        }
    }
}
