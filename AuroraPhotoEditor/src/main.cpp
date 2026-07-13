#include <QtQuick>
#include <auroraapp.h>
#include <QQmlContext>
#include "SegmentationEngine.h"
#include "ImageProcessor.h"
#include "EnhanceEngine.h"
#include "StyleTransferEngine.h"
#include "PipelineManager.h"
#include "EditorImageProvider.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> application(Aurora::Application::application(argc, argv));
    application->setOrganizationName(QStringLiteral("ru.template"));
    application->setApplicationName(QStringLiteral("AuroraPhotoEditor"));

    qmlRegisterType<SegmentationEngine>("ru.template.AuroraPhotoEditor", 1, 0, "SegmentationEngine");
    qmlRegisterType<ImageProcessor>("ru.template.AuroraPhotoEditor", 1, 0, "ImageProcessor");
    qmlRegisterType<EnhanceEngine>("ru.template.AuroraPhotoEditor", 1, 0, "EnhanceEngine");
    qmlRegisterType<StyleTransferEngine>("ru.template.AuroraPhotoEditor", 1, 0, "StyleTransferEngine");

    // Initialize the PipelineManager
    QScopedPointer<PipelineManager> pipelineManager(new PipelineManager());

    QScopedPointer<QQuickView> view(Aurora::Application::createView());
    
    // Register PipelineManager as a context property
    view->engine()->rootContext()->setContextProperty("pipelineManager", pipelineManager.data());
    
    // Register the image provider
    view->engine()->addImageProvider("editor", new EditorImageProvider(pipelineManager.data()));

    view->setSource(Aurora::Application::pathTo(QStringLiteral("qml/AuroraPhotoEditor.qml")));
    view->show();

    return application->exec();
}
