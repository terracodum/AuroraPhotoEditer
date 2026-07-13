import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: historyPage
    objectName: "historyPage"
    allowedOrientations: Orientation.All

    property var projects: []

    Component.onCompleted: {
        projects = pipelineManager.getSavedProjects();
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: projects
        
        header: PageHeader {
            title: qsTr("История проектов")
        }
        
        delegate: ListItem {
            id: delegate
            
            Column {
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 2*x
                
                Label {
                    text: modelData.name
                    color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                    font.pixelSize: Theme.fontSizeMedium
                }
                Label {
                    text: modelData.date + " — " + modelData.summary
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.WordWrap
                }
            }
            
            onClicked: {
                // Here we could load the project or show its commands
                infoBanner.show(qsTr("Команды: ") + modelData.commands)
            }
        }
        
        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("Нет сохраненных проектов")
            hintText: qsTr("Сохраните проект в редакторе, чтобы увидеть его здесь")
        }
    }
    
    // Notification banner
    Rectangle {
        id: infoBanner
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingLarge
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.9
        height: Theme.itemSizeMedium * 1.5
        radius: Theme.paddingMedium
        color: Theme.highlightBackgroundColor
        opacity: 0.0
        z: 100
        
        property alias text: bannerText.text
        
        Label {
            id: bannerText
            anchors.centerIn: parent
            anchors.margins: Theme.paddingMedium
            width: parent.width - 2*Theme.paddingMedium
            color: Theme.primaryColor
            wrapMode: Text.WrapAnywhere
            font.pixelSize: Theme.fontSizeSmall
            horizontalAlignment: Text.AlignHCenter
        }
        
        Behavior on opacity { FadeAnimation {} }
        
        Timer {
            id: bannerTimer
            interval: 5000
            onTriggered: infoBanner.opacity = 0.0
        }
        
        function show(msg) {
            text = msg;
            opacity = 0.9;
            bannerTimer.restart();
        }
    }
}
