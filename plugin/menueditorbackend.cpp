#include "menueditorbackend.h"
#include <QProcess>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QProcess>
#include <KServiceGroup>
#include <KService>
#include <QFileInfo>
#include <KDesktopFile>
#include <KConfigGroup>
#include <QRegularExpression>

static QString resolveRealFolderName(const QString &displayName)
{
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
        
        QDomNodeList menus = doc.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement dirElem = menu.firstChildElement(QStringLiteral("Directory"));
            if (!dirElem.isNull()) {
                QString dirText = dirElem.text();
                QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/") + dirText;
                QFile dirFile(dirPath);
                if (dirFile.exists() && dirFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&dirFile);
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        if (line.startsWith(QStringLiteral("Name="))) {
                            if (line.mid(5).trimmed() == displayName) {
                                QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
                                if (!nameElem.isNull()) {
                                    return nameElem.text();
                                }
                            }
                            break;
                        }
                    }
                } else {
                    if (dirText == displayName + QStringLiteral(".directory")) {
                        QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
                        if (!nameElem.isNull()) return nameElem.text();
                    }
                }
            }
        }
    }
    return displayName;
}

static QString extractBaseName(const QString &urlStr, const QString &extension) {
    QString fileName = QUrl(urlStr).fileName();
    if (fileName.isEmpty()) fileName = urlStr;
    if (fileName.endsWith(extension)) {
        fileName.chop(extension.length());
    }
    return fileName;
}

static QString extractFileName(const QString &urlStr, const QString &extension) {
    QString fileName = QUrl(urlStr).fileName();
    if (fileName.isEmpty()) fileName = urlStr;
    if (!fileName.endsWith(extension)) {
        fileName += extension;
    }
    return fileName;
}

