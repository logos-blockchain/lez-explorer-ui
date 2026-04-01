#include "mainwindow.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    // Determine the appropriate plugin extension based on the platform
    QString pluginExtension;
#if defined(Q_OS_WIN)
    pluginExtension = ".dll";
#elif defined(Q_OS_MAC)
    pluginExtension = ".dylib";
#else
    pluginExtension = ".so";
#endif

    QString pluginPath = QCoreApplication::applicationDirPath() + "/../lez_explorer_ui" + pluginExtension;
    QPluginLoader loader(pluginPath);

    QWidget* explorerWidget = nullptr;

    if (loader.load()) {
        QObject* plugin = loader.instance();
        if (plugin) {
            QMetaObject::invokeMethod(plugin, "createWidget",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(QWidget*, explorerWidget));
        }
    }

    if (explorerWidget) {
        setCentralWidget(explorerWidget);
    } else {
        qWarning() << "Failed to load LEZ Explorer UI plugin from:" << pluginPath;
        qWarning() << "Error:" << loader.errorString();

        auto* fallbackWidget = new QWidget(this);
        auto* layout = new QVBoxLayout(fallbackWidget);

        auto* messageLabel = new QLabel("LEZ Explorer UI module not loaded", fallbackWidget);
        QFont font = messageLabel->font();
        font.setPointSize(14);
        messageLabel->setFont(font);
        messageLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(messageLabel);
        setCentralWidget(fallbackWidget);
    }

    setWindowTitle("LEZ Explorer");
    resize(1024, 768);
}
