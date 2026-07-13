import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: editorPage
    objectName: "editorPage"
    allowedOrientations: Orientation.All

    property string imagePath: ""

    property int currentTab: 0

    // Define style models statically. In a real app, these would come from C++.
    property var styleModels: [
        { name: "Candy", path: "/usr/share/ru.template.AuroraPhotoEditor/models/candy.onnx", icon: "🍬" },
        { name: "Mosaic", path: "/usr/share/ru.template.AuroraPhotoEditor/models/mosaic.onnx", icon: "🎨" },
        { name: "Rain", path: "/usr/share/ru.template.AuroraPhotoEditor/models/rain-princess.onnx", icon: "🌧" },
        { name: "Udnie", path: "/usr/share/ru.template.AuroraPhotoEditor/models/udnie.onnx", icon: "🖼" }
    ]

    Component.onCompleted: {
        if (imagePath !== "") {
            pipelineManager.loadImage(imagePath)
        }
        
        // Connect to signals for save notifications
        pipelineManager.saveDone.connect(function(path) {
            infoBanner.show("Сохранено: " + path)
        })
        pipelineManager.saveError.connect(function(msg) {
            infoBanner.show("Ошибка: " + msg)
        })
        pipelineManager.processingError.connect(function(msg) {
            infoBanner.show("Ошибка обработки: " + msg)
        })
    }
    
    Component.onDestruction: {
        // Disconnect to avoid memory leaks if object destroyed
        pipelineManager.saveDone.disconnect()
        pipelineManager.saveError.disconnect()
        pipelineManager.processingError.disconnect()
    }

    // AppBar for actions
    PageHeader {
        id: header
        title: qsTr("Редактор")
        extraContent.children: [
            Row {
                spacing: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter
                
                IconButton {
                    icon.source: "image://theme/icon-m-undo"
                    enabled: pipelineManager.canUndo && !pipelineManager.isProcessing
                    onClicked: pipelineManager.undoLast()
                }
                IconButton {
                    icon.source: "image://theme/icon-m-refresh"
                    enabled: pipelineManager.hasImage && !pipelineManager.isProcessing
                    onClicked: pipelineManager.resetToOriginal()
                }
                IconButton {
                    icon.source: "image://theme/icon-m-save"
                    enabled: pipelineManager.canSave && !pipelineManager.isProcessing
                    onClicked: pipelineManager.saveResult()
                }
            }
        ]
    }

    // Main image view
    SilicaFlickable {
        id: imageFlickable
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: toolPanel.top
        }
        contentWidth: Math.max(width, mainImage.width)
        contentHeight: Math.max(height, mainImage.height)
        clip: true

        PullDownMenu {
            MenuItem {
                text: qsTr("Сохранить как проект")
                onClicked: {
                    var name = "Project_" + Qt.formatDateTime(new Date(), "yyyyMMdd_hhmmss");
                    pipelineManager.saveProject(name);
                    infoBanner.show(qsTr("Проект сохранен: ") + name);
                }
            }
        }

        Image {
            id: mainImage
            // Use image provider with timestamp for cache invalidation
            source: pipelineManager.hasImage ? "image://editor/working_copy?" + pipelineManager.layoutTimestamp : ""
            anchors.centerIn: parent
            
            // Fit to screen by default
            width: imageFlickable.width
            height: imageFlickable.height
            fillMode: Image.PreserveAspectFit
            cache: false // Ensure we always fetch fresh frames
        }
        
        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: pipelineManager.isProcessing
            visible: running
        }
    }
    
    // Bottom Tool Panel (Tabs)
    Rectangle {
        id: toolPanel
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: toolColumn.height
        color: Theme.highlightDimmerColor
        
        Column {
            id: toolColumn
            width: parent.width
            
            // Tool selector
            Row {
                width: parent.width
                height: Theme.itemSizeMedium
                
                BackgroundItem {
                    width: parent.width / 2
                    height: parent.height
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Стиль")
                        color: currentTab === 0 ? Theme.highlightColor : Theme.primaryColor
                        font.bold: currentTab === 0
                    }
                    onClicked: currentTab = 0
                }
                
                BackgroundItem {
                    width: parent.width / 2
                    height: parent.height
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Улучшение")
                        color: currentTab === 1 ? Theme.highlightColor : Theme.primaryColor
                        font.bold: currentTab === 1
                    }
                    onClicked: currentTab = 1
                }
            }
            
            // Tool contents
            Item {
                id: toolStack
                width: parent.width
                height: Theme.itemSizeExtraLarge * 2.5
                
                // --- Style Tool ---
                Item {
                    anchors.fill: parent
                    visible: currentTab === 0
                    
                    Column {
                        width: parent.width
                        spacing: Theme.paddingMedium
                        
                        // Style thumbnails (Horizontal)
                        ListView {
                            id: styleList
                            width: parent.width
                            height: Theme.itemSizeLarge
                            orientation: ListView.Horizontal
                            spacing: Theme.paddingMedium
                            model: styleModels
                            
                            delegate: BackgroundItem {
                                width: Theme.itemSizeLarge
                                height: Theme.itemSizeLarge
                                
                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    border.color: styleList.currentIndex === index ? Theme.highlightColor : "transparent"
                                    border.width: 2
                                    radius: Theme.paddingSmall
                                    
                                    Label {
                                        anchors.centerIn: parent
                                        text: modelData.icon
                                        font.pixelSize: Theme.fontSizeExtraLarge
                                    }
                                }
                                
                                onClicked: {
                                    styleList.currentIndex = index;
                                    debounceTimer.restart();
                                }
                            }
                        }
                        
                        // Style Intensity Slider
                        Slider {
                            id: intensitySlider
                            width: parent.width
                            label: qsTr("Интенсивность стиля")
                            minimumValue: 0.0
                            maximumValue: 1.0
                            value: 1.0
                            stepSize: 0.05
                            enabled: !pipelineManager.isProcessing && pipelineManager.hasImage
                            
                            onValueChanged: {
                                if (enabled && !debounceTimer.running) {
                                    // Don't re-run inference, just blend cached result
                                    pipelineManager.blendStyle(value);
                                }
                            }
                        }
                    }
                    
                    // Debounce timer for fast swiping through styles
                    Timer {
                        id: debounceTimer
                        interval: 400 // ms
                        onTriggered: {
                            if (styleList.currentIndex >= 0 && styleList.currentIndex < styleModels.length) {
                                intensitySlider.value = 1.0; // Reset intensity on new style
                                pipelineManager.applyStyle(styleModels[styleList.currentIndex].path);
                            }
                        }
                    }
                }
                
                // --- Enhance Tool ---
                Item {
                    anchors.fill: parent
                    visible: currentTab === 1
                    
                    Column {
                        width: parent.width
                        
                        TextSwitch {
                            id: swContrast
                            text: qsTr("Автоконтраст")
                            checked: true
                        }
                        TextSwitch {
                            id: swWB
                            text: qsTr("Баланс белого")
                            checked: true
                        }
                        TextSwitch {
                            id: swCLAHE
                            text: qsTr("CLAHE (Адаптивный контраст)")
                            checked: false
                        }
                        
                        Button {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Применить")
                            enabled: !pipelineManager.isProcessing
                            onClicked: {
                                pipelineManager.applyEnhance(swContrast.checked, swWB.checked, swCLAHE.checked, 2.0);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Notification banner
    Rectangle {
        id: infoBanner
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingLarge
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.9
        height: Theme.itemSizeMedium
        radius: Theme.paddingMedium
        color: Theme.highlightBackgroundColor
        opacity: 0.0
        z: 100
        
        property alias text: bannerText.text
        
        Label {
            id: bannerText
            anchors.centerIn: parent
            color: Theme.primaryColor
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeSmall
        }
        
        Behavior on opacity { FadeAnimation {} }
        
        Timer {
            id: bannerTimer
            interval: 3000
            onTriggered: infoBanner.opacity = 0.0
        }
        
        function show(msg) {
            text = msg;
            opacity = 0.9;
            bannerTimer.restart();
        }
    }
}
