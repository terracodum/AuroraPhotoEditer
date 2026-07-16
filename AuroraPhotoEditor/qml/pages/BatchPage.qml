import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"
import "../components"

// Batch processing screen (README screen 8 / ISSUE-6.4). Nothing here has
// a backend yet: MOCK — the real implementation needs (a) a multi-select
// gallery picker (Sailfish.Pickers multi-select) instead of these
// placeholder tiles, and (b) a background BatchWorker that serializes the
// current command stack (pipelineManager.historySteps) to JSON and replays
// it per-file (ISSUE-6.4). runMockBatch() below simulates that worker's
// progress signal so the UI/UX is fully wired and ready to swap in.
Page {
    id: batchPage
    objectName: "batchPage"
    allowedOrientations: Orientation.All

    property int photoCount: 8
    property var selected: []
    property bool running: false
    property int processedCount: 0

    Component.onCompleted: {
        var arr = []
        for (var i = 0; i < photoCount; i++) arr.push(false)
        selected = arr
    }

    readonly property int selectedCount: selected.filter(function (s) { return s }).length
    readonly property bool canApply: selectedCount > 0 && pipelineManager.commandCount() > 0 && !running

    Timer {
        id: batchTimer
        interval: 260
        repeat: true
        onTriggered: {
            batchPage.processedCount++
            if (batchPage.processedCount >= batchPage.selectedCount) {
                batchTimer.stop()
                batchPage.running = false
            }
        }
    }

    function runMockBatch() {
        // MOCK (ISSUE-6.4): real BatchWorker runs in QThreadPool, applying
        // the serialized command stack to each selected file and reporting
        // real per-file progress instead of a fixed-interval Timer.
        processedCount = 0
        running = true
        batchTimer.start()
    }

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    PageHeaderBar {
        id: header
        title: qsTr("Пакетная обработка")
        showBack: true
        onBackClicked: pageStack.pop()
    }

    SilicaFlickable {
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: content.y + content.height + NeonTheme.paddingLarge
        clip: true

        Column {
            id: content
            width: parent.width - 2 * NeonTheme.paddingXLarge
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: NeonTheme.paddingLarge
            spacing: NeonTheme.paddingMedium

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: pipelineManager.commandCount() > 0
                      ? qsTr("В текущем проекте: %1 шаг(ов)").arg(pipelineManager.commandCount())
                      : qsTr("В текущем проекте нет шагов — сначала примените хотя бы один инструмент")
                color: NeonTheme.textSecondary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightMedium
                font.pixelSize: NeonTheme.fontSizeBodyLarge
            }

            Grid {
                width: parent.width
                columns: 4
                spacing: NeonTheme.paddingSmall

                Repeater {
                    model: batchPage.photoCount
                    delegate: Rectangle {
                        width: (content.width - 3 * NeonTheme.paddingSmall) / 4
                        height: width
                        radius: NeonTheme.radiusSmall
                        color: NeonTheme.bgCard

                        GlyphIcon {
                            anchors.centerIn: parent
                            name: "gallery"
                            width: parent.width * 0.4
                            height: parent.height * 0.4
                            strokeColor: NeonTheme.textTertiary
                        }

                        Rectangle {
                            visible: batchPage.selected[index] === true
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: NeonTheme.paddingTiny
                            width: NeonTheme.px(36)
                            height: NeonTheme.px(36)
                            radius: width / 2
                            color: NeonTheme.accentGreen

                            GlyphIcon {
                                anchors.centerIn: parent
                                name: "check"
                                width: parent.width * 0.6
                                height: parent.height * 0.6
                                strokeColor: NeonTheme.bgBase
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !batchPage.running
                            onClicked: {
                                var arr = batchPage.selected.slice()
                                arr[index] = !arr[index]
                                batchPage.selected = arr
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                visible: batchPage.running || batchPage.processedCount > 0
                height: NeonTheme.px(64)
                radius: NeonTheme.radiusSmall
                color: NeonTheme.bgChip

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    radius: parent.radius
                    color: NeonTheme.accentGreen
                    width: batchPage.selectedCount > 0
                           ? parent.width * (batchPage.processedCount / batchPage.selectedCount)
                           : 0
                    Behavior on width { NumberAnimation { duration: 150 } }
                }

                Text {
                    anchors.centerIn: parent
                    text: batchPage.running
                          ? qsTr("Обработано %1 из %2").arg(batchPage.processedCount).arg(batchPage.selectedCount)
                          : qsTr("Готово! Обработано %1 фото").arg(batchPage.processedCount)
                    color: NeonTheme.textPrimary
                    font.family: NeonTheme.fontBody
                    font.weight: NeonTheme.fontWeightBold
                    font.pixelSize: NeonTheme.fontSizeCaption
                }
            }
        }
    }

    Column {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: NeonTheme.paddingLarge
        spacing: NeonTheme.paddingSmall

        Text {
            text: qsTr("Выбрано: %1").arg(batchPage.selectedCount)
            color: NeonTheme.textSecondary
            font.family: NeonTheme.fontBody
            font.weight: NeonTheme.fontWeightMedium
            font.pixelSize: NeonTheme.fontSizeBody
        }

        GradientButton {
            width: parent.width
            enabled: batchPage.canApply
            text: qsTr("Применить проект")
            onClicked: batchPage.runMockBatch()
        }
    }
}
