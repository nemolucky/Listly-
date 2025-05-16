#include "Database.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>

Database::Database(QObject *parent) : QObject(parent)
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    m_dbPath = appDataPath + "/database.db";
    openDatabase();
}

bool Database::openDatabase()
{
    if (!QFile::exists(m_dbPath)) {
        if (!QFile::copy(":/data/database.db", m_dbPath)) {
            qWarning() << "Failed to copy database from resources!";
            return false;
        }
    }
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }
    return true;
}

QStringList Database::searchProducts(const QString &query)
{
    QStringList result;
    if(query.trimmed().isEmpty()) {
        return result;
    }
    QString searchTerm = query.trimmed().toLower();
    QSqlQuery q;
    q.prepare("SELECT name FROM products WHERE "
              "name_lower LIKE ? "
              "ORDER BY "
              "CASE "
              "WHEN name_lower LIKE ? THEN 0 "
              "WHEN name_lower LIKE ? THEN 1 "
              "ELSE 2 END, "
              "name "
              "LIMIT 50");
    QString pattern = "%" + searchTerm + "%";
    QString startPattern = searchTerm + "%";
    QString wordStartPattern = "% " + searchTerm + "%";
    q.addBindValue(pattern);
    q.addBindValue(startPattern);
    q.addBindValue(wordStartPattern);
    if(q.exec()) {
        while(q.next()) {
            result.append(q.value(0).toString());
        }
    } else {
        qDebug() << "Query error:" << q.lastError().text();
    }
    return result;
}

void Database::closeDatabase()
{
    m_db.close();
}
