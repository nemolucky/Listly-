#ifndef GIGACHATMANAGER_H
#define GIGACHATMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QSettings>

class AuthManager;

class GigaChatManager : public QObject
{
    Q_OBJECT
public:
    explicit GigaChatManager(QObject *parent = nullptr);

signals:


private:
    void getAccessKey();
    QString getAuthorizationKey() const;
    QNetworkAccessManager *m_manager;
    QString m_databaseUrl;
    QString m_currentAccessToken;
    QString m_AccessKeyUrl = "https://ngw.devices.sberbank.ru:9443/api/v2/oauth";
};
#endif
