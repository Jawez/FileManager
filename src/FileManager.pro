QT       += core gui
QT       += concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_NO_DEBUG_OUTPUT   # disable qDebug() output

SOURCES += \
    filedockwidget.cpp \
    filefilterproxymodel.cpp \
    filesystemmodel.cpp \
    filewidget.cpp \
    main.cpp \
    mainwindow.cpp \
    navdockwidget.cpp \
    progressdialog.cpp \
    settings.cpp \
    treeview.cpp

HEADERS += \
    filedockwidget.h \
    filefilterproxymodel.h \
    filesystemmodel.h \
    filewidget.h \
    mainwindow.h \
    navdockwidget.h \
    progressdialog.h \
    settings.h \
    treeview.h

TRANSLATIONS += \
    translations\FileManager_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    FileManager.qrc

RC_ICONS += \
    resources/icon_app.ico
