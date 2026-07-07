#include <QtQuick>
#include <auroraapp.h>
#include "SegmentationEngine.h"
#include "ImageProcessor.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> application(Aurora::Application::application(argc, argv));
    application->setOrganizationName(QStringLiteral("ru.template"));
    application->setApplicationName(QStringLiteral("AuroraPhotoEditor"));

    qmlRegisterType<SegmentationEngine>("ru.template.AuroraPhotoEditor", 1, 0, "SegmentationEngine");
    qmlRegisterType<ImageProcessor>("ru.template.AuroraPhotoEditor", 1, 0, "ImageProcessor");

    QScopedPointer<QQuickView> view(Aurora::Application::createView());
    view->setSource(Aurora::Application::pathTo(QStringLiteral("qml/AuroraPhotoEditor.qml")));
    view->show();

    return application->exec();
}
