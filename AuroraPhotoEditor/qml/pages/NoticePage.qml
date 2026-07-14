import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: noticePage
    allowedOrientations: Orientation.All
    
    property string message: "Success"
    
    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.horizontalPageMargin * 2
        spacing: Theme.paddingLarge
        
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "image://theme/icon-l-acknowledge"
            highlighted: true
        }
        
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: noticePage.message
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeLarge
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
        
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("OK")
            onClicked: pageStack.pop()
        }
    }
}
