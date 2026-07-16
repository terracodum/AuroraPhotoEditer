import QtQuick 2.0
import "../"

// Equal-width segment control used for "Режим" (Фон) and Кисть/Ластик
// toggle (mask edit). Active segment gets an accentPurple fill.
Row {
    id: root

    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    height: NeonTheme.px(64)
    spacing: NeonTheme.paddingTiny / 2

    Repeater {
        model: root.model
        delegate: Rectangle {
            width: (root.width - (root.model.length - 1) * root.spacing) / root.model.length
            height: root.height
            radius: NeonTheme.radiusSmall
            color: index === root.currentIndex ? NeonTheme.accentPurple : NeonTheme.bgChip

            Text {
                anchors.centerIn: parent
                text: modelData
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightMedium
                font.pixelSize: NeonTheme.fontSizeCaption
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = index
                    root.activated(index)
                }
            }
        }
    }
}
