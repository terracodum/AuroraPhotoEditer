import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: batchPage
    objectName: "batchPage"
    allowedOrientations: Orientation.All

    property var selectedImages: []
    property string currentProjectCommands: ""

    Component.onCompleted: {
        // Load the current command stack from PipelineManager as the "project"
        if (pipelineManager.commandCount > 0) {
            currentProjectCommands = pipelineManager.serializeCommandStack()
        }
        
        pipelineManager.batchProgress.connect(onBatchProgress)
        pipelineManager.batchFinished.connect(onBatchFinished)
    }

    Component.onDestruction: {
        pipelineManager.batchProgress.disconnect(onBatchProgress)
        pipelineManager.batchFinished.disconnect(onBatchFinished)
    }

    function onBatchProgress(current, total) {
        progressBar.value = current
        progressBar.maximumValue = total
        progressBar.label = "Обработано " + current + " из " + total
    }

    function onBatchFinished(successCount, failCount) {
        progressBar.visible = false
        statusLabel.text = "Готово!\nУспешно: " + successCount + "\nОшибок: " + failCount
        statusLabel.visible = true
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Пакетная обработка")
            }

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                text: "Текущий проект содержит шагов: " + pipelineManager.commandCount
                color: Theme.highlightColor
                wrapMode: Text.WordWrap
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Выбрать фото (Multi-select)")
                enabled: !pipelineManager.isProcessing && pipelineManager.commandCount > 0
                onClicked: {
                    var picker = pageStack.push("Sailfish.Pickers.MultiImagePickerPage")
                    picker.selectedContentPropertiesChanged.connect(function() {
                        var paths = []
                        for (var i = 0; i < picker.selectedContentProperties.length; ++i) {
                            paths.push(picker.selectedContentProperties[i].filePath)
                        }
                        selectedImages = paths
                        selectedLabel.text = "Выбрано фото: " + selectedImages.length
                    })
                }
            }
            
            Label {
                id: selectedLabel
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Выбрано фото: 0"
                color: Theme.secondaryColor
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Запустить обработку")
                enabled: selectedImages.length > 0 && !pipelineManager.isProcessing && currentProjectCommands !== ""
                onClicked: {
                    progressBar.visible = true
                    statusLabel.visible = false
                    pipelineManager.startBatchProcessing(selectedImages, currentProjectCommands)
                }
            }
            
            ProgressBar {
                id: progressBar
                width: parent.width - Theme.horizontalPageMargin * 2
                anchors.horizontalCenter: parent.horizontalCenter
                visible: false
            }
            
            Label {
                id: statusLabel
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                visible: false
                color: Theme.highlightColor
            }
        }
    }
}
