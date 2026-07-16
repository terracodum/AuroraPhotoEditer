import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"
import "../components"

// Fullscreen manual mask edit (brush/eraser), README screen 6. The C++ side
// (ISSUE-6.2: touch coordinates -> PipelineManager.modifyMask(x, y, radius,
// isEraser) against the cached full-resolution alpha mask, then an instant
// Alpha Blending repaint) does not exist yet. MOCK (ISSUE-6.2): strokes are
// drawn on a local Canvas overlay purely for visual feedback — swap
// onStrokePoint() for the real modifyMask() call once PipelineManager
// exposes it, and drop this Canvas layer (the repainted image will already
// contain the edited mask).
Page {
    id: maskPage
    objectName: "maskEditPage"
    allowedOrientations: Orientation.All

    property int toolIndex: 0 // 0 = brush, 1 = eraser
    property real brushSize: 44

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    PageHeaderBar {
        id: header
        title: qsTr("Правка маски")

        SegmentedControl {
            width: NeonTheme.px(260)
            model: [qsTr("Кисть"), qsTr("Ластик")]
            currentIndex: maskPage.toolIndex
            onActivated: maskPage.toolIndex = index
        }
    }

    Item {
        id: canvasArea
        anchors.top: header.bottom
        anchors.bottom: brushSizeRow.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: NeonTheme.paddingLarge

        Image {
            id: photo
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            cache: false
            source: pipelineManager.hasImage ? "image://pipeline/current?t=" + Date.now() : ""
        }

        Canvas {
            id: strokeCanvas
            anchors.fill: parent
            renderTarget: Canvas.FramebufferObject

            function onStrokePoint(x, y) {
                // MOCK (ISSUE-6.2): real implementation calls
                // pipelineManager.modifyMask(x, y, maskPage.brushSize,
                // maskPage.toolIndex === 1) and lets the image provider
                // repaint; this local Canvas just visualizes the stroke.
                var ctx = getContext("2d")
                ctx.beginPath()
                ctx.fillStyle = maskPage.toolIndex === 1
                    ? Qt.rgba(0.89, 0.15, 0.37, 0.35)
                    : Qt.rgba(0.55, 0.36, 0.96, 0.35)
                ctx.arc(x, y, maskPage.brushSize / 2, 0, Math.PI * 2)
                ctx.fill()
                requestPaint()
            }

            MouseArea {
                anchors.fill: parent
                onPositionChanged: strokeCanvas.onStrokePoint(mouse.x, mouse.y)
                onPressed: strokeCanvas.onStrokePoint(mouse.x, mouse.y)
            }
        }

        Text {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Проведите пальцем по фото")
            color: NeonTheme.textTertiary
            font.family: NeonTheme.fontBody
            font.pixelSize: NeonTheme.fontSizeCaption
        }
    }

    Column {
        id: brushSizeRow
        width: parent.width - 2 * NeonTheme.paddingLarge
        anchors.left: parent.left
        anchors.leftMargin: NeonTheme.paddingLarge
        anchors.bottom: footer.top
        anchors.bottomMargin: NeonTheme.paddingMedium
        spacing: NeonTheme.paddingTiny

        Text {
            text: qsTr("Размер кисти")
            color: NeonTheme.textSecondary
            font.family: NeonTheme.fontBody
            font.weight: NeonTheme.fontWeightMedium
            font.pixelSize: NeonTheme.fontSizeCaption
        }
        Slider {
            width: parent.width
            minimumValue: 12
            maximumValue: 140
            value: maskPage.brushSize
            onValueChanged: maskPage.brushSize = value
        }
    }

    Row {
        id: footer
        anchors.bottom: parent.bottom
        anchors.margins: NeonTheme.paddingLarge
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: NeonTheme.paddingMedium

        Rectangle {
            width: (parent.width - parent.spacing) / 2
            height: NeonTheme.ctaHeight
            radius: NeonTheme.radiusMedium
            color: NeonTheme.bgChip

            Text {
                anchors.centerIn: parent
                text: qsTr("Отмена")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeButton
            }

            MouseArea {
                anchors.fill: parent
                onClicked: pageStack.pop()
            }
        }

        GradientButton {
            width: (parent.width - parent.spacing) / 2
            text: qsTr("Готово")
            onClicked: {
                // MOCK (ISSUE-6.2): real impl recomputes Alpha Blending
                // server-side and pushes a new history step; here we just
                // return to MainPage.
                pageStack.pop()
            }
        }
    }
}
