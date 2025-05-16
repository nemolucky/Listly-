#include "aimanager.h"
#include "authmanager.h"
#include <QUrl>
#include <QTimeZone>
#include <QSettings>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QNetworkAccessManager>

AIManager::AIManager(QObject *parent) : QObject(parent), m_manager(new QNetworkAccessManager(this)){
    m_AccessKeyUrl = "https://ngw.devices.sberbank.ru:9443/api/v2/oauth";
    m_url = "https://gigachat.devices.sberbank.ru/api/v1/chat/completions";
};

void AIManager::getAccessKey(std::function<void(bool)> callback){
    QString authorizationKey = getAuthorizationKey();
    QNetworkRequest request(m_AccessKeyUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("RqUID", QUuid::createUuid().toString().remove('{').remove('}').toUtf8());
    request.setRawHeader("Authorization", "Basic " + authorizationKey.toUtf8());
    QByteArray postData = "scope=GIGACHAT_API_PERS";
    QNetworkReply *reply = m_manager->post(request, postData);
    connect(reply, &QNetworkReply::finished, [this, reply, callback](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseData = reply->readAll();
            qDebug() << "Raw response:" << responseData;
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if(doc.isNull()) {
                qDebug() << "Invalid JSON response";
                callback(false);
                return;
            }
            QJsonObject obj = doc.object();
            if(obj.contains("access_token")) {
                m_currentAccessToken = obj["access_token"].toString();
                qDebug() << "Access token received, length:" << m_currentAccessToken.length();
                callback(true);
            } else {
                qDebug() << "No access_token in response";
                callback(false);
                return;
            }
        }
        else{
            QByteArray errorData = reply->readAll();
            qDebug() << "Error getting access key:" << reply->errorString();
            qDebug() << "Error details:" << errorData;
            callback(false);
            return;
        }
        reply->deleteLater();
    });
}

QString AIManager::getAuthorizationKey(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    return settings.value("AIManager/AuthorizationKey").toString();
}

void AIManager::runAnalytics(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString lastDate = settings.value("Analytics/date").toString();
    QDateTime lastDateTime = QDateTime::fromString(lastDate, Qt::ISODateWithMs);
    QDateTime now = QDateTime::currentDateTime();
    now.setTimeZone(QTimeZone::systemTimeZone());

    bool needRunAnalytics = false;

    if (!lastDateTime.isValid()) {
        needRunAnalytics = true;
    }
    else {
        qint64 secondsDiff = lastDateTime.secsTo(now);
        if (secondsDiff >= 12 * 3600) {
            needRunAnalytics = true;
        }
    }
    if (needRunAnalytics) {
        QString userId = AuthManager::instance().currentUser()->userId();
        QString token = AuthManager::instance().currentUser()->token();
        getAccessKey([this, userId, token](bool success){
            if (success){
                fetchPurchased(userId, token, [this, userId, token](bool success, QJsonArray purchases) {
                    if (success) {
                        qDebug() << purchases;
                    }
                    fetchDeposits(userId, token, [this, purchases](bool success, QJsonArray deposits) {
                        if (success) {
                            qDebug() << deposits;
                        }
                        QString prompt = buildRawDataPrompt(purchases, deposits);
                        sendToGigaChat(prompt);
                    });
                });
            }
        });
    }
    else {
        emit analyticsCompleted(settings.value("Analytics/response").toString());
    }
}


void AIManager::fetchPurchased(const QString& userId, const QString& token, std::function<void(bool, QJsonArray)> callback){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(databaseUrl + "/users/" + userId + "/historyOfPurchased.json?auth=" + token);
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, callback](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            QJsonObject rootObj = doc.object();
            QJsonArray resultArray;
            for(auto it = rootObj.begin(); it != rootObj.end(); ++it){
                QString id = it.key();
                QJsonObject depositObj = it.value().toObject();
                depositObj.insert("id", id);
                resultArray.append(depositObj);
            }
            emit fetchedPurchased(resultArray);
            callback(true, resultArray);
        }
        reply->deleteLater();
    });
}

