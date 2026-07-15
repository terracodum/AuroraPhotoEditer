import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

// History panel (README screen 7). The step list itself is REAL data —
// pipelineManager.historySteps mirrors the same command stack Undo/Reset
// already operate on (PipelineManager::historySteps(), added alongside
// this UI). Only "Сохранить как проект" is mocked: MOCK (ISSUE-6.4) — the
// JSON/SQLite command-stack serialization + Batch integration doesn't
// exist in the backend yet, so this just emits saveProjectRequested() for
// the host page to show a toast.
BottomSheet {
    id: root
    title: qsTr("История изменений")

    signal saveProjectRequested

    readonly property var reversedSteps: {
        var steps = pipelineManager.historySteps
        var out = []
        for (var i = steps.length - 1; i >= 0; i--) {
            out.push(steps[i])
        }
        return out
    }

    function subtitleFor(stepName) {
        // UI-only technical caption; extend this map as real commands
        // (Background/Style) land — the underlying step data is real.
        switch (stepName) {
        case "Enhance": return qsTr("CLAHE + авто баланс белого")
        default: return qsTr("Применённый шаг")
        }
    }

    function dotColorFor(stepName) {
        switch (stepName) {
        case "Enhance": return NeonTheme.accentGreen
        default: return NeonTheme.accentPurple
        }
    }

    Text {
        width: parent.width
        visible: root.reversedSteps.length === 0
        text: qsTr("Пока нет применённых шагов")
        color: NeonTheme.textTertiary
        font.family: NeonTheme.fontBody
        font.pixelSize: NeonTheme.fontSizeBody
        horizontalAlignment: Text.AlignHCenter
    }

    Column {
        width: parent.width
        visible: root.reversedSteps.length > 0
        spacing: NeonTheme.paddingSmall

        Repeater {
            model: root.reversedSteps
            delegate: Rectangle {
                width: parent.width
                height: NeonTheme.px(96)
                radius: NeonTheme.radiusSmall
                color: NeonTheme.bgChip

                Row {
                    anchors.fill: parent
                    anchors.margins: NeonTheme.paddingMedium
                    spacing: NeonTheme.paddingSmall

                    Rectangle {
                        width: NeonTheme.px(14)
                        height: width
                        radius: width / 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.dotColorFor(modelData.name)
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: NeonTheme.px(4)

                        Text {
                            text: modelData.name
                            color: NeonTheme.textPrimary
                            font.family: NeonTheme.fontBody
                            font.weight: NeonTheme.fontWeightBold
                            font.pixelSize: NeonTheme.fontSizeBody
                        }
                        Text {
                            text: root.subtitleFor(modelData.name)
                            color: NeonTheme.textTertiary
                            font.family: NeonTheme.fontBody
                            font.pixelSize: NeonTheme.fontSizeCaption
                        }
                    }
                }

                Rectangle {
                    visible: index === 0
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: NeonTheme.paddingSmall
                    radius: NeonTheme.radiusPill
                    color: NeonTheme.accentGreen
                    width: currentLabel.implicitWidth + NeonTheme.paddingSmall
                    height: currentLabel.implicitHeight + NeonTheme.paddingTiny

                    Text {
                        id: currentLabel
                        anchors.centerIn: parent
                        text: qsTr("Текущий")
                        color: NeonTheme.bgBase
                        font.family: NeonTheme.fontBody
                        font.weight: NeonTheme.fontWeightBold
                        font.pixelSize: NeonTheme.px(18)
                    }
                }
            }
        }
    }

    Text {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("Отмена доступна только для последнего шага.")
        color: NeonTheme.textTertiary
        font.family: NeonTheme.fontBody
        font.pixelSize: NeonTheme.fontSizeCaption
    }

    Rectangle {
        width: parent.width
        height: NeonTheme.ctaHeight
        radius: NeonTheme.radiusPill
        enabled: root.reversedSteps.length > 0
        opacity: enabled ? 1.0 : 0.4
        gradient: Gradient {
            GradientStop { position: 0.0; color: NeonTheme.gradientStart }
            GradientStop { position: 1.0; color: NeonTheme.gradientEnd }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Сохранить как проект")
            color: NeonTheme.textPrimary
            font.family: NeonTheme.fontDisplay
            font.weight: NeonTheme.fontWeightBold
            font.pixelSize: NeonTheme.fontSizeButton
        }

        MouseArea {
            anchors.fill: parent
            enabled: parent.enabled
            onClicked: {
                // MOCK (ISSUE-6.4): no real serialization yet.
                root.saveProjectRequested()
                root.hide()
            }
        }
    }
}
