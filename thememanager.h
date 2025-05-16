#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QSettings>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setCurrentTheme NOTIFY themeChanged)
    Q_PROPERTY(QColor primaryColor READ primaryColor NOTIFY themeChanged)
    Q_PROPERTY(QString background READ background NOTIFY themeChanged)
    Q_PROPERTY(QString logo READ logo NOTIFY themeChanged)
    Q_PROPERTY(QString settingsIcon READ settingsIcon NOTIFY themeChanged)
    Q_PROPERTY(QString userIcon READ userIcon NOTIFY themeChanged)
    Q_PROPERTY(QString editAccIcon READ editAccIcon NOTIFY themeChanged)
    Q_PROPERTY(QString warningIcon READ warningIcon NOTIFY themeChanged)
    Q_PROPERTY(QString tickIcon READ tickIcon NOTIFY themeChanged)
    Q_PROPERTY(QString themeIcon READ themeIcon NOTIFY themeChanged)
    Q_PROPERTY(QString themeIcon READ themeIcon NOTIFY themeChanged)
    Q_PROPERTY(QString fileIcon READ fileIcon NOTIFY themeChanged)
    Q_PROPERTY(QString plusIcon READ plusIcon NOTIFY themeChanged)
    Q_PROPERTY(QString binIcon READ binIcon NOTIFY themeChanged)
    Q_PROPERTY(QString shareIcon READ shareIcon NOTIFY themeChanged)
    Q_PROPERTY(QString backIcon READ backIcon NOTIFY themeChanged)
    Q_PROPERTY(QString starIcon READ starIcon NOTIFY themeChanged)
    Q_PROPERTY(QString minusIcon READ minusIcon NOTIFY themeChanged)
    Q_PROPERTY(QString lupIcon READ lupIcon NOTIFY themeChanged)
    Q_PROPERTY(QColor windowColor READ windowColor NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY themeChanged)
public:
    explicit ThemeManager(QObject *parent = nullptr);
    QString background() const;
    QString logo() const;
    QString settingsIcon() const;
    QString userIcon() const;
    QString editAccIcon() const;
    QString warningIcon() const;
    QString tickIcon() const;
    QString themeIcon() const;
    QString fileIcon() const;
    QString plusIcon() const;
    QString binIcon() const;
    QString shareIcon() const;
    QString backIcon() const;
    QString starIcon() const;
    QString minusIcon() const;
    QString lupIcon() const;
    QColor primaryColor() const;
    QColor windowColor() const;
    QColor backgroundColor() const;
    QString currentTheme() const;
    Q_INVOKABLE void setCurrentTheme(const QString &theme);

signals:
    void themeChanged();

private:
    QSettings m_settings;
    QString m_currentTheme;
};

#endif // THEMEMANAGER_H