static QString resolveFolderId(const QString &folderId) {
    // 1. Search applications-kmenuedit.menu for a <Menu> whose <Directory> matches folderId's display name
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
        
        QDomElement root = doc.documentElement();
        if (!root.isNull()) {
            QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
            for (int i = 0; i < menus.size(); ++i) {
                QDomElement menu = menus.at(i).toElement();
                QDomElement dirElem = menu.firstChildElement(QStringLiteral("Directory"));
                QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
                if (!dirElem.isNull() && !nameElem.isNull()) {
                    QString dirFileName = dirElem.text();
                    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/") + dirFileName;
                    QFile dirFile(dirPath);
                    if (dirFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream in(&dirFile);
                        while (!in.atEnd()) {
                            QString line = in.readLine();
                            if (line.startsWith(QStringLiteral("Name="))) {
                                if (line.mid(5).trimmed() == folderId) {
                                    return nameElem.text();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 2. Fallback: Search ~/.local/share/desktop-directories/ (for system folders without <Directory> tag)
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/");
    QDir dir(dirPath, QStringLiteral("*.directory"));
    
    for (const QFileInfo &fileInfo : dir.entryInfoList()) {
        QFile dirFile(fileInfo.absoluteFilePath());
        if (dirFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&dirFile);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.startsWith(QStringLiteral("Name="))) {
                    if (line.mid(5).trimmed() == folderId) {
                        return fileInfo.completeBaseName();
                    }
                }
            }
        }
    }
    return folderId;
}

static void extractCategoriesRecursive(KServiceGroup::Ptr group, const QString &appId, QStringList &categories, const QString &topLevelName) {
    if (!group) return;
    for (auto entry : group->entries()) {
        if (entry->isType(KST_KServiceGroup)) {
            KServiceGroup::Ptr subGroup(static_cast<KServiceGroup*>(entry.data()));
            extractCategoriesRecursive(subGroup, appId, categories, topLevelName.isEmpty() ? subGroup->name() : topLevelName);
        } else if (entry->isType(KST_KService)) {
            KService::Ptr service(static_cast<KService*>(entry.data()));
            if (service && (service->storageId() == appId || service->desktopEntryName() + QStringLiteral(".desktop") == appId)) {
                if (!topLevelName.isEmpty() && !categories.contains(topLevelName)) {
                    categories << topLevelName;
                }
            }
        }
    }
}

static QStringList findAppCategories(const QString &appId) {
    QStringList categories;
    KServiceGroup::Ptr root = KServiceGroup::root();
    if (root) {
        extractCategoriesRecursive(root, appId, categories, QString());
    }
    
    // Fallback if KServiceGroup is broken (KF6)
    if (categories.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(appId);
        if (!service) {
            QString desktopName = appId;
            if (desktopName.endsWith(QStringLiteral(".desktop"))) {
                desktopName.chop(8);
            }
            KService::List all = KService::allServices();
            for (const KService::Ptr &svc : all) {
                if (svc->desktopEntryName() == desktopName) {
                    service = svc;
                    break;
                }
            }
        }
        
        if (service) {
            for (const QString &cat : service->categories()) {
                if (cat == QLatin1String("Network")) categories << QStringLiteral("Internet");
                else if (cat == QLatin1String("AudioVideo")) categories << QStringLiteral("Multimedia");
                else if (cat == QLatin1String("Utility")) categories << QStringLiteral("Utilities");
                else if (cat == QLatin1String("Game")) categories << QStringLiteral("Games");
                else categories << cat; // System, Graphics, Office, etc.
            }
        }
    }
    return categories;
}

MenuEditorBackend::MenuEditorBackend(QObject *parent) : QObject(parent)
{
}

void MenuEditorBackend::triggerKBuildSycoca()
{
    QProcess::startDetached(QStringLiteral("kbuildsycoca6"), QStringList());
}

void MenuEditorBackend::moveAppToFolder(const QString &appId, const QString &folderId, const QString &oldFolderId)
{

    
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    QString folderName = resolveRealFolderName(folderId);
    QString oldFolderName = oldFolderId.isEmpty() ? QString() : resolveRealFolderName(oldFolderId);// First, remove the app from any other custom folders so it doesn't exist in two places
    removeAppFromFolder(appFileName, oldFolderId);
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }
    
    // Remove from root's <Include> since removeAppFromFolder added it there
    QDomElement rootIncludeElem = root.firstChildElement(QStringLiteral("Include"));
    if (!rootIncludeElem.isNull()) {
        QDomNodeList filenames = rootIncludeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int i = filenames.size() - 1; i >= 0; --i) {
            QDomElement elem = filenames.at(i).toElement();
            if (elem.text() == appFileName) {
                rootIncludeElem.removeChild(elem);
            }
        }
    }
    
    QDomElement targetFolderElem;
    if (folderName.isEmpty()) {
        targetFolderElem = root;
    } else {
        // Find or create the target folder <Menu>
        QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == folderName) {
                targetFolderElem = menu;
                break;
            }
        }
        
        if (targetFolderElem.isNull()) {
            targetFolderElem = doc.createElement(QStringLiteral("Menu"));
            QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
            nameElem.appendChild(doc.createTextNode(folderName));
            targetFolderElem.appendChild(nameElem);
            root.appendChild(targetFolderElem);
        }
    }
    
    // Remove from target folder's <Exclude> just in case it was excluded
    QDomElement targetExcludeElem = targetFolderElem.firstChildElement(QStringLiteral("Exclude"));
    if (!targetExcludeElem.isNull()) {
        QDomNodeList filenames = targetExcludeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int i = filenames.size() - 1; i >= 0; --i) {
            QDomElement elem = filenames.at(i).toElement();
            if (elem.text() == appFileName) {
                targetExcludeElem.removeChild(elem);
            }
        }
    }
    
    // Add the <Include><Filename>
    QDomElement includeElem = targetFolderElem.firstChildElement(QStringLiteral("Include"));
    if (includeElem.isNull()) {
        includeElem = doc.createElement(QStringLiteral("Include"));
        targetFolderElem.appendChild(includeElem);
    }
    
    QDomElement filenameElem = doc.createElement(QStringLiteral("Filename"));
    filenameElem.appendChild(doc.createTextNode(appFileName));
    includeElem.appendChild(filenameElem);
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }

    triggerKBuildSycoca();
}

