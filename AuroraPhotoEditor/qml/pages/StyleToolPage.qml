import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import ru.template.AuroraPhotoEditor 1.0

Page {
    objectName: "styleToolPage"
    allowedOrientations: Orientation.All

    property string selectedImagePath: ""
    property string resultImagePath: ""
    property int selectedStyleIndex: -1
    property bool showOriginal: false

    // Available styles — each maps to an ONNX model file
    // Models from ONNX Model Zoo (Fast Neural Style Transfer, ~6.6 MB each)
    ListModel {
        id: stylesModel
        ListElement {
            name: "Candy"
            description: "Яркий леденцовый стиль"
            modelFile: "candy.onnx"
            emoji: "🍬"
        }
        ListElement {
            name: "Mosaic"
            description: "Античная мозаика"
            modelFile: "mosaic.onnx"
            emoji: "🏛"
        }
        ListElement {
            name: "Udnie"
            description: "Абстрактная живопись"
            modelFile: "udnie.onnx"
            emoji: "🎨"
        }
        ListElement {
            name: "Rain Princess"
            description: "Дождливый импрессионизм"
            modelFile: "rain_princess.onnx"
            emoji: "🌧"
        }
        ListElement {
            name: "Pointilism"
            description: "Точечная живопись"
            modelFile: "pointilism.onnx"
            emoji: "🔵"
        }
    }

    StyleTransferEngine {
        id: styleEngine
        onStyleDone: {
            resultImagePath = resultPath
        }
        onErrorOccurred: {
            console.log("Style Error:", error)
            statusLabel.text = qsTr("Ошибка: ") + error
            statusLabel.opacity = 1.0
            statusTimer.restart()
        }
    }

    ImageProcessor {
        id: imgProcessor
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Выбрать фото")
                onClicked: pageStack.push(imagePickerComponent)
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Стилизация")
            }

            // --- Preview area ---
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
                    source: {
                        if (showOriginal && selectedImagePath !== "") {
                            return "file://" + selectedImagePath
                        }
                        if (resultImagePath !== "") {
                            return "file://" + resultImagePath
                        }
                        if (selectedImagePath !== "") {
                            return "file://" + selectedImagePath
                        }
                        return ""
                    }
                    visible: source != ""
                    cache: false

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: styleEngine.isProcessing
                        size: BusyIndicatorSize.Large
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Выберите фото для стилизации")
                    color: Theme.secondaryColor
                    visible: selectedImagePath === ""
                    font.pixelSize: Theme.fontSizeSmall
                }

                // Before/After indicator
                Label {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Theme.paddingSmall
                    text: showOriginal ? qsTr("Оригинал") : qsTr("Результат")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    visible: resultImagePath !== ""
                    opacity: 0.8

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -Theme.paddingSmall / 2
                        color: Theme.rgba(Theme.highlightDimmerColor, 0.7)
                        radius: Theme.paddingSmall / 2
                        z: -1
                    }
                }
            }

            // --- Before/After toggle ---
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: showOriginal ? qsTr("Показать результат") : qsTr("Показать оригинал")
                visible: resultImagePath !== ""
                onClicked: showOriginal = !showOriginal
            }

            // --- Select photo ---
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Выбрать фото")
                onClicked: pageStack.push(imagePickerComponent)
            }

            // --- Styles selection ---
            SectionHeader {
                text: qsTr("Выберите стиль")
            }

            // Style selector as a vertical list with highlight
            Repeater {
                model: stylesModel
                delegate: BackgroundItem {
                    width: parent.width
                    height: Theme.itemSizeMedium
                    highlighted: selectedStyleIndex === index

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.horizontalPageMargin
                        anchors.rightMargin: Theme.horizontalPageMargin
                        spacing: Theme.paddingMedium

                        // Emoji icon
                        Label {
                            text: model.emoji
                            font.pixelSize: Theme.fontSizeLarge
                            anchors.verticalCenter: parent.verticalCenter
                            width: Theme.itemSizeSmall
                            horizontalAlignment: Text.AlignHCenter
                        }

                        // Name + description
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Theme.paddingSmall / 2

                            Label {
                                text: model.name
                                color: selectedStyleIndex === index
                                       ? Theme.highlightColor : Theme.primaryColor
                                font.pixelSize: Theme.fontSizeMedium
                            }

                            Label {
                                text: model.description
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                            }
                        }
                    }

                    // Selection indicator
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: Theme.paddingSmall / 2
                        color: Theme.highlightColor
                        visible: selectedStyleIndex === index
                    }

                    onClicked: {
                        selectedStyleIndex = index
                    }
                }
            }

            // --- Process button ---
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Стилизовать")
                enabled: selectedImagePath !== "" && selectedStyleIndex >= 0
                         && !styleEngine.isProcessing
                onClicked: {
                    resultImagePath = ""
                    showOriginal = false

                    var styleItem = stylesModel.get(selectedStyleIndex)
                    var modelPath = "/usr/share/ru.template.AuroraPhotoEditor/models/" + styleItem.modelFile

                    styleEngine.setModelPath(modelPath)
                    styleEngine.process(selectedImagePath)
                }
            }

            // --- Processing status ---
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Обработка может занять некоторое время...")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                visible: styleEngine.isProcessing
            }

            // --- Export ---
            SectionHeader {
                text: qsTr("Экспорт")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Сохранить в галерею")
                enabled: resultImagePath !== ""
                onClicked: {
                    if (imgProcessor.saveToGallery(resultImagePath)) {
                        statusLabel.text = qsTr("Сохранено!")
                        statusLabel.opacity = 1.0
                        statusTimer.restart()
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge * 2
            }
        }
    }

    // --- Status notification ---
    Label {
        id: statusLabel
        anchors.bottom: parent.bottom
        anchors.margins: Theme.paddingLarge
        anchors.horizontalCenter: parent.horizontalCenter
        color: Theme.highlightColor
        opacity: 0.0
        Behavior on opacity { FadeAnimation {} }
    }

    Timer {
        id: statusTimer
        interval: 2500
        onTriggered: statusLabel.opacity = 0.0
    }

    // --- Image picker ---
    Component {
        id: imagePickerComponent
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                selectedImagePath = selectedContentProperties.filePath
                resultImagePath = ""
                showOriginal = false
                selectedStyleIndex = -1
                pageStack.pop()
            }
        }
    }
}
