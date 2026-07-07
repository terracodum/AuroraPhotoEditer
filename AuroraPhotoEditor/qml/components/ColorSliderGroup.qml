import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    id: root
    width: parent.width
    spacing: Theme.paddingSmall

    // Expose the resulting color
    property color selectedColor: Qt.rgba(rSlider.value / 255.0, gSlider.value / 255.0, bSlider.value / 255.0, 1.0)

    // Function to set the initial color from outside
    function setRgb(r, g, b) {
        rSlider.value = r
        gSlider.value = g
        bSlider.value = b
    }

    Row {
        width: parent.width - Theme.horizontalPageMargin * 2
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.paddingLarge
        
        Rectangle {
            width: Theme.itemSizeMedium
            height: Theme.itemSizeSmall
            radius: Theme.paddingSmall
            color: root.selectedColor
            border.color: Theme.highlightColor
            border.width: 2
            anchors.verticalCenter: parent.verticalCenter
        }
        
        Label {
            text: root.selectedColor.toString()
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }
    }
    
    Slider {
        id: rSlider
        width: parent.width
        minimumValue: 0
        maximumValue: 255
        stepSize: 1
        value: 255 // Default white
        label: qsTr("Red")
        valueText: value.toString()
    }
    Slider {
        id: gSlider
        width: parent.width
        minimumValue: 0
        maximumValue: 255
        stepSize: 1
        value: 255
        label: qsTr("Green")
        valueText: value.toString()
    }
    Slider {
        id: bSlider
        width: parent.width
        minimumValue: 0
        maximumValue: 255
        stepSize: 1
        value: 255
        label: qsTr("Blue")
        valueText: value.toString()
    }
}
