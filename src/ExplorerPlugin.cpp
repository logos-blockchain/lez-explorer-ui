#include "ExplorerPlugin.h"
#include "ExplorerWidget.h"

ExplorerPlugin::ExplorerPlugin(QObject* parent)
    : QObject(parent)
{
}

QWidget* ExplorerPlugin::createWidget(LogosAPI* logosAPI)
{
    return new ExplorerWidget(logosAPI);
}

void ExplorerPlugin::destroyWidget(QWidget* widget)
{
    delete widget;
}