void AIManager::fetchDeposits(const QString& userId, const QString& token, std::function<void(bool, QJsonArray)> callback){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(databaseUrl + "/users/" + userId + "/historyOfDeposit.json?auth=" + token);
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, callback](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            QJsonObject rootObj = doc.object();
            QJsonArray resultArray;
            for(auto it = rootObj.begin(); it != rootObj.end(); ++it){
                QString id = it.key();
                QJsonObject depositObj = it.value().toObject();
                depositObj.insert("id", id);
                resultArray.append(depositObj);
            }
            emit fetchedDeposits(resultArray);
            callback(true, resultArray);
        }
        reply->deleteLater();
    });
}

void AIManager::fetchPurchased(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(databaseUrl + "/users/" + AuthManager::instance().currentUser()->userId() + "/historyOfPurchased.json?auth=" + AuthManager::instance().currentUser()->token());
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            QJsonObject rootObj = doc.object();
            QList<QPair<QString, QJsonObject>> purchasesList;
            for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
                QString id = it.key();
                QJsonObject purchaseObj = it.value().toObject();
                purchaseObj.insert("id", id);
                purchasesList.append(qMakePair(purchaseObj["purchaseDate"].toString(), purchaseObj));
            }
            std::sort(purchasesList.begin(), purchasesList.end(),
                      [](const QPair<QString, QJsonObject> &a, const QPair<QString, QJsonObject> &b) {
                          return a.first > b.first;
                      });
            QJsonArray resultArray;
            for (const auto &purchase : purchasesList) {
                resultArray.append(purchase.second);
            }
            emit fetchedPurchased(resultArray);
        }
        reply->deleteLater();
    });
}

void AIManager::fetchDeposits(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(databaseUrl + "/users/" + AuthManager::instance().currentUser()->userId() + "/historyOfDeposit.json?auth=" + AuthManager::instance().currentUser()->token());
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            QJsonObject rootObj = doc.object();

            QList<QPair<QString, QJsonObject>> depositsList;

            for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
                QString id = it.key();
                QJsonObject depositObj = it.value().toObject();
                depositObj.insert("id", id);
                depositsList.append(qMakePair(depositObj["date"].toString(), depositObj));
            }

            // Сортируем по дате (от новых к старым)
            std::sort(depositsList.begin(), depositsList.end(),
                      [](const QPair<QString, QJsonObject> &a, const QPair<QString, QJsonObject> &b) {
                          return a.first > b.first;
                      });
            QJsonArray resultArray;
            for (const auto &deposit : depositsList) {
                resultArray.append(deposit.second);
            }

            emit fetchedDeposits(resultArray);
        }
        reply->deleteLater();
    });
}

QString AIManager::buildRawDataPrompt(const QJsonArray& purchases, const QJsonArray& deposits) {
    QJsonDocument purchasesDoc(purchases);
    QJsonDocument depositsDoc(deposits);

    QString purchasesJson = QString::fromUtf8(purchasesDoc.toJson(QJsonDocument::Compact));
    QString depositsJson = QString::fromUtf8(depositsDoc.toJson(QJsonDocument::Compact));

    QString prompt = "Вот мои данные по истории покупок:\n" + purchasesJson + "\n\n";
    prompt += "Вот мои данные по истории пополнений:\n" + depositsJson + "\n\n";
    prompt += "Проанализируй эти данные и дай рекомендации по улучшению моего финансового состояния.";

    return prompt;
}

void AIManager::sendToGigaChat(const QString& prompt) {
    QUrl url("https://gigachat.devices.sberbank.ru/api/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_currentAccessToken).toUtf8());
    QJsonObject json;
    json["model"] = "GigaChat";
    QJsonArray messages;
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "Отвечай простым, понятным текстом на русском языке, без форматирования, без markdown, без списков, без рамок и без оформления. Просто дай развернутый ответ.";
    messages.append(systemMessage);
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    json["messages"] = messages;
    QNetworkReply* reply = m_manager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject message = choices[0].toObject()["message"].toObject();
                QString content = message["content"].toString();
                QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
                QSettings settings(configPath, QSettings::IniFormat);
                settings.beginGroup("Analytics");
                settings.remove("");
                settings.setValue("prompt", prompt);
                settings.setValue("response", content);
                settings.setValue("date", QDateTime::currentDateTime().toString(Qt::ISODate));
                settings.endGroup();
                settings.sync();
                emit analyticsCompleted(content);
            }
        } else {
            qDebug() << "Ошибка запроса к GigaChat:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

