import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// "Стиль" tool bottom sheet (README screen 5). Neural style transfer
// (ISSUE-5.1/5.2, ISSUE-6.3) is not implemented in the C++ backend yet, so
// style selection below is a UI-only mock: MOCK (ISSUE-5.1) — swap
// runMockInference() for pipelineManager.applyStyle(styleId) once the ONNX
// model + BackgroundReplaceCommand-style composite exists. The debounce
// (450ms, ISSUE-5.2) and the "strength" slider not re-triggering inference
// (ISSUE-6.3: Result = Original*(1-Opacity) + Styled*Opacity) are real
// behavior the mock demonstrates so the eventual backend wiring is a
// drop-in swap of one function body.
BottomSheet {
    id: root
    title: qsTr("Стиль")

    readonly property var styles: [
        { id: "vivid", name: qsTr("Vivid"), c1: "#8b5cf6", c2: "#ec4899" },
        { id: "mono", name: qsTr("Mono"), c1: "#6d5c8f", c2: "#241b38" },
        { id: "film", name: qsTr("Film"), c1: "#e2265f", c2: "#8b5cf6" },
        { id: "vangogh", name: qsTr("Ван Гог"), c1: "#22e5a0", c2: "#8b5cf6" },
        { id: "ukiyoe", name: qsTr("Укиё-э"), c1: "#ec4899", c2: "#22e5a0" }
    ]

    property int selectedIndex: -1
    property int pendingIndex: -1
    property bool busy: false
    property real strength: 1.0

    Timer {
        id: debounceTimer
        interval: NeonTheme.debounceStyleMs
        onTriggered: root.runMockInference(root.pendingIndex)
    }

    Timer {
        id: mockInferenceDelay
        interval: 500
        onTriggered: {
            root.selectedIndex = root.pendingIndex
            root.busy = false
        }
    }

    function selectStyle(index) {
        // Only the *last* fast tap survives — matches ISSUE-5.2 acceptance
        // criteria ("предыдущие отменяются").
        pendingIndex = index
        debounceTimer.restart()
    }

    function runMockInference(index) {
        busy = true
        mockInferenceDelay.restart()
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
        text: qsTr("Превью стилей")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    ListView {
        width: parent.width
        height: NeonTheme.px(150)
        orientation: ListView.Horizontal
        spacing: NeonTheme.paddingSmall
        model: root.styles
        clip: true

        delegate: Rectangle {
            width: NeonTheme.px(150)
            height: NeonTheme.px(150)
            radius: NeonTheme.radiusMedium
            border.width: root.selectedIndex === index ? 3 : 0
            border.color: NeonTheme.accentGreen

            gradient: Gradient {
                GradientStop { position: 0.0; color: modelData.c1 }
                GradientStop { position: 1.0; color: modelData.c2 }
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
                onClicked: root.selectStyle(index)
            }
        }
    }

    Text {
        width: parent.width
        text: qsTr("Сила эффекта")
        color: NeonTheme.textSecondary
        font.family: NeonTheme.fontBody
        font.weight: NeonTheme.fontWeightMedium
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    Slider {
        width: parent.width
        minimumValue: 0.0
        maximumValue: 1.0
        value: root.strength
        // Alpha-blend-only recompute (ISSUE-6.3) — intentionally does not
        // touch `busy` / re-run runMockInference().
        onValueChanged: root.strength = value
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Ползунок меняет только смешивание — без повторного запуска нейросети")
        color: NeonTheme.textTertiary
        font.family: NeonTheme.fontBody
        font.pixelSize: NeonTheme.fontSizeCaption
    }
}
