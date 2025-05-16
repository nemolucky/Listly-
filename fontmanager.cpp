#include "fontmanager.h"
#include <QCoreApplication>
#include <QSettings>

FontManager::FontManager(QObject *parent): QObject(parent){
    loadSettings();
};

qreal FontManager::fontMultiplier(){
    return m_multiplier;
}

void FontManager::setFontMultiplier(qreal multiplier){
    if (qFuzzyCompare(m_multiplier, multiplier))
        return;
    m_multiplier = multiplier;
    saveSettings();
    emit fontMultiplierChanged();
}

void FontManager::saveSettings(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.setValue("AppSettings/Multiplier", m_multiplier);
}

void FontManager::loadSettings(){
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    m_multiplier = settings.value("AppSettings/Multiplier").toReal();
}
