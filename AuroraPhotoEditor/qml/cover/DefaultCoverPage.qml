import QtQuick 2.0
import Sailfish.Silica 1.0
import "../"

CoverBackground {
    objectName: "defaultCover"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: NeonTheme.bgSurface }
            GradientStop { position: 1.0; color: NeonTheme.bgBase }
        }
    }

    CoverTemplate {
        objectName: "applicationCover"
        primaryText: "AuroraPhotoEditor"
        secondaryText: qsTr("PhotoEditor")
        icon {
            source: Qt.resolvedUrl("../icons/AuroraPhotoEditor.svg")
            sourceSize { width: icon.width; height: icon.height }
        }
    }
}
