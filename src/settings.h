#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QVariant>


#define CONFIG_FILE             "/config.ini"

#define LANGUAGE_PATTERN        ":/translations/FileManager_%1.qm"
#define LANGUAGE_NAME_CHINESE   "简体中文"
#define LANGUAGE_CHINESE        "zh_CN"
#define LANGUAGE_NAME_ENGLISH   "English"
#define LANGUAGE_ENGLISH        "en_US"


#define CONFIG_GROUP_DEFAULT    ""
#define CONFIG_DEF_LANGUAGE     "language"
#define CONFIG_DEF_SYSTRAY      "Tray"

#define CONFIG_GROUP_WINDOW     "Window"
#define CONFIG_WIN_SIZE         "size"
#define CONFIG_WIN_POS          "pos"
#define CONFIG_WIN_GEOMETRY     "Geometry"
#define CONFIG_WIN_STATE        "WindowState"

#define CONFIG_GROUP_TOOLBAR    "ToolBar"

#define CONFIG_GROUP_NAVDOCK    "NavDock"
#define CONFIG_GROUP_FILEDOCK   "FileDock"
#define CONFIG_DOCK_HIDE        "hide"

#define CONFIG_GROUP_MAINWIN    "MainWidget"

#define CONFIG_TAB_COUNT        "count"
#define CONFIG_TAB_INDEX        "index"

#define CONFIG_GROUP_FIND       "FindWidget"

#define CONFIG_ARR_ITEM         "item"


#define ITEM_DEFAULT_HEIGHT     (30)


QVariant readSettings(const QString &group, const QString &key);
void writeSettings(const QString &group, const QString &key, const QVariant &value);

QStringList readArraySettings(const QString &group);
void writeArraySettings(const QString &group, QStringList list);

#endif // SETTINGS_H
