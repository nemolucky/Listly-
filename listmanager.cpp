#include "listmanager.h"
#include <authmanager.h>
#include <QSettings>
#include <QVariantMap>
#include <QCoreApplication>
#include <QNetworkReply>
#include <QJsonDocument>

ListManager::ListManager(QObject *parent) : QObject(parent), m_manager(new QNetworkAccessManager(this)){};

void ListManager::loadUserLists(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + AuthManager::instance().currentUser()->userId() + "/lists_metadata.json?auth=" + AuthManager::instance().currentUser()->token()));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
            sortListByColor(doc);
        }
        reply->deleteLater();
    });
}

void ListManager::createList(const QString& title, const QString& color) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString listId = "list_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QNetworkRequest listRequest(QUrl(databaseUrl + "/lists/" + listId + "/metadata.json?auth=" + AuthManager::instance().currentUser()->token()));
    listRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject listData;
    listData["title"] = title;
    listData["color"] = color;
    listData["owner"] =  AuthManager::instance().currentUser()->userId();
    QNetworkReply* listReply = m_manager->put(listRequest, QJsonDocument(listData).toJson());
    connect(listReply, &QNetworkReply::finished, [=]() {
        if (listReply->error() == QNetworkReply::NoError) {
            QNetworkRequest userRequest(QUrl(databaseUrl + "/users/" +  AuthManager::instance().currentUser()->userId() + "/lists_metadata/" + listId + ".json?auth=" + AuthManager::instance().currentUser()->token()));
            userRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QJsonObject list_metadata;
            list_metadata["title"] = title;
            list_metadata["color"] = color;
            QNetworkReply* userReply = m_manager->put(userRequest,QJsonDocument(list_metadata).toJson());
            connect(userReply, &QNetworkReply::finished, [=](){
                if (userReply->error() == QNetworkReply::NoError) {
                    emit listCreated(listData);
                } else {
                    emit listCreateFailed("Failed to update user lists");
                }
                userReply->deleteLater();
            });
        } else {
            emit listCreateFailed(listReply->errorString());
        }
        listReply->deleteLater();
    });
}

void ListManager::sortListByColor(const QJsonObject& lists){
    QVariantList result;

    QVariantMap highPriority, mediumPriority, lowPriority;
    highPriority["color"] = "#CD5D5D";
    mediumPriority["color"] = "#C4B55D";
    lowPriority["color"] = "#69C877";
    highPriority["items"] = QVariantList();
    mediumPriority["items"] = QVariantList();
    lowPriority["items"] = QVariantList();

    for (auto it = lists.begin(); it != lists.end(); ++it) {
        const QString listId = it.key();
        const QJsonObject list = it.value().toObject();
        const QString color = list["color"].toString();

        QVariantMap listData;
        listData["id"] = listId;
        listData["title"] = list["title"].toString();

        if (color == "#CD5D5D") {
            QVariantList items = highPriority["items"].toList();
            items.append(listData);
            highPriority["items"] = items;
        }
        else if (color == "#C4B55D") {
            QVariantList items = mediumPriority["items"].toList();
            items.append(listData);
            mediumPriority["items"] = items;
        }
        else if (color == "#69C877") {
            QVariantList items = lowPriority["items"].toList();
            items.append(listData);
            lowPriority["items"] = items;
        }
    }
    if (!highPriority["items"].toList().isEmpty())
        result.append(highPriority);
    if (!mediumPriority["items"].toList().isEmpty())
        result.append(mediumPriority);
    if (!lowPriority["items"].toList().isEmpty())
        result.append(lowPriority);
    emit userListsLoaded(result);
}

