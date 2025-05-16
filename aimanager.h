#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <QJsonObject>
#include <QNetworkAccessManager>

class AIManager : public QObject {
    Q_OBJECT
public:
    explicit AIManager(QObject *parent = nullptr);
    Q_INVOKABLE void runAnalytics();
    Q_INVOKABLE void fetchPurchased();
    Q_INVOKABLE void fetchDeposits();
signals:
    void analyticsCompleted(const QString& data);
    void fetchedPurchased(const QJsonArray& purchases);
    void fetchedDeposits(const QJsonArray& deposits);
private:
    QNetworkAccessManager *m_manager;
    QString m_AccessKeyUrl;
    QString m_url;
    QString m_currentAccessToken;
    QString getAuthorizationKey();
    void getAccessKey(std::function<void(bool)> callback);
    void fetchPurchased(const QString& userId, const QString& token,
                                    std::function<void(bool, QJsonArray)> callback);
    void fetchDeposits(const QString& userId, const QString& token,
                                   std::function<void(bool, QJsonArray)> callback);
    QString buildRawDataPrompt(const QJsonArray& purchases,
                                          const QJsonArray& deposits);
    void sendToGigaChat(const QString& promt);
};

#endif // AIMANAGER_H
