#ifndef GOALMANAGER_H
#define GOALMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QNetworkAccessManager>

class GoalManager : public QObject
{
    Q_OBJECT
public:
    explicit GoalManager(QObject *parent = nullptr);
    Q_INVOKABLE void loadUserGoals();
    Q_INVOKABLE void createGoal(const QString& title, const QString& price, const QString& date);
    Q_INVOKABLE void deleteGoal(const QString& goalId);
    Q_INVOKABLE void addDeposit(const QString& goalId, const QString& deposit);
private:
    QNetworkAccessManager *m_manager;
signals:
    void userGoalsLoaded(const QVariantList& data);
    void goalCreated();
    void goalDeleted();
    void depositAdded(const double& data, const QVariantList& history);
};

#endif // GOALMANAGER_H