void MenuEditorBackend::removeAppFromFolder(const QString &appId, const QString &oldFolderId)
{

    
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    QString oldFolderName = resolveRealFolderName(oldFolderId);
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    
    QFile file(menuFile);
    QDomDocument doc(QStringLiteral("Menu"));
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }
    
    bool changed = false;
    QDomNodeList includes = doc.elementsByTagName(QStringLiteral("Include"));
    for (int i = 0; i < includes.size(); ++i) {
        QDomElement includeElem = includes.at(i).toElement();
        QDomNodeList filenames = includeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int j = filenames.size() - 1; j >= 0; --j) {
            QDomElement elem = filenames.at(j).toElement();
            if (elem.text() == appFileName) {
                includeElem.removeChild(elem);
                changed = true;
            }
        }
    }

    // Find all categories the app currently belongs to natively
    QStringList currentCategories = findAppCategories(appFileName);
    
    // If oldFolderId was explicitly passed (e.g., from a custom folder drop), 
    // ensure it's in the list of categories to exclude from
    if (!oldFolderName.isEmpty() && !currentCategories.contains(oldFolderName)) {
        currentCategories << oldFolderName;
    }
    
    for (const QString &categoryName : currentCategories) {
        QDomElement targetFolderElem;
        QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == categoryName) {
                targetFolderElem = menu;
                break;
            }
        }
        
        if (targetFolderElem.isNull()) {
            targetFolderElem = doc.createElement(QStringLiteral("Menu"));
            QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
            nameElem.appendChild(doc.createTextNode(categoryName));
            targetFolderElem.appendChild(nameElem);
            root.appendChild(targetFolderElem);
        }
        
        QDomElement excludeElem = targetFolderElem.firstChildElement(QStringLiteral("Exclude"));
        if (excludeElem.isNull()) {
            excludeElem = doc.createElement(QStringLiteral("Exclude"));
            targetFolderElem.appendChild(excludeElem);
        }
        
        QDomElement newFilename = doc.createElement(QStringLiteral("Filename"));
        newFilename.appendChild(doc.createTextNode(appFileName));
        excludeElem.appendChild(newFilename);
        changed = true;
    }
    
    // Include at root
    QDomElement rootIncludeElem = root.firstChildElement(QStringLiteral("Include"));
    if (rootIncludeElem.isNull()) {
        rootIncludeElem = doc.createElement(QStringLiteral("Include"));
        root.appendChild(rootIncludeElem);
    }
    QDomElement rootFilename = doc.createElement(QStringLiteral("Filename"));
    rootFilename.appendChild(doc.createTextNode(appFileName));
    rootIncludeElem.appendChild(rootFilename);
    changed = true;
    
    if (changed) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (doc.doctype().isNull()) {
                out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
            }
            out << doc.toString();
            file.close();
        }
        triggerKBuildSycoca();
    }
}

void MenuEditorBackend::renameFolder(const QString &folderId, const QString &newName)
{

    
    QString realFolderName = resolveRealFolderName(folderId);
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }
    
    QDomElement targetMenu;
    QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
    for (int i = 0; i < menus.size(); ++i) {
        QDomElement menu = menus.at(i).toElement();
        QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
        if (!nameElem.isNull() && nameElem.text() == realFolderName) {
            targetMenu = menu;
            break;
        }
    }
    
    if (targetMenu.isNull()) {
        targetMenu = doc.createElement(QStringLiteral("Menu"));
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(realFolderName));
        targetMenu.appendChild(nameElem);
        root.appendChild(targetMenu);
    }
    
    QString newDirFileName = newName + QStringLiteral(".directory");
    
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/");
    QDir().mkpath(dirPath);
    QString newFilePath = dirPath + newDirFileName;
    
    // Find old file to preserve Icon
    QString oldDirFileName = realFolderName + QStringLiteral(".directory");
    if (!targetMenu.isNull() && !targetMenu.firstChildElement(QStringLiteral("Directory")).isNull()) {
        QString oldDirText = targetMenu.firstChildElement(QStringLiteral("Directory")).text();
        if (oldDirText != newDirFileName) {
            oldDirFileName = oldDirText;
        }
    }
    
    QString oldFilePath = dirPath + oldDirFileName;
    if (!QFile::exists(oldFilePath)) {
        oldFilePath = QStringLiteral("/usr/share/desktop-directories/") + oldDirFileName;
    }
    
    QString content;
    bool nameFound = false;
    QFile oldFile(oldFilePath);
    if (oldFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&oldFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("Name="))) {
                content += QStringLiteral("Name=") + newName + QStringLiteral("\n");
                nameFound = true;
            } else {
                content += line + QStringLiteral("\n");
            }
        }
        oldFile.close();
    }
    
    if (!nameFound) {
        if (content.isEmpty()) {
            content = QStringLiteral("[Desktop Entry]\nIcon=folder\nType=Directory\n");
        }
        content += QStringLiteral("Name=") + newName + QStringLiteral("\n");
    }
    
    QFile newFile(newFilePath);
    if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&newFile);
        out << content;
        newFile.close();
    }
    
    // Update the Directory tag in the XML now that we read from the old one
    QDomElement dirElem = targetMenu.firstChildElement(QStringLiteral("Directory"));
    if (dirElem.isNull()) {
        dirElem = doc.createElement(QStringLiteral("Directory"));
        targetMenu.appendChild(dirElem);
    }
    if (dirElem.childNodes().size() > 0) {
        dirElem.firstChild().setNodeValue(newDirFileName);
    } else {
        dirElem.appendChild(doc.createTextNode(newDirFileName));
    }
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }
    
    triggerKBuildSycoca();
}

