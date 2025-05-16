#include "authmanager.h"
#include <tokenstorage.h>
#include <QUrl>
#include <QSettings>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QNetworkAccessManager>

AuthManager::AuthManager(QObject *parent) : QObject(parent),
    m_manager(new QNetworkAccessManager(this)), m_currentUser(new User()){};

AuthManager& AuthManager::instance() {
    static AuthManager instance;
    return instance;
}

AuthManager* AuthManager::create(QQmlEngine* engine, QJSEngine* scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return &instance();
}

void AuthManager::registerUser(const QString &username, const QString &email, const QString &password){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString apiKey = settings.value("Firebase/ApiKey").toString().trimmed();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString().trimmed();
    QNetworkRequest request(QUrl("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["email"] = email;
    body["password"] = password;
    body["returnSecureToken"] = true;
    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            QString userId = response["localId"].toString();
            QString token = response["idToken"].toString();
            QNetworkRequest checkRequest(QUrl(databaseUrl + "/users.json?orderBy=\"username\"&equalTo=\"" + username + "\"&auth=" + token));
            QNetworkReply *checkReply = m_manager->get(checkRequest);
            connect(checkReply, &QNetworkReply::finished, [=]() {
                if (checkReply->error() == QNetworkReply::NoError) {
                    QJsonDocument checkResponse = QJsonDocument::fromJson(checkReply->readAll());
                    QJsonObject users = checkResponse.object();
                    if (users.size() > 0) {
                        QNetworkRequest deleteRequest(QUrl("https://identitytoolkit.googleapis.com/v1/accounts:delete?key=" + apiKey));
                        deleteRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                        QJsonObject deleteBody;
                        deleteBody["idToken"] = token;
                        m_manager->post(deleteRequest, QJsonDocument(deleteBody).toJson());
                        emit registrationFailed("Username already exists");
                    }else{
                        saveUsernameToDatabase(userId, username, email, token);
                    }
                }
            });
        }else{
            emit registrationFailed("Email already exists");
        }
        reply->deleteLater();
    });
}

void AuthManager::loginUser(const QString &email, const QString &password, bool rememberMe){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString apiKey = settings.value("Firebase/ApiKey").toString();
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QNetworkRequest request(QUrl("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["email"] = email;
    body["password"] = password;
    body["returnSecureToken"] = true;
    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            QString userId = response["localId"].toString();
            QString token = response["idToken"].toString();
            if (rememberMe) TokenStorage::saveTokens(token, response["refreshToken"].toString(), userId);
            m_currentUser->initializeUrl(userId, token, [=](bool success) {
                if (success) {
                    emit loginSuccess();
                } else {
                    emit authError("Failed to load user data");
                }
            });
        } else {
            emit loginFailed(reply->errorString());
        }
        reply->deleteLater();
    });
}

void AuthManager::saveUsernameToDatabase(const QString &userId, const QString &username, const QString &email, const QString &token) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString().trimmed();
    QNetworkRequest request(QUrl(databaseUrl + "/users/" + userId + ".json?auth=" + token));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject userData;
    userData["username"] = username;
    userData["email"] = email;
    QNetworkReply *reply = m_manager->put(request, QJsonDocument(userData).toJson());
    connect(reply, &QNetworkReply::finished, [=](){
        if (reply->error() == QNetworkReply::NoError){
            emit registrationSuccess();
        }
        reply->deleteLater();
    });
}

void AuthManager::checkAuthState() {
    QString idToken, Token, userId;
    if (!TokenStorage::loadTokens(idToken, Token, userId)) {
        emit authError("No saved tokens found");
        return;
    }
    verifyToken(idToken, [=](bool isValid) {
        if (isValid) {
            m_currentUser->initializeUrl(userId, idToken, [=](bool success) {
                if (success) {
                    emit tokenValid();
                } else {
                    emit authError("Failed to load user data");
                }
            });
        } else {
            refreshToken(Token, userId, [=](bool success) {
                if (success) {
                    QString idToken, Token, userId;
                    TokenStorage::loadTokens(idToken, Token, userId);
                    m_currentUser->initializeUrl(userId, idToken, [=](bool success) {
                        if (success) {
                            emit tokenValid();
                        } else {
                            emit authError("Failed to load user data");
                        }
                    });
                } else {
                    emit authError("Token refresh failed");
                }
            });
        }
    });
}

void AuthManager::verifyToken(const QString& token, std::function<void(bool)> callback) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString apiKey = settings.value("Firebase/ApiKey").toString();
    QNetworkRequest request(QUrl("https://identitytoolkit.googleapis.com/v1/accounts:lookup?key=" + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["idToken"] = token;
    QNetworkReply* reply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, [=]() {
        bool valid = false;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            valid = !response["users"].toArray().isEmpty();
        }
        reply->deleteLater();
        callback(valid);
    });
}

void AuthManager::refreshToken(const QString &refreshToken, const QString& userId,
                               std::function<void (bool)> callback) {
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString apiKey = settings.value("Firebase/ApiKey").toString();

    QNetworkRequest request(QUrl("https://securetoken.googleapis.com/v1/token?key=" + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QString body = QString("grant_type=refresh_token&refresh_token=%1").arg(refreshToken);
    QNetworkReply* reply = m_manager->post(request, body.toUtf8());

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            QString newIdToken = response["id_token"].toString();
            QString newRefreshToken = response["refresh_token"].toString();
            if (!newIdToken.isEmpty() && !newRefreshToken.isEmpty()) {
                TokenStorage::saveTokens(newIdToken, newRefreshToken, userId);
                callback(true);
            }
        }
        reply->deleteLater();
    });
}

void AuthManager::logout(){
    TokenStorage::clearTokens();
    emit loggedOut();
}

void AuthManager::changeUsername(const QString& username){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString databaseUrl = settings.value("Firebase/DatabaseUrl").toString();
    QString token = m_currentUser->token();
    QNetworkRequest checkRequest(QUrl(databaseUrl + "/users.json?orderBy=\"username\"&equalTo=\"" + username + "\"&auth=" + token));
    QNetworkReply *checkReply = m_manager->get(checkRequest);
    connect(checkReply, &QNetworkReply::finished, [=]() {
        if (checkReply->error() == QNetworkReply::NoError) {
            QJsonDocument checkResponse = QJsonDocument::fromJson(checkReply->readAll());
            QJsonObject users = checkResponse.object();
            if (users.size() > 0) {
                emit usernameExist();
            }else{
                QNetworkRequest request(QUrl(databaseUrl + "/users/" + m_currentUser->userId() + ".json?auth=" + token));
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QJsonObject userData;
                userData["username"] = username;
                QNetworkReply *reply = m_manager->sendCustomRequest(request, "PATCH", QJsonDocument(userData).toJson());
                connect(reply, &QNetworkReply::finished, [=](){
                    if (reply->error() == QNetworkReply::NoError){
                        m_currentUser->initializeUrl(m_currentUser->userId(), token, [=](bool success) {
                            if (success) {
                                emit usernameChanged();
                            }
                        });
                    }
                });
            }
        }
    });
}
