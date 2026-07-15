import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../"
import "../components"

Page {
    id: mainPage
    objectName: "mainPage"
    allowedOrientations: Orientation.All

    property string activeTool: ""

    // Label shown under the busy spinner — set right before triggering the
    // matching pipelineManager call, so it stays correct even if a later
    // tool supersedes the running one (see operationCanceled handling below).
    property string currentOperationLabel: qsTr("Processing")

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    PageHeaderBar {
        id: header
        objectName: "pageHeader"
        title: qsTr("PhotoEditor")

        GlyphButton {
            objectName: "undoButton"
            iconName: "undo"
            enabled: pipelineManager.canUndo
            onClicked: pipelineManager.undoLast()
        }
        GlyphButton {
            objectName: "resetButton"
            iconName: "reset"
            enabled: pipelineManager.canUndo
            onClicked: pipelineManager.resetToOriginal()
        }
        GlyphButton {
            objectName: "saveButton"
            iconName: "save"
            visible: pipelineManager.hasImage
            width: visible ? NeonTheme.iconButtonSize : 0
            onClicked: pipelineManager.exportImage()
        }
        GlyphButton {
            objectName: "aboutButton"
            iconName: "about"
            onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
        }
    }

    // Decorative "pull down for menu" affordance (README screen 1). Purely
    // visual — the actual gesture is Silica's native PullDownMenu drag,
    // which works regardless of what is drawn above the flickable.
    Item {
        id: pullHint
        anchors.top: header.bottom
        width: parent.width
        height: NeonTheme.px(72)

        Rectangle {
            anchors.centerIn: parent
            width: hintRow.width + NeonTheme.paddingLarge * 2
            height: NeonTheme.px(48)
            radius: height / 2
            color: "#241B38"

            Row {
                id: hintRow
                anchors.centerIn: parent
                spacing: NeonTheme.paddingTiny

                GlyphIcon {
                    name: "chevronDown"
                    anchors.verticalCenter: parent.verticalCenter
                    width: NeonTheme.px(24)
                    height: NeonTheme.px(24)
                    strokeColor: NeonTheme.textTertiary
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Потянуть вниз — меню")
                    color: NeonTheme.textTertiary
                    font.family: NeonTheme.fontBody
                    font.pixelSize: NeonTheme.fontSizeCaption
                }
            }
        }
    }

    SilicaFlickable {
        id: flick
        anchors.top: pullHint.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true

        PullDownMenu {
            MenuItem {
                text: qsTr("Выбрать фото")
                onClicked: pageStack.push(imagePickerComponent)
            }
            MenuItem {
                text: pipelineManager.commandCount() > 0
                      ? qsTr("История изменений") + " (" + pipelineManager.commandCount() + ")"
                      : qsTr("История изменений")
                onClicked: historyPanel.show()
            }
            MenuItem {
                text: qsTr("Пакетная обработка")
                onClicked: pageStack.push(Qt.resolvedUrl("BatchPage.qml"))
            }
        }

        // Empty state
        Column {
            id: placeholder
            anchors.centerIn: parent
            spacing: NeonTheme.paddingLarge
            visible: !pipelineManager.hasImage
            width: parent.width - 2 * NeonTheme.paddingXLarge

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: NeonTheme.px(180)
                height: NeonTheme.px(180)
                radius: NeonTheme.radiusXLarge
                color: NeonTheme.bgCard

                GlyphIcon {
                    anchors.centerIn: parent
                    name: "image"
                    width: parent.width * 0.5
                    height: parent.height * 0.5
                    strokeColor: NeonTheme.textSecondary
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Нет выбранного фото")
                color: NeonTheme.textPrimary
                font.family: NeonTheme.fontDisplay
                font.weight: NeonTheme.fontWeightBold
                font.pixelSize: NeonTheme.fontSizeCardTitle
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Выберите фото из галереи, чтобы начать редактирование")
                color: NeonTheme.textSecondary
                font.family: NeonTheme.fontBody
                font.weight: NeonTheme.fontWeightMedium
                font.pixelSize: NeonTheme.fontSizeBodyLarge
            }

            GradientButton {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width, NeonTheme.px(520))
                text: qsTr("Выбрать фото")
                onClicked: pageStack.push(imagePickerComponent)
            }
        }

        // Loaded state
        Image {
            id: selectedImage
            anchors.fill: parent
            anchors.margins: NeonTheme.paddingLarge
            anchors.bottomMargin: toolsPanel.height + NeonTheme.paddingLarge
            fillMode: Image.PreserveAspectFit
            visible: pipelineManager.hasImage
            opacity: visible ? 1.0 : 0.0
            cache: false // Prevent memory leaks from timestamp updates

            Behavior on opacity {
                FadeAnimation { duration: NeonTheme.fadeDuration }
            }
        }

        Rectangle {
            anchors.fill: selectedImage
            visible: pipelineManager.hasImage
            color: "transparent"
            radius: NeonTheme.radiusLarge
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.08)
        }

        BusyOverlay {
            anchors.fill: parent
            running: pipelineManager.isProcessing
            operationLabel: mainPage.currentOperationLabel
        }
    }

    DockedPanel {
        id: toolsPanel
        width: parent.width
        height: NeonTheme.toolPanelHeight
        dock: Dock.Bottom
        open: pipelineManager.hasImage && activeTool === ""

        Rectangle {
            anchors.fill: parent
            color: NeonTheme.bgBase
        }

        Row {
            anchors.centerIn: parent
            spacing: NeonTheme.paddingMedium

            ToolButton {
                width: NeonTheme.px(150)
                height: NeonTheme.px(132)
                label: qsTr("Фон")
                iconName: "layers"
                accent: NeonTheme.accentPurple
                enabled: !pipelineManager.isProcessing
                onClicked: {
                    // Real background-removal backend (updateBackground()) is
                    // wired to the legacy backgroundPanel below, not to the
                    // new BackgroundSheet mock — see the note above that panel.
                    mainPage.currentOperationLabel = qsTr("Фон")
                    activeTool = "background"
                    pipelineManager.applyBackgroundRemoval()
                }
            }
            ToolButton {
                width: NeonTheme.px(150)
                height: NeonTheme.px(132)
                label: qsTr("Улучшение")
                iconName: "spark"
                accent: NeonTheme.accentGreen
                enabled: !pipelineManager.isProcessing
                onClicked: {
                    mainPage.currentOperationLabel = qsTr("Улучшение")
                    pipelineManager.applyEnhance()
                }
            }
            ToolButton {
                width: NeonTheme.px(150)
                height: NeonTheme.px(132)
                label: qsTr("Стиль")
                iconName: "palette"
                accent: NeonTheme.accentPink
                enabled: !pipelineManager.isProcessing
                onClicked: styleSheet.show()
            }
        }
    }

    // NOTE: predates the neon-pop redesign and is intentionally not
    // restyled with NeonTheme — kept as-is (rather than swapped for the new
    // BackgroundSheet mock) because it is the only place the real
    // background-removal/color/blur backend (updateBackground()) is wired
    // up end-to-end. Restyle in place once BackgroundSheet grows real
    // backend calls.
    DockedPanel {
        id: backgroundPanel
        width: parent.width
        height: Theme.itemSizeExtraLarge * 4
        dock: Dock.Bottom
        open: activeTool === "background"

        property string currentTab: "color"
        property string selectedColor1: "white"
        property string selectedColor2: "black"
        property bool isGradient: false

        function updateBg() {
            if (currentTab === "color") {
                if (isGradient) {
                    pipelineManager.updateBackground(1, selectedColor1, selectedColor2, 0)
                } else {
                    pipelineManager.updateBackground(0, selectedColor1, "transparent", 0)
                }
            } else {
                pipelineManager.updateBackground(2, "transparent", "transparent", blurSlider.value)
            }
        }

        Rectangle {
            anchors.fill: parent
            color: Theme.overlayBackgroundColor

            Column {
                anchors.fill: parent
                spacing: Theme.paddingSmall

                // Tabs
                Row {
                    width: parent.width
                    height: Theme.itemSizeMedium
                    
                    Button {
                        width: parent.width / 2
                        text: qsTr("Цвет")
                        highlighted: backgroundPanel.currentTab === "color"
                        onClicked: {
                            backgroundPanel.currentTab = "color"
                            backgroundPanel.updateBg()
                        }
                    }
                    Button {
                        width: parent.width / 2
                        text: qsTr("Размытие")
                        highlighted: backgroundPanel.currentTab === "blur"
                        onClicked: {
                            backgroundPanel.currentTab = "blur"
                            backgroundPanel.updateBg()
                        }
                    }
                }

                // Color Tab Content
                Item {
                    width: parent.width
                    height: Theme.itemSizeExtraLarge * 1.5
                    visible: backgroundPanel.currentTab === "color"

                    Column {
                        anchors.fill: parent
                        spacing: Theme.paddingSmall
                        
                        TextSwitch {
                            text: qsTr("Градиент")
                            checked: backgroundPanel.isGradient
                            onCheckedChanged: {
                                backgroundPanel.isGradient = checked
                                backgroundPanel.updateBg()
                            }
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: Theme.paddingMedium
                            
                            Repeater {
                                model: ["white", "black", "red", "green", "blue", "yellow"]
                                Rectangle {
                                    width: Theme.iconSizeMedium
                                    height: Theme.iconSizeMedium
                                    color: modelData
                                    radius: width / 2
                                    border.color: (backgroundPanel.selectedColor1 === modelData || (backgroundPanel.isGradient && backgroundPanel.selectedColor2 === modelData)) ? Theme.highlightColor : Theme.primaryColor
                                    border.width: (backgroundPanel.selectedColor1 === modelData || (backgroundPanel.isGradient && backgroundPanel.selectedColor2 === modelData)) ? 4 : 1
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (backgroundPanel.isGradient) {
                                                if (backgroundPanel.selectedColor1 === modelData) {
                                                    backgroundPanel.selectedColor2 = modelData
                                                } else {
                                                    backgroundPanel.selectedColor1 = modelData
                                                }
                                            } else {
                                                backgroundPanel.selectedColor1 = modelData
                                            }
                                            backgroundPanel.updateBg()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Blur Tab Content
                Item {
                    width: parent.width
                    height: Theme.itemSizeExtraLarge * 1.5
                    visible: backgroundPanel.currentTab === "blur"

                    Slider {
                        id: blurSlider
                        width: parent.width - Theme.paddingLarge * 2
                        anchors.centerIn: parent
                        minimumValue: 0
                        maximumValue: 100
                        value: 50
                        stepSize: 1
                        label: qsTr("Интенсивность")
                        valueText: value
                        onValueChanged: backgroundPanel.updateBg()
                    }
                }

                // Accept/Cancel buttons
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingLarge

                    Button {
                        text: qsTr("Вернуться")
                        onClicked: {
                            pipelineManager.undoLast()
                            activeTool = ""
                        }
                    }

                    Button {
                        text: qsTr("Применить")
                        onClicked: {
                            activeTool = ""
                        }
                    }
                }
            }
        }
    }

    ToastBanner {
        id: toast
        z: 1000
        anchors.top: pullHint.bottom
        anchors.topMargin: NeonTheme.paddingSmall
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Connections {
        target: pipelineManager
        onCurrentImageChanged: {
            if (pipelineManager.hasImage) {
                selectedImage.source = "image://pipeline/current?t=" + Date.now()
            } else {
                selectedImage.source = undefined
            }
        }

        onExportCompleted: {
            if (success) {
                pageStack.push(Qt.resolvedUrl("NoticePage.qml"), {
                    "message": qsTr("Image successfully saved to:\n") + filePath,
                    "success": true
                })
            } else {
                pageStack.push(Qt.resolvedUrl("NoticePage.qml"), {
                    "message": qsTr("Failed to save image"),
                    "success": false
                })
            }
        }

        onLoadFailed: toast.show(reason, "error")
        onOperationCanceled: toast.show(qsTr("Предыдущая операция отменена"), "info")
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

    HistoryPanel {
        id: historyPanel
        onSaveProjectRequested: toast.show(qsTr("Проект сохранён (демо-режим)"), "success")
    }

    BackgroundSheet {
        id: backgroundSheet
        onMaskEditRequested: {
            backgroundSheet.hide()
            pageStack.push(Qt.resolvedUrl("MaskEditPage.qml"))
        }
    }

    StyleSheet {
        id: styleSheet
    }
}
