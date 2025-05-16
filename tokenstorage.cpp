#include "tokenstorage.h"
#include <QCoreApplication>


QString getMachineKey() {
    QString machineId = QSysInfo::machineUniqueId();
    if (machineId.isEmpty()) {
        machineId = "default-secret-key";
    }
    return QCryptographicHash::hash(
               machineId.toUtf8(),
               QCryptographicHash::Sha256
               ).toHex();
}

void TokenStorage::saveTokens(const QString& idToken, const QString& refreshToken, const QString& userId){
    QString configPath = QCoreApplication::applicationDirPath() + "/authconfig.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString key = getMachineKey();
    settings.beginGroup("Auth");
    settings.setValue("id_token", encrypt(idToken, key));
    settings.setValue("refresh_token", encrypt(refreshToken, key));
    settings.setValue("user_id", userId);
    settings.endGroup();
}

bool TokenStorage::loadTokens(QString &outIdToken,
                              QString &outRefreshToken,
                              QString &outUserId)
{
    QString configPath = QCoreApplication::applicationDirPath() + "/authconfig.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    if (!settings.contains("Auth/id_token")) {
        return false;
    }
    QString key = getMachineKey();
    settings.beginGroup("Auth");
    outIdToken = decrypt(settings.value("id_token").toString(), key);
    outRefreshToken = decrypt(settings.value("refresh_token").toString(), key);
    outUserId = settings.value("user_id").toString();
    settings.endGroup();

    return !outIdToken.isEmpty() && !outRefreshToken.isEmpty();
}

void TokenStorage::clearTokens() {

    QString configPath = QCoreApplication::applicationDirPath() + "/authconfig.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    settings.beginGroup("Auth");
    settings.remove("");
    settings.endGroup();

    settings.sync();

#ifdef TOKEN_CACHE_ENABLED
    TokenCache::clear();
#endif
}

QString TokenStorage::encrypt(const QString& data, const QString &key){
    QByteArray dataBytes = data.toUtf8();
    QByteArray keyBytes = key.toUtf8();
    QByteArray result;

    for (int i = 0; i < dataBytes.size(); ++i) {
        result.append(dataBytes.at(i) ^ keyBytes.at(i % keyBytes.size()));
    }

    return result.toBase64();
}

QString TokenStorage::decrypt(const QString& encrypted, const QString &key) {
    QByteArray encryptedBytes = QByteArray::fromBase64(encrypted.toUtf8());
    QByteArray keyBytes = key.toUtf8();
    QByteArray result;

    for (int i = 0; i < encryptedBytes.size(); ++i) {
        result.append(encryptedBytes.at(i) ^ keyBytes.at(i % keyBytes.size()));
    }

    return QString::fromUtf8(result);
}
