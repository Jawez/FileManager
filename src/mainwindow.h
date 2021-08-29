#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>

#include "filesystemmodel.h"
#include "dockwidget.h"

#define CONFIG_FILE         "/config.ini"

#define LANGUAGE_PATTERN    ":/translations/FileManager_%1.qm"
#define LANGUAGE_CHINESE    "zh_CN"
#define LANGUAGE_ENGLISH    "en_US"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    DockWidget *dirDock;
    QDockWidget *fileDock;
    FileSystemModel *fileModel;

    QString appLanguage;

    QShortcut *cutShortcut;
    QShortcut *copyShortcut;
    QShortcut *pasteShortcut;
    QShortcut *deleteShortcut;
    QShortcut *refreshShortcut;
    QShortcut *expCollOneShortcut;
    QShortcut *expCollAllShortcut;

    void fileModelInit();

    void setupMenus();
    void setupWidgets();
    void setupShortCut();

    void connectShortcut(QWidget *widget);

    void loadTranslation();
    void languageChanged(const QString &lang);

public slots:
    void about();
    void aboutQt();

private slots:
    void languageZHCN();
    void languageENUS();
};
#endif // MAINWINDOW_H
