import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    objectName: "mainPage"
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Photo Editor Hub")
            }

            SectionHeader {
                text: qsTr("Инструменты")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Открыть редактор (Стиль/Улучшение)")
                onClicked: {
                    var picker = pageStack.push("Sailfish.Pickers.ImagePickerPage");
                    picker.selectedContentPropertiesChanged.connect(function() {
                        var filePath = picker.selectedContentProperties.filePath;
                        pageStack.push(Qt.resolvedUrl("EditorPage.qml"), { "imagePath": filePath });
                    });
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Фон (Вырезание/Замена)")
                onClicked: pageStack.push(Qt.resolvedUrl("BackgroundToolPage.qml"))
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("История проектов")
                onClicked: pageStack.push(Qt.resolvedUrl("HistoryPage.qml"))
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Пакетная обработка")
                onClicked: pageStack.push(Qt.resolvedUrl("BatchProcessingPage.qml"))
            }
        }
    }

    Label {
        id: stubLabel
        anchors.bottom: parent.bottom
        anchors.margins: Theme.paddingLarge
        anchors.horizontalCenter: parent.horizontalCenter
        color: Theme.highlightColor
        opacity: 0.0
        Behavior on opacity { FadeAnimation {} }
    }

    Timer {
        id: stubTimer
        interval: 2500
        onTriggered: stubLabel.opacity = 0.0
    }

    function showStub(msg) {
        stubLabel.text = msg;
        stubLabel.opacity = 1.0;
        stubTimer.restart();
    }
}
