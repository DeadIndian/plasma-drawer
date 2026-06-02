#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include "menueditorbackend.h"
#include "allappsmodel.h"

class PlasmaDrawerPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override
    {
        qmlRegisterType<MenuEditorBackend>(uri, 1, 0, "MenuEditorBackend");
        qmlRegisterType<AllAppsModel>(uri, 1, 0, "AllAppsModel");
    }
};

#include "plugin.moc"
