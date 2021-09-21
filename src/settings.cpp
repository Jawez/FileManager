#include "settings.h"

#include <QtDebug>
#include <QApplication>

QVariant readSettings(const QString &group, const QString &key)
{
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);

    // default "General" section
//    return settings.value(key, QVariant(LANGUAGE_CHINESE));

    if (!group.isEmpty())
        settings.beginGroup(group);

    QVariant value = settings.value(key);

    if (!group.isEmpty())
        settings.endGroup();

    return value;
}

void writeSettings(const QString &group, const QString &key, const QVariant &value)
{
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);

    if (!group.isEmpty())
        settings.beginGroup(group);

    settings.setValue(key, value);

    if (!group.isEmpty())
        settings.endGroup();
}

QStringList readArraySettings(const QString &group)
{
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);

    QStringList list;
    int size = settings.beginReadArray(group);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        list.append(settings.value(CONFIG_ARR_ITEM).toString());
    }
    settings.endArray();

    return list;
}

void writeArraySettings(const QString &group, QStringList list)
{
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);

    settings.remove(group);

    settings.beginWriteArray(group);
    for (int i = 0; i < list.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(CONFIG_ARR_ITEM, list.at(i));
    }
    settings.endArray();
}

