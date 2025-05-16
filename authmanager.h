#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include "user.h"
#include <QObject>
#include <QtQml/qqml.h>
#include <QSharedPointer>
#include <QNetworkAccessManager>

class AuthManager: public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(User* currentUser READ currentUser NOTIFY currentUserChanged)
public:
    ~AuthManager() = default;
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;
    static AuthManager* create(QQmlEngine*, QJSEngine*);
    static AuthManager& instance();
    static AuthManager& instanceRef();
    User* currentUser() {
        if (m_currentUser.isNull()) {
            qFatal("User object is null!");
        }
        return m_currentUser.data();
    }
    Q_INVOKABLE void registerUser(const QString &username,
                                  const QString &email, const QString &password);
    Q_INVOKABLE void loginUser(const QString &email,
                               const QString &password, bool rememberMe = false);
    Q_INVOKABLE void checkAuthState();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void changeUsername(const QString& username);
private:
    explicit AuthManager(QObject *parent = nullptr);
    QNetworkAccessManager *m_manager;
    QSharedPointer<User> m_currentUser;
    void saveUsernameToDatabase(const QString &userId, const QString &username, const QString &email, const QString &token);
    void verifyToken(const QString &token, std::function<void(bool)> callback);
    void refreshToken(const QString &refreshToken, const QString& userId, std::function<void (bool)> callback);
signals:
    void registrationSuccess();
    void registrationFailed(const QString& error);
    void loginSuccess();
    void loginFailed(const QString& error);
    void authError(const QString& error);
    void tokenValid();
    void currentUserChanged();
    void loggedOut();
    void usernameChanged();
    void usernameExist();
};

#endif // AUTHMANAGER_H
