#include "allappsmodel.h"
#include <KService>
#include <QUrl>

AllAppsModel::AllAppsModel(QObject *parent) : QAbstractListModel(parent)
{
    KService::List all = KService::allServices();
    for (const KService::Ptr &service : all) {
        if (service->isApplication() && !service->noDisplay() && service->name() != QLatin1String("Plasma Drawer Dummy")) {
            m_apps.append(service);
        }
    }
    std::sort(m_apps.begin(), m_apps.end(), [](const KService::Ptr &a, const KService::Ptr &b) {
        return a->name().compare(b->name(), Qt::CaseInsensitive) < 0;
    });
}

int AllAppsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_apps.count();
}

QVariant AllAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_apps.count()) return QVariant();

    KService::Ptr service = m_apps.at(index.row());
    if (role == DisplayRole) {
        return service->name();
    } else if (role == DecorationRole) {
        return service->icon();
    } else if (role == UrlRole) {
        return QUrl::fromLocalFile(service->entryPath());
    } else if (role == FavoriteIdRole) {
        return service->storageId();
    } else if (role == HasChildrenRole) {
        return false;
    } else if (role == HasActionListRole) {
        return false;
    } else if (role == ActionListRole) {
        return QVariantList();
    }

    return QVariant();
}

QHash<int, QByteArray> AllAppsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DisplayRole] = "display";
    roles[DecorationRole] = "decoration";
    roles[UrlRole] = "url";
    roles[FavoriteIdRole] = "favoriteId";
    roles[HasChildrenRole] = "hasChildren";
    roles[HasActionListRole] = "hasActionList";
    roles[ActionListRole] = "actionList";
    return roles;
}

QObject* AllAppsModel::modelForRow(int row)
{
    Q_UNUSED(row)
    return nullptr;
}
