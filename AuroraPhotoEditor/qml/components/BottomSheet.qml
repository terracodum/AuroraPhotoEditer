import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// Shared bottom-sheet chrome for the "Фон"/"Стиль"/История tool panels
// (per the task brief: Silica's DockedPanel instead of a custom Rectangle
// bottom sheet). Host pages set `title` and put controls in the default
// `content` Column; call show()/hide() to toggle. Note: DockedPanel already
// has a native `open` property, so the convenience methods here are named
// show()/hide() to avoid shadowing it.
DockedPanel {
    id: root

    dock: Dock.Bottom
    width: parent ? parent.width : NeonTheme.px(800)

    property string title: ""
    default property alias content: bodyColumn.data

    signal closed

    height: Math.min(
        (parent ? parent.height : NeonTheme.px(1200)) * 0.88,
        headerItem.height + bodyColumn.height + NeonTheme.paddingXLarge * 2 + NeonTheme.paddingMedium)

    function show() { root.open = true }
    function hide() { root.open = false }

    onOpenChanged: if (!open) root.closed()

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgSurface
        radius: NeonTheme.radiusLarge
    }

    SilicaFlickable {
        anchors.fill: parent
        anchors.margins: NeonTheme.paddingXLarge
        contentHeight: headerItem.height + bodyColumn.height + NeonTheme.paddingMedium
        clip: true

        Item {
            id: headerItem
            width: parent.width
            height: NeonTheme.px(64)

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeCardTitle
            }

            GlyphButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconName: "close"
                onClicked: root.hide()
            }
        }

        Column {
            id: bodyColumn
            width: parent.width
            anchors.top: headerItem.bottom
            anchors.topMargin: NeonTheme.paddingMedium
            spacing: NeonTheme.paddingMedium
        }
    }
}
