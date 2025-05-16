#ifndef TOKENSTORAGE_H
#define TOKENSTORAGE_H

#include <QSettings>
#include <QCryptographicHash>

class TokenStorage
{
public:
    static void saveTokens(const QString& idToken,
                           const QString& refreshToken,
                           const QString& userId);

    static bool loadTokens(QString& outIdToken,
                           QString& outRefreshToken,
                           QString& outUserId);

    static void clearTokens();

private:
    static QString encrypt(const QString& data, const QString &key);
    static QString decrypt(const QString& encrypted, const QString &key);
};

#endif // TOKENSTORAGE_H
