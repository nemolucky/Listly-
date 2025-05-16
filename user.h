#ifndef USER_H
#define USER_H

#include <QObject>
#include <QNetworkAccessManager>

class User : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString userId READ userId NOTIFY userIdChanged)
    Q_PROPERTY(QString email READ email NOTIFY emailChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
public:
    explicit User(QObject* parent = nullptr);
    User& operator=(const User&) = delete;
    QString userId() const;
    QString email() const;
    QString username() const;
    QString token() const;
    void initializeIni(const QString &userId, const QString& token,
                       std::function<void(bool)> callback);
    void initializeUrl(const QString &userId, const QString& token,
                    std::function<void(bool)> callback);
private:
    QNetworkAccessManager *m_manager;
    QString m_userId;
    QString m_email;
    QString m_username;
    QString m_token;
signals:
    void userIdChanged();
    void emailChanged();
    void usernameChanged();
};

#endif // USER_H
