import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../"

// "Фон" tool bottom sheet (README screen 4). The real ML segmentation
// (BackgroundReplaceCommand / ISSUE-4.1-4.3, ISSUE-6.1) is not implemented
// in the C++ backend yet, so every action below is a UI-only mock behind a
// short fake delay (`busy`), clearly marked "MOCK (ISSUE-4.x)" — swap the
// body of applyPreset()/applyCustomBackground() for a real
// pipelineManager.applyBackgroundXxx(...) call once that command exists.
// The intensity slider intentionally does NOT re-trigger `busy`: per
// ISSUE-4.3, moving it should only recompute Alpha Blending from a cached
// mask, never re-run inference.
BottomSheet {
    id: root
    title: qsTr("Фон")

    property bool busy: false
    signal maskEditRequested

    property int modeIndex: 0
    property real intensity: 0.5

    Timer {
        id: mockDelay
        interval: 550
        onTriggered: root.busy = false
    }

    function applyPreset(presetName) {
        // MOCK (ISSUE-4.1/4.3): no real segmentation model wired up yet.
        root.busy = true
        mockDelay.restart()
    }

    function applyCustomBackground(filePath) {
        // MOCK (ISSUE-6.1): real impl crops/centers the picked photo
        // (Aspect Fill) in C++ and re-runs Alpha Blending against the
        // cached mask.
        root.busy = true
        mockDelay.restart()
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
        BusyIndicator {
            anchors.verticalCenter: parent.verticalCenter
            running: root.busy
            visible: root.busy
            size: BusyIndicatorSize.Small
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
        model: [qsTr("Удалить"), qsTr("Размыть"), qsTr("Заменить")]
        currentIndex: root.modeIndex
        onActivated: {
            root.modeIndex = index
            root.applyPreset("mode-" + index)
        }
    }

    Text {
        width: parent.width
        text: qsTr("Интенсивность")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    Slider {
        width: parent.width
        minimumValue: 0.0
        maximumValue: 1.0
        value: root.intensity
        // Alpha-blend-only recompute — never touches `busy` (no re-inference).
        onValueChanged: root.intensity = value
    }

    Text {
        width: parent.width
        text: qsTr("Пресеты")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    Flow {
        width: parent.width
        spacing: NeonTheme.paddingSmall

        Repeater {
            model: [qsTr("Портрет"), qsTr("Студия"), qsTr("Улица")]
            delegate: Rectangle {
                height: NeonTheme.px(64)
                width: presetLabel.implicitWidth + NeonTheme.paddingXLarge
                radius: NeonTheme.radiusPill
                color: NeonTheme.bgChip

                Text {
                    id: presetLabel
                    anchors.centerIn: parent
                    text: modelData
                    color: NeonTheme.textPrimary
                    font.family: NeonTheme.fontBody
                    font.weight: NeonTheme.fontWeightMedium
                    font.pixelSize: NeonTheme.fontSizeCaption
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.applyPreset(modelData)
                }
            }
        }

        Rectangle {
            height: NeonTheme.px(64)
            width: customLabel.implicitWidth + NeonTheme.paddingXLarge
            radius: NeonTheme.radiusPill
            gradient: Gradient {
                GradientStop { position: 0.0; color: NeonTheme.gradientStart }
                GradientStop { position: 1.0; color: NeonTheme.gradientEnd }
            }

            Text {
                id: customLabel
                anchors.centerIn: parent
                text: qsTr("Своё фото из галереи")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeCaption
            }

            MouseArea {
                anchors.fill: parent
                onClicked: pageStack.push(customBackgroundPicker)
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: NeonTheme.bgChip
    }

    Text {
        width: parent.width
        text: qsTr("Точная правка маски")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
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
                name: "brush"
                width: NeonTheme.px(32)
                height: NeonTheme.px(32)
                strokeColor: NeonTheme.accentPurple
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Кисть/ластик — подправить вручную")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightMedium
                font.pixelSize: NeonTheme.fontSizeBody
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.maskEditRequested()
        }
    }

    Rectangle {
        width: parent.width
        height: NeonTheme.px(96)
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
            onClicked: root.hide()
        }
    }

    Component {
        id: customBackgroundPicker
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties.filePath) {
                    root.applyCustomBackground(selectedContentProperties.filePath)
                    pageStack.pop()
                }
            }
        }
    }
}
