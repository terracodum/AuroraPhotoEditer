import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import ru.template.AuroraPhotoEditor 1.0

Page {
    objectName: "enhanceToolPage"
    allowedOrientations: Orientation.All

    property string selectedImagePath: ""
    property string resultImagePath: ""
    property bool showOriginal: false

    // Enhancement settings
    property bool enableContrast: true
    property bool enableWhiteBalance: true
    property bool enableClahe: false
    property double claheClipLimit: 2.0

    EnhanceEngine {
        id: enhanceEngine
        onEnhanceDone: {
            resultImagePath = resultPath
        }
        onErrorOccurred: {
            console.log("Enhance Error:", error)
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
                title: qsTr("Улучшение")
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
                        running: enhanceEngine.isProcessing
                        size: BusyIndicatorSize.Large
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Выберите фото для улучшения")
                    color: Theme.secondaryColor
                    visible: selectedImagePath === ""
                    font.pixelSize: Theme.fontSizeSmall
                }

                // Before/After toggle indicator
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

            // --- Enhancement options ---
            SectionHeader {
                text: qsTr("Параметры улучшения")
            }

            TextSwitch {
                id: contrastSwitch
                text: qsTr("Автоконтраст")
                description: qsTr("Обрезка гистограммы — убирает серость и тусклость")
                checked: enableContrast
                onCheckedChanged: enableContrast = checked
            }

            TextSwitch {
                id: wbSwitch
                text: qsTr("Баланс белого")
                description: qsTr("Алгоритм «Серого мира» — убирает цветовой оттенок")
                checked: enableWhiteBalance
                onCheckedChanged: enableWhiteBalance = checked
            }

            TextSwitch {
                id: claheSwitch
                text: qsTr("Адаптивный контраст (CLAHE)")
                description: qsTr("Вытягивает детали из теней без пересветов")
                checked: enableClahe
                onCheckedChanged: enableClahe = checked
            }

            Slider {
                width: parent.width
                minimumValue: 1.0
                maximumValue: 4.0
                stepSize: 0.5
                value: claheClipLimit
                label: qsTr("Сила CLAHE")
                valueText: value.toFixed(1)
                visible: claheSwitch.checked
                onValueChanged: claheClipLimit = value
            }

            // --- Process button ---
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Улучшить")
                enabled: selectedImagePath !== "" && !enhanceEngine.isProcessing
                         && (enableContrast || enableWhiteBalance || enableClahe)
                onClicked: {
                    resultImagePath = ""
                    showOriginal = false
                    enhanceEngine.process(
                        selectedImagePath,
                        enableContrast,
                        enableWhiteBalance,
                        enableClahe,
                        claheClipLimit
                    )
                }
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
                pageStack.pop()
            }
        }
    }
}
