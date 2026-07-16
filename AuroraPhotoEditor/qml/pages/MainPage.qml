import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import "../"
import "../components"

Page {
    id: mainPage
    objectName: "mainPage"
    allowedOrientations: Orientation.All

    // Label shown under the busy spinner — set right before triggering the
    // matching pipelineManager call, so it stays correct even if a later
    // tool supersedes the running one (see operationCanceled handling below).
    property string currentOperationLabel: qsTr("Processing")

    // Extra bottom margin while a bottom sheet ("Фон"/"Стиль") is open, so
    // the photo shrinks to fit above it instead of being covered — see
    // BottomSheet's capped height (0.5 of screen, was 0.88).
    readonly property real openSheetHeight: Math.max(
        backgroundSheet.open ? backgroundSheet.height : 0,
        styleSheet.open ? styleSheet.height : 0)

    // True from the moment a photo is picked until it actually appears.
    // loadFromUri() runs on the UI thread, so nothing can animate during
    // the read itself — this just avoids a blank frame right before/after
    // that blocking call instead of a jump-cut from empty to loaded.
    property bool photoLoading: false

    Rectangle {
        anchors.fill: parent
        color: NeonTheme.bgBase
    }

    PageHeaderBar {
        id: header
        objectName: "pageHeader"
        // Brand name — intentionally not qsTr()'d, shown as-is in every locale.
        title: "ZeroPhotos"

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
            iconName: "save-black"
            bgColor: "#EC4899"
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
            backgroundColor: NeonTheme.bgSurface

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
        }

        // Empty state
        Column {
            id: placeholder
            anchors.centerIn: parent
            spacing: NeonTheme.paddingLarge
            visible: !pipelineManager.hasImage && !mainPage.photoLoading
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

        // Shown from the moment a photo is picked until it actually appears,
        // instead of a blank area (only when there's no previous photo
        // already on screen — re-picking over an existing photo just lets
        // the new one fade in directly, no need to flash a skeleton over it).
        ShimmerPlaceholder {
            anchors.fill: parent
            anchors.margins: NeonTheme.paddingLarge
            anchors.bottomMargin: Math.max(toolsPanel.height, mainPage.openSheetHeight) + NeonTheme.paddingLarge
            running: mainPage.photoLoading && !pipelineManager.hasImage
        }

        // Loaded state
        Image {
            id: selectedImage
            anchors.fill: parent
            anchors.margins: NeonTheme.paddingLarge
            // The sheet renders on top of (not below) the tool panel, so only
            // the taller of the two needs to be reserved — not both stacked.
            anchors.bottomMargin: Math.max(toolsPanel.height, mainPage.openSheetHeight) + NeonTheme.paddingLarge
            fillMode: Image.PreserveAspectFit
            visible: pipelineManager.hasImage
            opacity: visible ? 1.0 : 0.0
            cache: false // Prevent memory leaks from timestamp updates

            Behavior on anchors.bottomMargin {
                NumberAnimation { duration: NeonTheme.fadeDuration; easing.type: Easing.InOutQuad }
            }

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
            // Centered on the photo's own (possibly shrunk) bounds, not the
            // whole flickable — otherwise the spinner sits in empty space
            // once the image shrinks to fit above an open bottom sheet.
            anchors.fill: selectedImage
            running: pipelineManager.isProcessing
            operationLabel: mainPage.currentOperationLabel
        }
    }

    DockedPanel {
        id: toolsPanel
        width: parent.width
        height: NeonTheme.toolPanelHeight
        dock: Dock.Bottom
        open: pipelineManager.hasImage

        Rectangle {
            anchors.fill: parent
            color: NeonTheme.bgBase
        }

        Row {
            id: toolsRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: NeonTheme.paddingMedium
            anchors.rightMargin: NeonTheme.paddingMedium
            anchors.verticalCenter: parent.verticalCenter
            spacing: NeonTheme.paddingMedium

            ToolButton {
                width: (toolsRow.width - NeonTheme.paddingMedium * 2) / 3
                height: NeonTheme.px(132)
                label: qsTr("Фон")
                iconName: "layers"
                accent: NeonTheme.accentPurple
                enabled: !pipelineManager.isProcessing
                onClicked: {
                    mainPage.currentOperationLabel = qsTr("Фон")
                    pipelineManager.applyBackgroundRemoval()
                    backgroundSheet.show()
                }
            }
            ToolButton {
                width: (toolsRow.width - NeonTheme.paddingMedium * 2) / 3
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
                width: (toolsRow.width - NeonTheme.paddingMedium * 2) / 3
                height: NeonTheme.px(132)
                label: qsTr("Стиль")
                iconName: "palette"
                accent: NeonTheme.accentPink
                enabled: !pipelineManager.isProcessing
                onClicked: styleSheet.show()
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
                mainPage.photoLoading = false
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

        onLoadFailed: {
            mainPage.photoLoading = false
            toast.show(reason, "error")
        }
        onOperationCanceled: toast.show(qsTr("Предыдущая операция отменена"), "info")
    }

    Component {
        id: imagePickerComponent
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                if (selectedContentProperties.filePath) {
                    mainPage.photoLoading = true
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
    }

    StyleSheet {
        id: styleSheet
    }
}
