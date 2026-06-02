#ifndef ALLAPPSMODEL_H
#define ALLAPPSMODEL_H

#include <QAbstractListModel>
#include <QVariant>
#include <KService>

class AllAppsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)
public:
    explicit AllAppsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int count() const { return m_apps.count(); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum Roles {
        DisplayRole = Qt::DisplayRole,
        DecorationRole = Qt::DecorationRole,
        UrlRole = Qt::UserRole + 1,
        FavoriteIdRole,
        HasChildrenRole,
        HasActionListRole,
        ActionListRole
    };

    Q_INVOKABLE QObject* modelForRow(int row);

private:
    KService::List m_apps;
};

#endif // ALLAPPSMODEL_H
