#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include "QObject"

class FontManager:public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal fontMultiplier READ fontMultiplier WRITE setFontMultiplier NOTIFY fontMultiplierChanged)
public:
    explicit FontManager(QObject *parent = nullptr);
    qreal fontMultiplier();
    void setFontMultiplier(qreal multiplier);

private:
    qreal m_multiplier;
    void loadSettings();
    void saveSettings();
signals:
    void fontMultiplierChanged();
};

#endif // FONTMANAGER_H
