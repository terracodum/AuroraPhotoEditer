import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import ru.template.AuroraPhotoEditor 1.0
import "../components"

Page {
    objectName: "backgroundToolPage"
    allowedOrientations: Orientation.All

    property string selectedImagePath: ""
    property string currentMaskPath: ""
    property string resultImagePath: ""
    
    // Store current settings
    property bool enableBlur: false
    property int blurRadius: 10
    
    property bool enableColor: false
    property color solidColor: Qt.rgba(1.0, 1.0, 1.0, 1.0)
    
    property bool enableGradient: false
    property int gradientType: 0 // 0: Linear, 1: Radial
    property color gradStartColor: Qt.rgba(1.0, 0.0, 0.0, 1.0)
    property color gradEndColor: Qt.rgba(0.0, 0.0, 1.0, 1.0)
    
    SegmentationEngine {
        id: segEngine
        onSegmentationDone: {
            currentMaskPath = maskPath
            applyEffects()
        }
        onErrorOccurred: {
            console.log("Segmentation Error:", error)
        }
    }

    ImageProcessor {
        id: imgProcessor
    }

    function applyEffects() {
        if (currentMaskPath === "" || selectedImagePath === "") return;
        
        var settings = {
            "enableBlur": enableBlur,
            "blurRadius": blurRadius,
            "enableColor": enableColor,
            "solidColor": solidColor,
            "enableGradient": enableGradient,
            "gradientType": gradientType,
            "gradStartColor": gradStartColor,
            "gradEndColor": gradEndColor
        };
        
        var res = imgProcessor.processAdvancedBackground(selectedImagePath, currentMaskPath, settings);
        if (res !== "") {
            resultImagePath = res;
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Фон")
            }

            Rectangle {
                width: parent.width - Theme.horizontalPageMargin * 2
                height: Math.min(width * 3 / 4, Screen.height / 3)
                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.1)
                radius: Theme.paddingSmall

                Image {
                    id: previewImage
                    anchors.fill: parent
                    anchors.margins: Theme.paddingSmall
                    fillMode: Image.PreserveAspectFit
                    source: resultImagePath !== "" ? "file://" + resultImagePath : (selectedImagePath !== "" ? "file://" + selectedImagePath : "")
                    visible: source != ""

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: parent.status === Image.Loading || segEngine.isProcessing
                        size: BusyIndicatorSize.Large
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("No Image Selected")
                    color: Theme.secondaryColor
                    visible: selectedImagePath === ""
                }
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Select Photo")
                onClicked: pageStack.push(imagePickerComponent)
            }

            SectionHeader {
                text: qsTr("Модель ИИ")
            }
            
            ComboBox {
                id: modelSelector
                width: parent.width
                label: qsTr("Модель")
                menu: ContextMenu {
                    MenuItem { text: "RMBG-1.4 (Ультимативная, 170МБ)" }
                    MenuItem { text: "U-2-NetP (Легкая, 10МБ)" }
                }
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Вырезать фон")
                enabled: selectedImagePath !== "" && !segEngine.isProcessing
                onClicked: {
                    var modelPath = modelSelector.currentIndex === 0 
                        ? "/usr/share/ru.template.AuroraPhotoEditor/models/rmbg14.onnx"
                        : "/usr/share/ru.template.AuroraPhotoEditor/models/u2netp.onnx";
                    
                    segEngine.setModelPath(modelPath);
                    segEngine.process(selectedImagePath);
                }
            }
            
            // --- BLUR ---
            SectionHeader { text: qsTr("Размытие оригинального фона") }
            TextSwitch {
                id: blurSwitch
                text: qsTr("Включить размытие")
                checked: enableBlur
                onCheckedChanged: {
                    enableBlur = checked;
                    if (currentMaskPath !== "") applyEffects();
                }
            }
            Slider {
                width: parent.width
                minimumValue: 1
                maximumValue: 50
                stepSize: 1
                value: blurRadius
                label: qsTr("Сила размытия")
                valueText: value.toString()
                enabled: blurSwitch.checked
                onValueChanged: {
                    blurRadius = value;
                }
                onReleased: {
                    if (enableBlur && currentMaskPath !== "") applyEffects();
                }
            }
            
            // --- COLOR ---
            SectionHeader { text: qsTr("Статичный цвет") }
            TextSwitch {
                id: colorSwitch
                text: qsTr("Включить заливку цветом")
                checked: enableColor
                onCheckedChanged: {
                    enableColor = checked;
                    if (currentMaskPath !== "") applyEffects();
                }
            }
            ColorSliderGroup {
                id: solidColorGroup
                visible: colorSwitch.checked
                onSelectedColorChanged: {
                    solidColor = selectedColor;
                }
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Применить цвет")
                visible: colorSwitch.checked
                onClicked: {
                    if (currentMaskPath !== "") applyEffects();
                }
            }
            
            // --- GRADIENT ---
            SectionHeader { text: qsTr("Градиент") }
            TextSwitch {
                id: gradSwitch
                text: qsTr("Включить градиент")
                checked: enableGradient
                onCheckedChanged: {
                    enableGradient = checked;
                    if (currentMaskPath !== "") applyEffects();
                }
            }
            ComboBox {
                id: gradTypeSelector
                width: parent.width
                label: qsTr("Тип градиента")
                visible: gradSwitch.checked
                menu: ContextMenu {
                    MenuItem { text: "Линейный" }
                    MenuItem { text: "Радиальный" }
                }
                onCurrentIndexChanged: {
                    gradientType = currentIndex;
                    if (enableGradient && currentMaskPath !== "") applyEffects();
                }
            }
            Label {
                text: "Начальный цвет:"
                x: Theme.horizontalPageMargin
                visible: gradSwitch.checked
                color: Theme.highlightColor
            }
            ColorSliderGroup {
                id: gradStartGroup
                visible: gradSwitch.checked
                onSelectedColorChanged: {
                    gradStartColor = selectedColor;
                }
            }
            Label {
                text: "Конечный цвет:"
                x: Theme.horizontalPageMargin
                visible: gradSwitch.checked
                color: Theme.highlightColor
            }
            ColorSliderGroup {
                id: gradEndGroup
                visible: gradSwitch.checked
                onSelectedColorChanged: {
                    gradEndColor = selectedColor;
                }
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Применить градиент")
                visible: gradSwitch.checked
                onClicked: {
                    if (currentMaskPath !== "") applyEffects();
                }
            }
            
            // --- SAVE ---
            SectionHeader { text: qsTr("Экспорт") }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Сохранить в галерею")
                enabled: resultImagePath !== ""
                onClicked: {
                    if (imgProcessor.saveToGallery(resultImagePath)) {
                        console.log("Saved!")
                    }
                }
            }
            
            Item {
                width: 1
                height: Theme.paddingLarge * 2
            }
        }
    }

    Component {
        id: imagePickerComponent
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                selectedImagePath = selectedContentProperties.filePath
                currentMaskPath = ""
                resultImagePath = ""
                pageStack.pop()
            }
        }
    }
}
