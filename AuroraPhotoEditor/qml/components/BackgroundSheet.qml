import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// "Фон" tool bottom sheet. Segmentation itself (U2Net via MLInferenceEngine)
// is real and runs once per photo (PipelineManager::applyBackgroundRemoval());
// this sheet only controls how the cut-out subject is recomposited — solid
// color, two-color gradient, blurred original background, or a custom
// picked photo — all four backed 1:1 by
// PipelineManager::updateBackground(mode, c1, c2, blur, imageUri), which
// recomputes the alpha blend from the cached mask without re-running
// inference (mode 3 = ModeCustomImage).
BottomSheet {
    id: root
    title: qsTr("Фон")

    // Picking a photo needs Sailfish.Pickers' ImagePickerPage, which has to
    // be pushed via pageStack from MainPage.qml (a plain Item/DockedPanel
    // like this one can't see MainPage's own `id`s) — so this just asks and
    // lets onPickCustomImageRequested do the actual pageStack.push().
    signal pickCustomImageRequested

    // 0 = color, 1 = gradient, 2 = blur, 3 = custom image — matches
    // BackgroundCommand::BackgroundMode.
    property int modeIndex: 2
    property color selectedColor1: "white"
    property color selectedColor2: "black"
    property int blurValue: 50
    property string customImagePath: ""
    // Which gradient endpoint the PalettePicker below is currently editing.
    property int editingSlot: 1

    function updateBg() {
        if (modeIndex === 0) {
            pipelineManager.updateBackground(0, selectedColor1, "transparent", 0)
        } else if (modeIndex === 1) {
            pipelineManager.updateBackground(1, selectedColor1, selectedColor2, 0)
        } else if (modeIndex === 2) {
            pipelineManager.updateBackground(2, "transparent", "transparent", blurValue)
        } else if (customImagePath !== "") {
            pipelineManager.updateBackground(3, "transparent", "transparent", 0, customImagePath)
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
        model: [qsTr("Цвет"), qsTr("Градиент"), qsTr("Блюр"), qsTr("Фото")]
        currentIndex: root.modeIndex
        onActivated: {
            root.modeIndex = index
            root.updateBg()
        }
    }

    // --- Цвет: single-color palette ---------------------------------
    PalettePicker {
        width: parent.width
        // Cheap opacity crossfade between modes: fade out first, then
        // collapse out of the Column layout only once fully transparent
        // (rather than an instant `visible` swap), so switching modes
        // doesn't jump. No extra repaint cost — plain compositor opacity.
        opacity: root.modeIndex === 0 ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        color: root.selectedColor1
        onColorPicked: {
            root.selectedColor1 = pickedColor
            root.updateBg()
        }
    }

    // --- Градиент: two endpoints, edited via the same palette --------
    Column {
        width: parent.width
        opacity: root.modeIndex === 1 ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
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
        opacity: root.modeIndex === 2 ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
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

    // --- Фото: replace the background with a picked photo -----------
    Column {
        width: parent.width
        opacity: root.modeIndex === 3 ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        spacing: NeonTheme.paddingMedium

        Text {
            width: parent.width
            text: root.customImagePath === "" ? qsTr("Фото не выбрано") : qsTr("Фото выбрано")
            color: NeonTheme.textSecondary
            font.family: NeonTheme.fontBody
            font.pixelSize: NeonTheme.fontSizeCaption
        }

        Rectangle {
            width: parent.width
            height: NeonTheme.px(88)
            radius: NeonTheme.radiusMedium
            color: NeonTheme.bgChip

            Row {
                anchors.centerIn: parent
                spacing: NeonTheme.paddingSmall

                GlyphIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "image"
                    width: NeonTheme.px(32)
                    height: NeonTheme.px(32)
                    strokeColor: NeonTheme.accentPurple
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Выбрать из галереи")
                    color: NeonTheme.textPrimary
                    font.family: NeonTheme.fontBody
                    font.weight: NeonTheme.fontWeightMedium
                    font.pixelSize: NeonTheme.fontSizeBody
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.pickCustomImageRequested()
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
