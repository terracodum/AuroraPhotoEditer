import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    id: mainPage
    objectName: "mainPage"
    allowedOrientations: Orientation.All

    PageHeader {
        id: header
        objectName: "pageHeader"
        title: qsTr("PhotoEditor")
        extraContent.children: [
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingSmall
                
                IconButton {
                    objectName: "undoButton"
                    icon.source: "image://theme/icon-m-back"
                    enabled: pipelineManager.canUndo
                    onClicked: pipelineManager.undoLast()
                }
                IconButton {
                    objectName: "resetButton"
                    icon.source: "image://theme/icon-m-refresh"
                    enabled: pipelineManager.canUndo
                    onClicked: pipelineManager.resetToOriginal()
                }
                IconButton {
                    objectName: "saveButton"
                    icon.source: "image://theme/icon-m-save"
                    visible: pipelineManager.hasImage
                    onClicked: pipelineManager.exportImage()
                }
            }
        ]
    }

    SilicaFlickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true

        PullDownMenu {
            MenuItem {
                text: qsTr("Select Photo")
                onClicked: pageStack.push(imagePickerComponent)
            }
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }

        // Placeholder when no image is selected
        Column {
            id: placeholder
            anchors.centerIn: parent
            spacing: Theme.paddingLarge
            visible: !pipelineManager.hasImage
            width: parent.width - 2 * Theme.horizontalPageMargin

            Icon {
                source: "image://theme/icon-l-image"
                anchors.horizontalCenter: parent.horizontalCenter
                highlighted: true
            }

            Label {
                text: qsTr("No photo selected")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Select Photo")
                onClicked: pageStack.push(imagePickerComponent)
            }
        }

        // Selected Image View
        Image {
            id: selectedImage
            anchors.fill: parent
            anchors.margins: Theme.paddingLarge
            anchors.bottomMargin: toolsPanel.height + Theme.paddingLarge
            fillMode: Image.PreserveAspectFit
            visible: pipelineManager.hasImage
            opacity: visible ? 1.0 : 0.0
            cache: false // Prevent memory leaks from timestamp updates

            Behavior on opacity {
                FadeAnimation { duration: 400 }
            }
        }
        
        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: pipelineManager.isProcessing
            visible: pipelineManager.isProcessing
        }
    }

    DockedPanel {
        id: toolsPanel
        width: parent.width
        height: pipelineManager.canUndo ? Theme.itemSizeExtraLarge * 2 : Theme.itemSizeExtraLarge
        dock: Dock.Bottom
        open: pipelineManager.hasImage

        Rectangle {
            anchors.fill: parent
            color: Theme.overlayBackgroundColor

            Column {
                anchors.fill: parent
                spacing: Theme.paddingSmall
                
                Item {
                    width: parent.width
                    height: Theme.paddingMedium
                }

                Slider {
                    width: parent.width
                    label: qsTr("Сила фильтра")
                    value: pipelineManager.filterStrength
                    minimumValue: 0.0
                    maximumValue: 1.0
                    stepSize: 0.01
                    valueText: Math.round(value * 100) + "%"
                    onValueChanged: {
                        if (pipelineManager.filterStrength !== value) {
                            pipelineManager.filterStrength = value
                        }
                    }
                    visible: pipelineManager.canUndo
                }

                Row {
                    width: parent.width - Theme.paddingMedium * 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingMedium

                    Button {
                        width: (parent.width - Theme.paddingMedium * 2) / 3
                        text: qsTr("Фон")
                        onClicked: pipelineManager.applyBackgroundRemoval()
                    }
                    Button {
                        width: (parent.width - Theme.paddingMedium * 2) / 3
                        text: qsTr("Улучшение")
                        onClicked: pipelineManager.applyEnhance()
                    }
                    Button {
                        width: (parent.width - Theme.paddingMedium * 2) / 3
                        text: qsTr("Стиль")
                        onClicked: pageStack.push(styleSelectionComponent)
                    }
                }
            }
        }
    }

    Connections {
        target: pipelineManager
        onCurrentImageChanged: {
            if (pipelineManager.hasImage) {
                selectedImage.source = "image://pipeline/current?t=" + Date.now()
            } else {
                selectedImage.source = undefined
            }
        }
        
        onExportCompleted: {
            if (success) {
                pageStack.push(Qt.resolvedUrl("NoticePage.qml"), { "message": qsTr("Image successfully saved to:\n") + filePath })
            } else {
                pageStack.push(Qt.resolvedUrl("NoticePage.qml"), { "message": qsTr("Failed to save image") })
            }
        }
    }

    Component {
        id: imagePickerComponent
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties.filePath) {
                    pipelineManager.loadFromUri(selectedContentProperties.filePath)
                }
            }
        }
    }
    Component {
        id: styleSelectionComponent
        Page {
            allowedOrientations: Orientation.All
            SilicaListView {
                anchors.fill: parent
                header: PageHeader { title: qsTr("Выбрать стиль") }
                model: pipelineManager.getAvailableStyles()
                delegate: BackgroundItem {
                    id: delegate
                    Label {
                        x: Theme.horizontalPageMargin
                        text: modelData.name
                        anchors.verticalCenter: parent.verticalCenter
                        color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    onClicked: {
                        console.log("Applying style: " + modelData.file)
                        pipelineManager.applyStyle(modelData.file)
                        pageStack.pop()
                    }
                }
            }
        }
    }
}
