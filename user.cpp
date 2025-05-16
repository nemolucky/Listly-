#include "user.h"
#include <QSettings>
#include <QJsonObject>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QCoreApplication>

User::User(QObject* parent) : QObject(parent), m_manager(new QNetworkAccessManager(this)) {}

QString User::userId() const {return m_userId;}

QString User::email() const {return m_email;}

QString User::username() const {return m_username;}

QString User::token() const {return m_token;}

void User::initializeIni(const QString &userId, const QString& token, std::function<void(bool)> callback){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("UserMappings");
    settings.beginGroup(userId);
    QString username = settings.value("username").toString();
    QString email = settings.value("email").toString();
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    settings.remove("UserMappings");
    settings.sync();
    m_userId = userId;
    m_email = email;
    m_username = username;
    m_token = token;
    callback(true);
}

void User::initializeUrl(const QString &userId, const QString& token,
                      std::function<void(bool)> callback) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString().trimmed();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + ".json?auth=" + token));
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [=]() {
        QByteArray response = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        QJsonObject rootObj = doc.object();
        m_userId = userId;
        m_email = rootObj["email"].toString();
        m_username = rootObj["username"].toString();
        m_token = token;
        callback(true);
        reply->deleteLater();
    });
}
