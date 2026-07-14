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
                    icon.source: "image://theme/icon-m-undo"
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
                IconButton {
                    objectName: "aboutButton"
                    icon.source: "image://theme/icon-m-about"
                    onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
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
    }

    DockedPanel {
        id: toolsPanel
        width: parent.width
        height: Theme.itemSizeExtraLarge
        dock: Dock.Bottom
        open: pipelineManager.hasImage

        Rectangle {
            anchors.fill: parent
            color: Theme.overlayBackgroundColor

            Row {
                anchors.centerIn: parent
                spacing: Theme.paddingMedium

                Button {
                    text: qsTr("Фон")
                    onClicked: console.log("Background selected")
                }
                Button {
                    text: qsTr("Улучшение")
                    onClicked: console.log("Enhance selected")
                }
                Button {
                    text: qsTr("Стиль")
                    onClicked: console.log("Style selected")
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
                selectedImage.source = ""
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
}
