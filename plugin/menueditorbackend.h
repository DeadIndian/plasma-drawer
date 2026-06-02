#ifndef MENUEDITORBACKEND_H
#define MENUEDITORBACKEND_H

#include <QObject>
#include <QString>

class MenuEditorBackend : public QObject
{
    Q_OBJECT
public:
    explicit MenuEditorBackend(QObject *parent = nullptr);

    Q_INVOKABLE void moveAppToFolder(const QString &appId, const QString &folderId, const QString &oldFolderId = QString());
    Q_INVOKABLE void removeAppFromFolder(const QString &appId, const QString &oldFolderId = QString());
    Q_INVOKABLE void renameFolder(const QString &folderId, const QString &newName);
    Q_INVOKABLE void deleteFolder(const QString &folderId);
    Q_INVOKABLE void renameApp(const QString &appId, const QString &newName);
    Q_INVOKABLE void createFolder(const QString &folderName, const QString &parentFolderId = QString());
    Q_INVOKABLE void resetToDefault();
    Q_INVOKABLE void deleteApp(const QString &appId, const QString &folderId = QString());
    Q_INVOKABLE void duplicateApp(const QString &appId, const QString &folderId = QString());
    Q_INVOKABLE bool hasMoreThanOneInstance(const QString &appId);
private:
    void triggerKBuildSycoca();
};

#endif // MENUEDITORBACKEND_H