void ListManager:: deleteList(const QString& listId){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QNetworkRequest listRequest(QUrl(databaseUrl + "/lists/" + listId + "/metadata.json?auth=" + token));
    QNetworkReply* metadataReply = m_manager->get(listRequest);
    connect(metadataReply, &QNetworkReply::finished, [=](){
        if (metadataReply->error() == QNetworkReply::NoError){
            QJsonObject listData = QJsonDocument::fromJson(metadataReply->readAll()).object();
            metadataReply->deleteLater();
            if (listData["owner"].toString() != userId) {
                qDebug() << "У вас нет прав на удаление этого списка";
                return;
            }
            QNetworkRequest colabRequest(QUrl(databaseUrl + "/lists/" + listId + "/colab.json?auth=" + token));
            QNetworkReply* colabReply = m_manager->get(colabRequest);
            connect(colabReply, &QNetworkReply::finished, [=]() {
                QJsonObject colabData;
                if (colabReply->error() == QNetworkReply::NoError) {
                    colabData = QJsonDocument::fromJson(colabReply->readAll()).object();
                }
                colabReply->deleteLater();
                QVector<QNetworkReply*> pendingDeletes;
                for (auto it = colabData.begin(); it != colabData.end(); ++it) {
                    QString collaboratorId = it.key();
                    QNetworkRequest deleteRequest(QUrl(databaseUrl + "/users/" + collaboratorId + "/lists_metadata/" + listId + ".json?auth=" + token));
                    pendingDeletes.append(m_manager->deleteResource(deleteRequest));
                }
                QNetworkRequest deleteListRequest(QUrl(databaseUrl + "/lists/" + listId + ".json?auth=" + token));
                QNetworkReply* listReply = m_manager->deleteResource(deleteListRequest);
                QNetworkRequest deleteMetadataRequest(QUrl(databaseUrl + "/users/" + userId + "/lists_metadata/" + listId + ".json?auth=" + token));
                QNetworkReply* metadataReply = m_manager->deleteResource(deleteMetadataRequest);
                QVector<QNetworkReply*> allReplies = pendingDeletes;
                allReplies.append(listReply);
                allReplies.append(metadataReply);
                for (QNetworkReply* reply : allReplies) {
                    connect(reply, &QNetworkReply::finished, [=]() {
                        if (reply->error() != QNetworkReply::NoError) {
                            qDebug() << "Ошибка при удалении:" << reply->errorString();
                        }
                        reply->deleteLater();
                    });
                }
                emit listDeleted();
            });
        }else{
            qDebug() << metadataReply->errorString();
        }
    });
}

void ListManager::shareList(const QString& listId, const QString& username){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString token = AuthManager::instance().currentUser()->token();
    QNetworkRequest userRequest(QUrl(databaseUrl + "/users.json?auth=" + token));
    QNetworkReply *usersReply = m_manager->get(userRequest);
    connect(usersReply, &QNetworkReply::finished, [=](){
        if (usersReply->error() == QNetworkReply::NoError){
            QJsonObject allUsers = QJsonDocument::fromJson(usersReply->readAll()).object();
            QString targetUserId;
            for (auto it = allUsers.begin(); it != allUsers.end(); ++it) {
                QJsonObject user = it.value().toObject();
                if (user["username"].toString() == username) {
                    targetUserId = it.key();
                    break;
                }
            }
            usersReply->deleteLater();
            if (targetUserId.isEmpty()) {
                qDebug() << "Пользователь '" + username + "' не найден";
                return;
            }
            QNetworkRequest listRequest(QUrl(databaseUrl + "/lists/" + listId + "/metadata.json?auth=" + token));
            QNetworkReply* listReply = m_manager->get(listRequest);
            connect(listReply, &QNetworkReply::finished, [=](){
                if (listReply->error() == QNetworkReply::NoError){
                    QJsonObject listData = QJsonDocument::fromJson(listReply->readAll()).object();
                    listReply->deleteLater();
                    QJsonObject sharedListData;
                    sharedListData["title"] = listData["title"];
                    sharedListData["color"] = listData["color"];
                    sharedListData["isDefault"] = listData["isDefault"];
                    QNetworkRequest shareRequest(QUrl(databaseUrl + "/users/" + targetUserId + "/lists_metadata/" + listId + ".json?auth=" + token));
                    shareRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                    QNetworkReply* shareReply = m_manager->put(shareRequest, QJsonDocument(sharedListData).toJson());
                    connect(shareReply, &QNetworkReply::finished, [=]() {
                        if (shareReply->error() == QNetworkReply::NoError) {
                            shareReply->deleteLater();
                            QJsonObject colabUpdate;
                            colabUpdate[targetUserId] = true;
                            QNetworkRequest colabRequest(QUrl(databaseUrl + "/lists/" + listId + "/colab.json?auth=" + token));
                            colabRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                            QNetworkReply* colabReply = m_manager->put(colabRequest, QJsonDocument(colabUpdate).toJson());
                            connect(colabReply, &QNetworkReply::finished, [=]() {
                                if (colabReply->error() == QNetworkReply::NoError) {
                                    emit listShared();
                                }
                            });
                        }else{
                            qDebug() << shareReply->errorString();
                        }
                    });
                }
                else qDebug() << listReply->errorString();
            });
        }
        else{
            qDebug() << usersReply->errorString();
        }
    });
}

