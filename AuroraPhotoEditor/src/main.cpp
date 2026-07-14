#include <QtQuick>
#include <auroraapp.h>

#include "PipelineImageProvider.h"
#include "PipelineManager.h"
#include <QQmlContext>
#include <QQmlEngine>

int main(int argc, char *argv[]) {
  QScopedPointer<QGuiApplication> application(
      Aurora::Application::application(argc, argv));
  application->setOrganizationName(QStringLiteral("ru.template"));
  application->setApplicationName(QStringLiteral("AuroraPhotoEditor"));

  QScopedPointer<QQuickView> view(Aurora::Application::createView());

  PipelineManager pipelineManager;
  view->rootContext()->setContextProperty("pipelineManager", &pipelineManager);
  view->engine()->addImageProvider(QStringLiteral("pipeline"),
                                   new PipelineImageProvider(&pipelineManager));

  view->setSource(
      Aurora::Application::pathTo(QStringLiteral("qml/AuroraPhotoEditor.qml")));
  view->show();

  return application->exec();
}