void MenuEditorBackend::renameApp(const QString &appId, const QString &newName)
{

    
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/applications/");
    QDir().mkpath(appPath);
    
    QString filePath = appPath + appFileName;
    
    QFile file(filePath);
    QString content;
    bool nameFound = false;
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("Name="))) {
                content += QStringLiteral("Name=") + newName + QStringLiteral("\n");
                nameFound = true;
            } else if (line == QStringLiteral("[Desktop%20Entry]")) {
                content += QStringLiteral("[Desktop Entry]\n");
            } else {
                content += line + QStringLiteral("\n");
            }
        }
        file.close();
    }
    
    if (!nameFound) {
        if (content.isEmpty()) {
            content = QStringLiteral("[Desktop Entry]\nType=Application\n");
        }
        content += QStringLiteral("Name=") + newName + QStringLiteral("\n");
    }
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
    }
    
    triggerKBuildSycoca();
}

void MenuEditorBackend::deleteFolder(const QString &folderId)
{

    
    QString folderName = resolveRealFolderName(folderId);
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    
    QFile file(menuFile);
    QDomDocument doc(QStringLiteral("Menu"));
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        return; // Nothing to do
    }
    
    QDomElement targetFolderElem;
    QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
    for (int i = 0; i < menus.size(); ++i) {
        QDomElement menu = menus.at(i).toElement();
        QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
        if (!nameElem.isNull() && nameElem.text() == folderName) {
            targetFolderElem = menu;
            break;
        }
    }
    
    if (targetFolderElem.isNull()) {
        targetFolderElem = doc.createElement(QStringLiteral("Menu"));
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(folderName));
        targetFolderElem.appendChild(nameElem);
        root.appendChild(targetFolderElem);
    }
    
    // Extract all included apps
    QStringList appsToMove;
    QDomElement includeElem = targetFolderElem.firstChildElement(QStringLiteral("Include"));
    if (!includeElem.isNull()) {
        QDomNodeList filenames = includeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int i = 0; i < filenames.size(); ++i) {
            appsToMove << filenames.at(i).toElement().text();
        }
    }
    

    
    // For system folders, extract native apps via recursive KServiceGroup search or KApplicationTrader fallback
    KServiceGroup::Ptr rootGroup = KServiceGroup::root();
    bool kserviceGroupFoundApps = false;
    if (rootGroup) {
        std::function<void(KServiceGroup::Ptr, bool)> extractApps = [&](KServiceGroup::Ptr group, bool isTarget) {
            bool currentIsTarget = isTarget || (group->name() == folderName) || (group->name() == folderName + QStringLiteral("/"));

            for (auto entry : group->entries(true, true)) {
                if (entry->isType(KST_KServiceGroup)) {
                    KServiceGroup::Ptr subGroup(static_cast<KServiceGroup*>(entry.data()));
                    extractApps(subGroup, currentIsTarget);
                } else if (entry->isType(KST_KService) && currentIsTarget) {
                    KService::Ptr service(static_cast<KService*>(entry.data()));
                    QString appFileName = service->desktopEntryName() + QStringLiteral(".desktop");
                    if (!appsToMove.contains(appFileName)) {
                        appsToMove << appFileName;
                        kserviceGroupFoundApps = true;
                    }
                }
            }
        };
        extractApps(rootGroup, false);
    }
    
    // Fallback: If KServiceGroup is empty (broken in KF6), use KApplicationTrader to map category directly
    if (!kserviceGroupFoundApps) {
        QStringList folderCategories;
        if (folderName == QLatin1String("Internet")) folderCategories << QStringLiteral("Network");
        else if (folderName == QLatin1String("Multimedia")) folderCategories << QStringLiteral("AudioVideo");
        else if (folderName == QLatin1String("Utilities")) folderCategories << QStringLiteral("Utility");
        else if (folderName == QLatin1String("Games")) folderCategories << QStringLiteral("Game");
        else folderCategories << folderName; // System, Graphics, Office, etc.

        KService::List services = KService::allServices();
        for (const KService::Ptr &service : services) {
            bool matches = false;
            for (const QString &cat : folderCategories) {
                if (service->categories().contains(cat)) {
                    matches = true;
                    break;
                }
            }
            if (matches) {
                QString appFileName = service->desktopEntryName() + QStringLiteral(".desktop");
                if (!appsToMove.contains(appFileName)) {
                    appsToMove << appFileName;
                }
            }
        }
    }
    

    
    // Mark as deleted for system folders, or remove entirely if custom
    if (targetFolderElem.firstChildElement(QStringLiteral("Deleted")).isNull()) {
        targetFolderElem.appendChild(doc.createElement(QStringLiteral("Deleted")));
    }
    
    // Remove the <Menuname> from all <Layout> elements
    QDomNodeList layouts = root.elementsByTagName(QStringLiteral("Layout"));
    for (int i = 0; i < layouts.size(); ++i) {
        QDomElement layoutElem = layouts.at(i).toElement();
        QDomNodeList menunames = layoutElem.elementsByTagName(QStringLiteral("Menuname"));
        for (int j = menunames.size() - 1; j >= 0; --j) {
            QDomElement menunameElem = menunames.at(j).toElement();
            if (menunameElem.text() == folderName) {
                layoutElem.removeChild(menunameElem);
            }
        }
    }
    
    // Ensure root Include element exists
    QDomElement rootIncludeElem = root.firstChildElement(QStringLiteral("Include"));
    if (rootIncludeElem.isNull()) {
        rootIncludeElem = doc.createElement(QStringLiteral("Include"));
        root.appendChild(rootIncludeElem);
    }
    
    // For each app, exclude from native categories and include at root
    for (const QString &appFileName : appsToMove) {
        if (appFileName == QLatin1String("plasma-drawer-dummy.desktop")) continue;
        QStringList currentCategories = findAppCategories(appFileName);
        
        if (!currentCategories.contains(folderName)) {
            currentCategories << folderName;
        }
        
        for (const QString &categoryName : currentCategories) {
            QDomElement catMenu;
            QDomNodeList allMenus = root.elementsByTagName(QStringLiteral("Menu"));
            for (int i = 0; i < allMenus.size(); ++i) {
                QDomElement menu = allMenus.at(i).toElement();
                QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
                if (!nameElem.isNull() && nameElem.text() == categoryName) {
                    catMenu = menu;
                    break;
                }
            }
            
            if (catMenu.isNull()) {
                catMenu = doc.createElement(QStringLiteral("Menu"));
                QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
                nameElem.appendChild(doc.createTextNode(categoryName));
                catMenu.appendChild(nameElem);
                root.appendChild(catMenu);
            }
            
            QDomElement excludeElem = catMenu.firstChildElement(QStringLiteral("Exclude"));
            if (excludeElem.isNull()) {
                excludeElem = doc.createElement(QStringLiteral("Exclude"));
                catMenu.appendChild(excludeElem);
            }
            
            QDomElement newFilename = doc.createElement(QStringLiteral("Filename"));
            newFilename.appendChild(doc.createTextNode(appFileName));
            excludeElem.appendChild(newFilename);
        }
        
        // Include at root
        QDomElement rootFilename = doc.createElement(QStringLiteral("Filename"));
        rootFilename.appendChild(doc.createTextNode(appFileName));
        rootIncludeElem.appendChild(rootFilename);
    }
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }
    
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/");
    QString filePath = dirPath + folderName + QStringLiteral(".directory");
    QFile::remove(filePath);
    
    QString displayFilePath = dirPath + folderId + QStringLiteral(".directory");
    QFile::remove(displayFilePath);
    
    QDomElement dirElem = targetFolderElem.firstChildElement(QStringLiteral("Directory"));
    if (!dirElem.isNull()) {
        QString customDirFile = dirPath + dirElem.text();
        QFile::remove(customDirFile);
    }
    
    triggerKBuildSycoca();
}