void ListManager::addItem(const QString& listId, const QString& title){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString token = AuthManager::instance().currentUser()->token();
    QString itemId = "item_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QNetworkRequest request(QUrl(databaseUrl + "/lists/" + listId + "/items/" + itemId + ".json?auth=" + token));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject itemData;
    itemData["title"] = title;
    itemData["isCompeleted"] = false;
    QNetworkReply *reply = m_manager->put(request, QJsonDocument(itemData).toJson());
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            emit itemAdded();
        }
    });
}

void ListManager::loadItems(const QString& listId){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString token = AuthManager::instance().currentUser()->token();
    QNetworkRequest request(QUrl(databaseUrl + "/lists/" + listId + "/items.json?auth=" + token));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
            QVariantList items;
            for (auto it = response.begin(); it != response.end(); ++it) {
                QVariantMap item;
                item["id"] = it.key();
                item["title"] = it.value().toObject()["title"].toString();
                item["isCompleted"] = it.value().toObject()["isCompeleted"].toBool();
                items.append(QVariant(item));
            }
            emit itemsLoaded(QVariant(items));
        }
        else{
            qDebug() << reply->errorString();
        }
    });
}

void ListManager::deleteItem(const QString& listId, const QString& itemId){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString token = AuthManager::instance().currentUser()->token();
    QNetworkRequest request(QUrl(databaseUrl + "/lists/" + listId + "/items/" + itemId + ".json?auth=" + token));
    QNetworkReply *reply = m_manager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            emit itemDeleted();
        }
    });
}

void ListManager::updateItemStatus(const QString& listId, const QString& itemId, const bool& state){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QNetworkRequest request(QUrl(databaseUrl + "/lists/" + listId + "/items/" + itemId + ".json?auth=" + token));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject itemData;
    itemData["isCompeleted"] = state;
    QNetworkReply *reply = m_manager->sendCustomRequest(request, "PATCH", QJsonDocument(itemData).toJson());
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            if (state){
                QNetworkRequest request(QUrl(databaseUrl + "/lists/" + listId + "/items/" + itemId + ".json?auth=" + token));
                QNetworkReply *reply = m_manager->get(request);
                connect(reply, &QNetworkReply::finished, [=](){
                    if (reply->error() == QNetworkReply::NoError){
                        QString purchaseId = "purchase_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
                        QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/historyOfPurchased/" + purchaseId + ".json?auth=" + token));
                        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
                        QJsonObject historyData;
                        historyData["title"] = response["title"].toString();
                        historyData["date"] = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
                        QNetworkReply *reply = m_manager->put(request, QJsonDocument(historyData).toJson());
                        connect(reply, &QNetworkReply::finished, [=](){
                            if (reply->error() == QNetworkReply::NoError){

                            }
                            reply->deleteLater();
                        });
                    }
                });
            }
        }
        reply->deleteLater();
    });
}
