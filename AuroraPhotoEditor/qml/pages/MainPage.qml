import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

Page {
    objectName: "mainPage"
    allowedOrientations: Orientation.All

    PageHeader {
        objectName: "pageHeader"
        title: qsTr("PhotoEditor")
        extraContent.children: [
            IconButton {
                objectName: "aboutButton"
                icon.source: "image://theme/icon-m-about"
                anchors.verticalCenter: parent.verticalCenter

                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        ]
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.paddingLarge
        width: parent.width

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Select Photo")
            onClicked: {
                pageStack.push(imagePickerComponent)
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