void MenuEditorBackend::resetToDefault()
{
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QFile::remove(menuFile);
    triggerKBuildSycoca();
}

void MenuEditorBackend::createFolder(const QString &folderName, const QString &parentFolderId)
{
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/applications/");
    QDir().mkpath(appPath);
    QString dummyApp = appPath + QStringLiteral("plasma-drawer-dummy.desktop");
    if (!QFile::exists(dummyApp)) {
        QFile file(dummyApp);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Name=Plasma Drawer Dummy\n"
                << "Exec=true\n"
                << "Icon=system-run\n"
                << "Type=Application\n"
                << "NoDisplay=false\n";
            file.close();
        }
    }

    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/desktop-directories/");
    QDir().mkpath(dirPath);
    QString dirFileName = folderName + QStringLiteral(".directory");
    QString dirFilePath = dirPath + dirFileName;

    int counter = 1;
    QString baseFolderName = folderName;
    QString finalFolderName = folderName;
    while (QFile::exists(dirFilePath)) {
        finalFolderName = baseFolderName + QStringLiteral(" ") + QString::number(counter++);
        dirFileName = finalFolderName + QStringLiteral(".directory");
        dirFilePath = dirPath + dirFileName;
    }

    QFile dirFile(dirFilePath);
    if (dirFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&dirFile);
        out << "[Desktop Entry]\n"
            << "Icon=folder\n"
            << "Type=Directory\n"
            << "Name=" << finalFolderName << "\n";
        dirFile.close();
    }

    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }

    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }

    QDomElement parentMenuElem = root;
    if (!parentFolderId.isEmpty()) {
        QString parentFolderName = resolveRealFolderName(parentFolderId);
        
        QDomNodeList menus = doc.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == parentFolderName) {
                parentMenuElem = menu;
                break;
            }
        }
    }

    QDomElement existingMenu;
    QDomNodeList childNodes = parentMenuElem.childNodes();
    for (int i = 0; i < childNodes.size(); ++i) {
        QDomNode n = childNodes.at(i);
        if (n.isElement() && n.toElement().tagName() == QStringLiteral("Menu")) {
            QDomElement nameElem = n.toElement().firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == finalFolderName) {
                existingMenu = n.toElement();
                break;
            }
        }
    }

    if (!existingMenu.isNull()) {
        QDomElement deletedElem = existingMenu.firstChildElement(QStringLiteral("Deleted"));
        if (!deletedElem.isNull()) {
            existingMenu.removeChild(deletedElem);
        }
        
        QDomElement includeElem = existingMenu.firstChildElement(QStringLiteral("Include"));
        if (includeElem.isNull()) {
            includeElem = doc.createElement(QStringLiteral("Include"));
            existingMenu.appendChild(includeElem);
        }
        QDomElement filenameElem = doc.createElement(QStringLiteral("Filename"));
        filenameElem.appendChild(doc.createTextNode(QStringLiteral("plasma-drawer-dummy.desktop")));
        includeElem.appendChild(filenameElem);
    } else {
        QDomElement newMenuElem = doc.createElement(QStringLiteral("Menu"));
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(finalFolderName));
        newMenuElem.appendChild(nameElem);

        QDomElement includeElem = doc.createElement(QStringLiteral("Include"));
        QDomElement filenameElem = doc.createElement(QStringLiteral("Filename"));
        filenameElem.appendChild(doc.createTextNode(QStringLiteral("plasma-drawer-dummy.desktop")));
        includeElem.appendChild(filenameElem);
        newMenuElem.appendChild(includeElem);

        parentMenuElem.appendChild(newMenuElem);
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }

    triggerKBuildSycoca();
}

