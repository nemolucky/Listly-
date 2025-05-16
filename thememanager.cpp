#include "thememanager.h"
#include <QCoreApplication>

ThemeManager::ThemeManager(QObject *parent) : QObject(parent),
    m_settings(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat)
{
    m_settings.beginGroup("AppSettings");
    m_currentTheme = m_settings.value("theme").toString();
    m_settings.endGroup();
}

QString ThemeManager::currentTheme() const {
    return m_currentTheme;
}

void ThemeManager::setCurrentTheme(const QString &theme) {
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        m_settings.beginGroup("AppSettings");
        m_settings.setValue("theme", theme);
        m_settings.endGroup();
        m_settings.sync();
        emit themeChanged();
    }
}

QString ThemeManager::background() const {
    return m_currentTheme == "dark" ? QString("qrc:images/dark-bg.jpg") : QString("qrc:images/light-bg.jpg");
}

QString ThemeManager::logo() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dLogo.png") : QString("qrc:icons/lLogo.png");
}

QString ThemeManager::settingsIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dSet.png") : QString("qrc:icons/lSet.png");
}

QString ThemeManager::userIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dUser.png") : QString("qrc:icons/lUser.png");
}

QString ThemeManager::editAccIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dEdit.png") : QString("qrc:icons/lEdit.png");
}

QString ThemeManager::warningIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dWarning.png") : QString("qrc:icons/lWarning.png");
}

QString ThemeManager::tickIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dTick.png") : QString("qrc:icons/lTick.png");
}

QString ThemeManager::themeIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dTheme.png") : QString("qrc:icons/lTheme.png");
}

QString ThemeManager::fileIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dFile.png") : QString("qrc:icons/lFile.png");
}

QString ThemeManager::plusIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dPlus.png") : QString("qrc:icons/lPlus.png");
}

QString ThemeManager::binIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dBin.png") : QString("qrc:icons/lBin.png");
}

QString ThemeManager::shareIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dShare.png") : QString("qrc:icons/lShare.png");
}

QString ThemeManager::backIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dBack.png") : QString("qrc:icons/lBack.png");
}

QString ThemeManager::starIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dStar.png") : QString("qrc:icons/lStar.png");
}

QString ThemeManager::minusIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dMinus.png") : QString("qrc:icons/lMinus.png");
}

QString ThemeManager::lupIcon() const {
    return m_currentTheme == "dark" ? QString("qrc:icons/dLup.png") : QString("qrc:icons/lLup.png");
}

QColor ThemeManager::primaryColor() const {
    return m_currentTheme == "dark" ? QColor("#A2A2A2") : QColor("#000000");
}

QColor ThemeManager::backgroundColor() const {
    return m_currentTheme == "dark" ? QColor("#2A2A2A") : QColor("#F4F4F4");
}

QColor ThemeManager::windowColor() const {
    return m_currentTheme == "dark" ? QColor("#191919") : QColor("#FFFFFF");
}

