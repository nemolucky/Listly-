#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QtQml/qqml.h>

class Database : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    explicit Database(QObject *parent = nullptr);
    Q_INVOKABLE bool openDatabase();
    Q_INVOKABLE QStringList searchProducts(const QString &query);
    Q_INVOKABLE void closeDatabase();

private:
    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // DATABASE_H
