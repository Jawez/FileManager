#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QCloseEvent>
#include <QSystemTrayIcon>

#include "filesystemmodel.h"
#include "navdockwidget.h"
#include "filedockwidget.h"
#include "findwidget.h"


#define MAX_TOOLBAR_COUNT           10

#define OBJECTNAME_NAV_DOCK         "Navigation Bar"
#define OBJECTNAME_FILE_DOCK        "File Dock"
#define OBJECTNAME_FIND_DOCK        "Find Dock"
#define OBJECTNAME_TOOLBAR          "Quick Button"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setVisible(bool visible) override;

private:
    QToolBar* toolBar;
    QStringList toolBarList;

    NavDockWidget *navDock;
    FileDockWidget *fileDock;
    FileSystemModel *fileModel;
    QDockWidget *findDock;

    QString appLanguage;

    QShortcut *cutShortcut;
    QShortcut *copyShortcut;
    QShortcut *pasteShortcut;
    QShortcut *deleteShortcut;
    QShortcut *refreshShortcut;
    QShortcut *expCollOneShortcut;
    QShortcut *expCollAllShortcut;

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;

    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;

    void fileModelInit();

    void setupToolBar();
    void setupMenuBar();
    void setupWidgets();
    void setupShortCut();

    void createActions();
    void createTrayIcon();

    void toolBarAddAction(bool addDir = true);

    void connectShortcut(QWidget *widget);

    void loadTranslation();
    void languageChanged(const QString &lang);

    void loadWindowInfo();
    void saveWindowInfo();

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void about();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void languageZHCN();
    void languageENUS();
    void onToolBarActionTriggered(QAction *action);
    void toolBarOnTextMenu(const QPoint &pos);
    void onFindFiles(const QString &path, const QString &find);
};
#endif // MAINWINDOW_H
