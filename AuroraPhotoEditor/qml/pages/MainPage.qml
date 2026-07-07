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
                text: qsTr("Фон (Вырезание/Замена)")
                onClicked: pageStack.push(Qt.resolvedUrl("BackgroundToolPage.qml"))
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Улучшение (Автокоррекция)")
                onClicked: showStub("Функция автокоррекции в разработке")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Стиль (Нейросети)")
                onClicked: showStub("Нейросетевая стилизация в разработке")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("История проектов")
                onClicked: showStub("История проектов появится позже")
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Пакетная обработка")
                onClicked: showStub("Пакетная обработка в разработке")
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
