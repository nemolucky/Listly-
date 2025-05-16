#ifndef LISTMANAGER_H
#define LISTMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QNetworkAccessManager>

class ListManager : public QObject
{
    Q_OBJECT
public:
    explicit ListManager(QObject *parent = nullptr);
    Q_INVOKABLE void loadUserLists();
    Q_INVOKABLE void createList(const QString& title, const QString& color);
    Q_INVOKABLE void sortListByColor(const QJsonObject& lists);
    Q_INVOKABLE void deleteList(const QString& listId);
    Q_INVOKABLE void shareList(const QString& listId, const QString& username);
    Q_INVOKABLE void addItem(const QString& listId, const QString& title);
    Q_INVOKABLE void loadItems(const QString& listId);
    Q_INVOKABLE void deleteItem(const QString& listId, const QString& itemId);
    Q_INVOKABLE void updateItemStatus(const QString& listId, const QString& itemId, const bool& state);
private:
    QNetworkAccessManager *m_manager;
signals:
    void userListsLoaded(QVariantList data);
    void listCreated(const QJsonObject& data);
    void listCreateFailed(const QString& error);
    void listDeleted();
    void listShared();
    void itemAdded();
    void itemsLoaded(QVariant data);
    void itemDeleted();
};

#endif // LISTMANAGER_H
