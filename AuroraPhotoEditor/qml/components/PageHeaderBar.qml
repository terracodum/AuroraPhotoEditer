import QtQuick 2.0
import "../"

// Neon Pop header: "PhotoEditor" (or back-arrow + title) on the left,
// trailing icon buttons on the right. Height/typography from NeonTheme
// (README Design Tokens: "Высота хедера: 140px").
Item {
    id: bar

    property string title: ""
    property bool showBack: false
    default property alias trailingContent: trailingRow.data
    signal backClicked

    width: parent ? parent.width : NeonTheme.px(800)
    height: NeonTheme.headerHeight

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: NeonTheme.paddingXLarge
        anchors.verticalCenter: parent.verticalCenter
        spacing: NeonTheme.paddingMedium

        GlyphButton {
            visible: bar.showBack
            width: visible ? NeonTheme.iconButtonSize : 0
            iconName: "back"
            onClicked: bar.backClicked()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: bar.title
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontDisplay
            font.weight: NeonTheme.fontWeightDisplay
            font.pixelSize: NeonTheme.fontSizeH1
        }
    }

    Row {
        id: trailingRow
        anchors.right: parent.right
        anchors.rightMargin: NeonTheme.paddingXLarge
        anchors.verticalCenter: parent.verticalCenter
        spacing: NeonTheme.paddingSmall
    }
}