void MenuEditorBackend::deleteApp(const QString &appId, const QString &folderId)
{
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }
    
    QString realFolderName;
    if (!folderId.isEmpty()) {
        realFolderName = resolveRealFolderName(folderId);
    }

    QDomElement targetMenu;
    if (!realFolderName.isEmpty()) {
        QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == realFolderName) {
                targetMenu = menu;
                break;
            }
        }
        if (targetMenu.isNull()) {
            targetMenu = doc.createElement(QStringLiteral("Menu"));
            QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
            nameElem.appendChild(doc.createTextNode(realFolderName));
            targetMenu.appendChild(nameElem);
            root.appendChild(targetMenu);
        }
    } else {
        targetMenu = root;
    }
    
    bool changed = false;
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString localAppPath = appPath + QDir::separator() + appFileName;
    QRegularExpression copyRegex(QStringLiteral("-copy(?:-\\d+)?\\.desktop$"));
    bool isCopy = copyRegex.match(appFileName).hasMatch();

    if (isCopy) {
        QFile::remove(localAppPath);
        
        // Completely scrub from XML to avoid orphaned tags
        QDomNodeList allIncludes = root.elementsByTagName(QStringLiteral("Include"));
        for (int i = 0; i < allIncludes.size(); ++i) {
            QDomElement incElem = allIncludes.at(i).toElement();
            QDomNodeList files = incElem.elementsByTagName(QStringLiteral("Filename"));
            for (int j = files.size() - 1; j >= 0; --j) {
                if (files.at(j).toElement().text() == appFileName) {
                    incElem.removeChild(files.at(j));
                    changed = true;
                }
            }
        }
        QDomNodeList allExcludes = root.elementsByTagName(QStringLiteral("Exclude"));
        for (int i = 0; i < allExcludes.size(); ++i) {
            QDomElement excElem = allExcludes.at(i).toElement();
            QDomNodeList files = excElem.elementsByTagName(QStringLiteral("Filename"));
            for (int j = files.size() - 1; j >= 0; --j) {
                if (files.at(j).toElement().text() == appFileName) {
                    excElem.removeChild(files.at(j));
                    changed = true;
                }
            }
        }
    } else {
        QDomElement includeElem = targetMenu.firstChildElement(QStringLiteral("Include"));
        if (!includeElem.isNull()) {
            QDomNodeList filenames = includeElem.elementsByTagName(QStringLiteral("Filename"));
            for (int j = filenames.size() - 1; j >= 0; --j) {
                QDomElement elem = filenames.at(j).toElement();
                if (elem.text() == appFileName) {
                    includeElem.removeChild(elem);
                    changed = true;
                }
            }
        }
        
        QDomElement excludeElem = targetMenu.firstChildElement(QStringLiteral("Exclude"));
        if (excludeElem.isNull()) {
            excludeElem = doc.createElement(QStringLiteral("Exclude"));
            targetMenu.appendChild(excludeElem);
        }
        
        bool alreadyExcluded = false;
        QDomNodeList excludedFiles = excludeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int i = 0; i < excludedFiles.size(); ++i) {
            if (excludedFiles.at(i).toElement().text() == appFileName) {
                alreadyExcluded = true;
                break;
            }
        }
        
        if (!alreadyExcluded) {
            QDomElement newFilename = doc.createElement(QStringLiteral("Filename"));
            newFilename.appendChild(doc.createTextNode(appFileName));
            excludeElem.appendChild(newFilename);
            changed = true;
        }
    }
    
    if (changed && file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }
    
    triggerKBuildSycoca();
}

