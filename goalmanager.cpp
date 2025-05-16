#include "goalmanager.h"
#include "authmanager.h"
#include <QSettings>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

GoalManager::GoalManager(QObject *parent) : QObject(parent), m_manager(new QNetworkAccessManager(this)){};

void GoalManager::loadUserGoals(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/goals.json?auth=" + token));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject goalsObj = doc.object();
            QVariantList goalsList;
            for (auto it = goalsObj.begin(); it != goalsObj.end(); ++it) {
                QString goalId = it.key();
                QJsonObject goalObj = it.value().toObject();
                QVariantMap goalMap;
                goalMap["id"] = goalId;
                goalMap["title"] = goalObj["title"].toString();
                goalMap["target"] = goalObj["target"].toDouble();
                goalMap["date"] = goalObj["date"].toString();
                goalMap["progress"] = goalObj["progress"].toDouble();
                goalMap["history"] = goalObj["history"].toVariant();
                goalsList.append(goalMap);
            }
            emit userGoalsLoaded(goalsList);
        }
        reply->deleteLater();
    });
}

void GoalManager::createGoal(const QString& title, const QString& price, const QString& date){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString goalId = "goal_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QNetworkRequest goalRequest(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + ".json?auth=" + token));
    goalRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject goalData;
    goalData["title"] = title;
    goalData["target"] = price.toDouble();
    goalData["date"] = date;
    goalData["progress"] = 0;
    QNetworkReply* goalReply = m_manager->put(goalRequest, QJsonDocument(goalData).toJson());
    connect(goalReply, &QNetworkReply::finished, [=](){
        if (goalReply->error() == QNetworkReply::NoError){
            emit goalCreated();
        }
        goalReply->deleteLater();
    });
}

void GoalManager::deleteGoal(const QString& goalId){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + ".json?auth=" + token));
    QNetworkReply* reply = m_manager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            emit goalDeleted();
        }
        reply->deleteLater();
    });
}

void GoalManager::addDeposit(const QString& goalId, const QString& deposit){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString userId = AuthManager::instance().currentUser()->userId();
    QString token = AuthManager::instance().currentUser()->token();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + ".json?auth=" + token));
    QNetworkReply* reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject goalObj = doc.object();
            reply->deleteLater();
            QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + ".json?auth=" + token));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QJsonObject goalData;
            goalData["progress"] = deposit.toDouble() + goalObj["progress"].toDouble();
            QNetworkReply* reply = m_manager->sendCustomRequest(request, "PATCH" ,QJsonDocument(goalData).toJson());
            connect(reply, &QNetworkReply::finished, [=](){
                if (reply->error() == QNetworkReply::NoError){
                    QString depositId = "deposit_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
                    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/historyOfDeposit/" + depositId + ".json?auth=" + token));
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                    QJsonObject depositData;
                    depositData["title"] = goalObj["title"].toString();
                    depositData["progress"] = deposit.toDouble();
                    depositData["date"] = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
                    QNetworkReply* reply = m_manager->put(request, QJsonDocument(depositData).toJson());
                    connect(reply, &QNetworkReply::finished, [=](){
                        if (reply->error() == QNetworkReply::NoError){
                            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
                            QNetworkRequest historyRequest(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + "/history/" + timestamp + ".json?auth=" + token));
                            historyRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                            QJsonObject historyRecord;
                            historyRecord["amount"] = deposit;
                            historyRecord["date"] = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
                            QNetworkReply* historyReply = m_manager->put(historyRequest, QJsonDocument(historyRecord).toJson());
                            connect(historyReply, &QNetworkReply::finished, [=]() {
                                if (historyReply->error() == QNetworkReply::NoError)
                                {
                                    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + "/goals/" + goalId + "/history.json?auth=" + token));
                                    QNetworkReply *reply = m_manager->get(request);
                                    connect(reply, &QNetworkReply::finished, [=](){
                                        if (reply->error() == QNetworkReply::NoError){
                                            QJsonDocument historyDoc = QJsonDocument::fromJson(reply->readAll());
                                            QJsonObject historyObj = historyDoc.object();
                                            QVariantList historyList;
                                            for (auto it = historyObj.begin(); it != historyObj.end(); ++it) {
                                                QVariantMap record;
                                                record["timestamp"] = it.key();
                                                record["amount"] = it.value().toObject()["amount"].toString();
                                                record["date"] = it.value().toObject()["date"].toString();
                                                historyList.append(record);
                                            }
                                            std::sort(historyList.begin(), historyList.end(), [](const QVariant &a, const QVariant &b) {
                                                return a.toMap()["timestamp"].toString() > b.toMap()["timestamp"].toString();
                                            });

                                            emit depositAdded(deposit.toDouble() + goalObj["progress"].toDouble(), historyList);
                                        }
                                        reply->deleteLater();
                                    });
                                }
                                historyReply->deleteLater();
                            });
                        }
                        reply->deleteLater();
                    });
                }
                reply->deleteLater();
            });
        }
    });
}
