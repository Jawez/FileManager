#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QCloseEvent>

#include "filesystemmodel.h"
#include "navdockwidget.h"
#include "filedockwidget.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    NavDockWidget *navDock;
    FileDockWidget *fileDock;
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

    void loadWindowInfo();
    void saveWindowInfo();

public slots:
    void about();
    void aboutQt();

private slots:
    void languageZHCN();
    void languageENUS();
};
#endif // MAINWINDOW_H
