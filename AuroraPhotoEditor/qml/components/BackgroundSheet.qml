import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// "Фон" tool bottom sheet. Segmentation itself (U2Net via MLInferenceEngine)
// is real and runs once per photo (PipelineManager::applyBackgroundRemoval());
// this sheet only controls how the cut-out subject is recomposited —
// solid color, two-color gradient, or blurred original background — all
// three backed 1:1 by PipelineManager::updateBackground(mode, c1, c2, blur),
// which recomputes the alpha blend from the cached mask without re-running
// inference. There is no "replace with a photo" backend yet, so that part
// of the original design mock was dropped rather than left as a fake button.
BottomSheet {
    id: root
    title: qsTr("Фон")

    // 0 = color, 1 = gradient, 2 = blur — matches BackgroundCommand::BackgroundMode.
    property int modeIndex: 2
    property color selectedColor1: "white"
    property color selectedColor2: "black"
    property int blurValue: 50
    // Which gradient endpoint the PalettePicker below is currently editing.
    property int editingSlot: 1

    function updateBg() {
        if (modeIndex === 0) {
            pipelineManager.updateBackground(0, selectedColor1, "transparent", 0)
        } else if (modeIndex === 1) {
            pipelineManager.updateBackground(1, selectedColor1, selectedColor2, 0)
        } else {
            pipelineManager.updateBackground(2, "transparent", "transparent", blurValue)
        }
    }

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
    }

    Text {
        width: parent.width
        text: qsTr("Режим")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    SegmentedControl {
        width: parent.width
        model: [qsTr("Цвет"), qsTr("Градиент"), qsTr("Блюр")]
        currentIndex: root.modeIndex
        onActivated: {
            root.modeIndex = index
            root.updateBg()
        }
    }

    // --- Цвет: single-color palette ---------------------------------
    PalettePicker {
        width: parent.width
        visible: root.modeIndex === 0
        color: root.selectedColor1
        onColorPicked: {
            root.selectedColor1 = pickedColor
            root.updateBg()
        }
    }

    // --- Градиент: two endpoints, edited via the same palette --------
    Column {
        width: parent.width
        visible: root.modeIndex === 1
        spacing: NeonTheme.paddingMedium

        Row {
            width: parent.width
            spacing: NeonTheme.paddingMedium

            Rectangle {
                width: NeonTheme.px(64)
                height: width
                radius: width / 2
                color: root.selectedColor1
                border.width: root.editingSlot === 1 ? 3 : 1
                border.color: root.editingSlot === 1 ? NeonTheme.accentPurple : Qt.rgba(1, 1, 1, 0.2)

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.editingSlot = 1
                }
            }

            GradientRect {
                width: parent.width - NeonTheme.px(64) * 2 - NeonTheme.paddingMedium * 2
                height: NeonTheme.px(64)
                radius: NeonTheme.radiusMedium
                anchors.verticalCenter: parent.verticalCenter
                stops: [
                    { position: 0.0, color: root.selectedColor1 },
                    { position: 1.0, color: root.selectedColor2 }
                ]
            }

            Rectangle {
                width: NeonTheme.px(64)
                height: width
                radius: width / 2
                color: root.selectedColor2
                border.width: root.editingSlot === 2 ? 3 : 1
                border.color: root.editingSlot === 2 ? NeonTheme.accentPurple : Qt.rgba(1, 1, 1, 0.2)

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.editingSlot = 2
                }
            }
        }

        Text {
            width: parent.width
            text: root.editingSlot === 1 ? qsTr("Редактируется: первый цвет") : qsTr("Редактируется: второй цвет")
            color: NeonTheme.textTertiary
            font.family: NeonTheme.fontBody
            font.pixelSize: NeonTheme.fontSizeCaption
        }

        PalettePicker {
            width: parent.width
            color: root.editingSlot === 1 ? root.selectedColor1 : root.selectedColor2
            onColorPicked: {
                if (root.editingSlot === 1)
                    root.selectedColor1 = pickedColor
                else
                    root.selectedColor2 = pickedColor
                root.updateBg()
            }
        }
    }

    // --- Блюр: intensity -------------------------------------------
    Column {
        width: parent.width
        visible: root.modeIndex === 2
        spacing: NeonTheme.paddingTiny

        Item {
            width: parent.width
            height: intensityLabel.height

            Text {
                id: intensityLabel
                anchors.left: parent.left
                text: qsTr("Интенсивность")
                color: NeonTheme.textSecondary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightMedium
                font.pixelSize: NeonTheme.fontSizeCaption
            }
            Text {
                anchors.right: parent.right
                text: root.blurValue
                color: NeonTheme.textTertiary
                font.family: NeonTheme.fontBody
                font.pixelSize: NeonTheme.fontSizeCaption
            }
        }

        GradientSlider {
            width: parent.width
            minimumValue: 0
            maximumValue: 100
            stepSize: 1
            value: root.blurValue
            onMoved: {
                root.blurValue = newValue
                root.updateBg()
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: NeonTheme.bgChip
    }

    Row {
        width: parent.width
        spacing: NeonTheme.paddingMedium

        Rectangle {
            width: (parent.width - NeonTheme.paddingMedium) / 2
            height: NeonTheme.px(88)
            radius: NeonTheme.radiusMedium
            color: NeonTheme.bgChip

            Text {
                anchors.centerIn: parent
                text: qsTr("Вернуться")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeButton
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    pipelineManager.undoLast()
                    root.hide()
                }
            }
        }

        Item {
            width: (parent.width - NeonTheme.paddingMedium) / 2
            height: NeonTheme.px(88)

            GradientRect {
                anchors.fill: parent
                radius: NeonTheme.radiusMedium
                stops: [
                    { position: 0.0, color: NeonTheme.gradientStart },
                    { position: 1.0, color: NeonTheme.gradientEnd }
                ]
            }

            Text {
                anchors.centerIn: parent
                text: qsTr("Применить")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeButton
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.hide()
            }
        }
    }
}