void MenuEditorBackend::duplicateApp(const QString &appId, const QString &folderId)
{
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    KService::Ptr service = KService::serviceByStorageId(appFileName);
    if (!service) {
        KService::List all = KService::allServices();
        for (const KService::Ptr &svc : all) {
            if (svc->desktopEntryName() + QStringLiteral(".desktop") == appFileName) {
                service = svc;
                break;
            }
        }
    }
    
    if (!service) return;
    
    QString origPath = service->entryPath();
    QString baseName = extractBaseName(appFileName, QStringLiteral(".desktop"));
    
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(appPath);
    
    int counter = 1;
    QString newFileName = baseName + QStringLiteral("-copy.desktop");
    QString newName = service->name() + QStringLiteral(" Copy");
    while (QFile::exists(appPath + QDir::separator() + newFileName)) {
        newFileName = baseName + QStringLiteral("-copy-") + QString::number(counter) + QStringLiteral(".desktop");
        newName = service->name() + QStringLiteral(" Copy ") + QString::number(counter);
        counter++;
    }
    
    QString localAppPath = appPath + QDir::separator() + newFileName;
    
    if (QFile::copy(origPath, localAppPath)) {
        QFile::setPermissions(localAppPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
        KDesktopFile desktopFile(localAppPath);
        KConfigGroup cg = desktopFile.desktopGroup();
        cg.writeEntry("Name", newName);
        cg.writeEntry("Categories", "PlasmaDrawerHidden;");
        desktopFile.sync();
    }
    
    QString menuFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/menus/applications-kmenuedit.menu");
    QDomDocument doc(QStringLiteral("Menu"));
    QFile file(menuFile);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("Menu"));
        doc.appendChild(root);
        QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
        nameElem.appendChild(doc.createTextNode(QStringLiteral("Applications")));
        root.appendChild(nameElem);
    }
    
    QString realFolderName;
    if (!folderId.isEmpty()) {
        realFolderName = resolveRealFolderName(folderId);
    }
    
    QDomElement targetMenu;
    if (!realFolderName.isEmpty()) {
        QDomNodeList menus = root.elementsByTagName(QStringLiteral("Menu"));
        for (int i = 0; i < menus.size(); ++i) {
            QDomElement menu = menus.at(i).toElement();
            QDomElement nameElem = menu.firstChildElement(QStringLiteral("Name"));
            if (!nameElem.isNull() && nameElem.text() == realFolderName) {
                targetMenu = menu;
                break;
            }
        }
        if (targetMenu.isNull()) {
            targetMenu = doc.createElement(QStringLiteral("Menu"));
            QDomElement nameElem = doc.createElement(QStringLiteral("Name"));
            nameElem.appendChild(doc.createTextNode(realFolderName));
            targetMenu.appendChild(nameElem);
            root.appendChild(targetMenu);
        }
    } else {
        targetMenu = root;
    }
    
    QDomElement includeElem = targetMenu.firstChildElement(QStringLiteral("Include"));
    if (includeElem.isNull()) {
        includeElem = doc.createElement(QStringLiteral("Include"));
        targetMenu.appendChild(includeElem);
    }
    
    // Remove from Exclude if it's there
    QDomElement excludeElem = targetMenu.firstChildElement(QStringLiteral("Exclude"));
    if (!excludeElem.isNull()) {
        QDomNodeList excludedFiles = excludeElem.elementsByTagName(QStringLiteral("Filename"));
        for (int i = excludedFiles.size() - 1; i >= 0; --i) {
            if (excludedFiles.at(i).toElement().text() == newFileName) {
                excludeElem.removeChild(excludedFiles.at(i));
            }
        }
    }
    
    QDomElement newFilenameElem = doc.createElement(QStringLiteral("Filename"));
    newFilenameElem.appendChild(doc.createTextNode(newFileName));
    includeElem.appendChild(newFilenameElem);
    
    // Exclude from root if it's placed in a specific folder, to prevent it showing in the root screen
    if (targetMenu != root) {
        QDomElement excludeRoot = root.firstChildElement(QStringLiteral("Exclude"));
        if (excludeRoot.isNull()) {
            excludeRoot = doc.createElement(QStringLiteral("Exclude"));
            root.appendChild(excludeRoot);
        }
        QDomElement newFilenameElemRoot = doc.createElement(QStringLiteral("Filename"));
        newFilenameElemRoot.appendChild(doc.createTextNode(newFileName));
        excludeRoot.appendChild(newFilenameElemRoot);
    }
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (doc.doctype().isNull()) {
            out << "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' 'http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd'>\n";
        }
        out << doc.toString();
        file.close();
    }
    
    triggerKBuildSycoca();
}

bool MenuEditorBackend::hasMoreThanOneInstance(const QString &appId)
{
    QString appFileName = extractFileName(appId, QStringLiteral(".desktop"));
    QRegularExpression copyRegex(QStringLiteral("-copy(?:-\\d+)?\\.desktop$"));
    QString baseName = appFileName;
    if (copyRegex.match(appFileName).hasMatch()) {
        baseName = appFileName;
        baseName.replace(copyRegex, QStringLiteral(".desktop"));
    }
    
    QString basePrefix = baseName;
    if (basePrefix.endsWith(QStringLiteral(".desktop"))) {
        basePrefix.chop(8);
    }
    
    int count = 0;
    KService::List all = KService::allServices();
    for (const KService::Ptr &svc : all) {
        QString entryName = svc->desktopEntryName();
        if (entryName == basePrefix || entryName.startsWith(basePrefix + QStringLiteral("-copy"))) {
            if (!svc->noDisplay()) {
                count++;
            }
        }
    }
    return count > 1;
}
