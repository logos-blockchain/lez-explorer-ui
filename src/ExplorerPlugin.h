#pragma once

#include "IComponent.h"

#include <QObject>
#include <QtPlugin>

class ExplorerPlugin : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")

public:
    explicit ExplorerPlugin(QObject* parent = nullptr);

    Q_INVOKABLE QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;
    void destroyWidget(QWidget* widget) override;
};
