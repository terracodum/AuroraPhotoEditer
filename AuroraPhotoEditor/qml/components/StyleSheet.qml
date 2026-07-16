import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// "Стиль" tool bottom sheet. Style transfer is real
// (PipelineManager::applyStyle / StyleTransferCommand, one ONNX model per
// style, real on-device inference — BusyOverlay covers the wait). The list
// of styles isn't hardcoded here: it's discovered at runtime from
// data/models/*.onnx (PipelineManager::getAvailableStyles(), which only
// keeps 3-channel/RGB-output models — segmentation models are 1-channel
// masks and get filtered out on the C++ side). Tile colors below are just
// a small rotating decorative palette, since style names/count aren't
// known ahead of time. The post-apply "Сила эффекта" blend is the shared
// pipelineManager.filterStrength control in the main toolbar (applies to
// whichever effect is on top of the stack), not duplicated in this sheet.
BottomSheet {
    id: root
    title: qsTr("Стиль")

    signal styleSelected(string file)

    readonly property var tileGradients: [
        ["#8b5cf6", "#ec4899"],
        ["#22e5a0", "#8b5cf6"],
        ["#ec4899", "#22e5a0"],
        ["#6d5c8f", "#241b38"],
        ["#e2265f", "#8b5cf6"]
    ]

    property var styles: []

    function refreshStyles() {
        styles = pipelineManager.getAvailableStyles()
    }

    onOpenChanged: if (open) refreshStyles()

    Row {
        width: parent.width
        spacing: NeonTheme.paddingSmall

        Rectangle {
            width: NeonTheme.px(14)
            height: width
            radius: width / 2
            color: NeonTheme.accentGreen
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("ML-модель · on-device · ONNX Runtime")
            color: NeonTheme.textTertiary
            font.family: NeonTheme.fontBody
            font.pixelSize: NeonTheme.fontSizeCaption
        }
        BusyIndicator {
            anchors.verticalCenter: parent.verticalCenter
            running: pipelineManager.isProcessing
            visible: pipelineManager.isProcessing
            size: BusyIndicatorSize.Small
        }
    }

    Text {
        width: parent.width
        text: qsTr("Превью стилей")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    Text {
        width: parent.width
        visible: root.styles.length === 0
        wrapMode: Text.WordWrap
        text: qsTr("Нет доступных моделей стиля")
        color: NeonTheme.textTertiary
        font.family: NeonTheme.fontBody
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    ListView {
        width: parent.width
        height: NeonTheme.px(150)
        visible: root.styles.length > 0
        orientation: ListView.Horizontal
        spacing: NeonTheme.paddingSmall
        model: root.styles
        clip: true

        delegate: Rectangle {
            width: NeonTheme.px(150)
            height: NeonTheme.px(150)
            radius: NeonTheme.radiusMedium

            gradient: Gradient {
                GradientStop { position: 0.0; color: root.tileGradients[index % root.tileGradients.length][0] }
                GradientStop { position: 1.0; color: root.tileGradients[index % root.tileGradients.length][1] }
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: NeonTheme.paddingSmall
                anchors.horizontalCenter: parent.horizontalCenter
                text: modelData.name
                color: "#ffffff"
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.px(19)
                style: Text.Outline
                styleColor: Qt.rgba(0, 0, 0, 0.4)
            }

            MouseArea {
                anchors.fill: parent
                enabled: !pipelineManager.isProcessing
                onClicked: {
                    root.styleSelected(modelData.file)
                    root.hide()
                }
            }
        }
    }
}
