#include "mainwindow.h"
#include "filewidget.h"
#include "settings.h"

#include <QtDebug>
#include <QApplication>
#include <QTranslator>
#include <QProcess>
#include <QAction>
#include <QScreen>
#include <QFileIconProvider>
#include <QFileDialog>
#include <QDockWidget>
#include <QMessageBox>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QWidget>
#include <QMenu>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    appLanguage = LANGUAGE_CHINESE;
    loadTranslation();

    setWindowTitle(tr("FileManagerDemo"));

    fileModel = new FileSystemModel();
    fileDock = new FileDockWidget(fileModel);
    navDock = new NavDockWidget(fileModel);
    findDock = new QDockWidget();

    cutShortcut = new QShortcut(this);
    copyShortcut = new QShortcut(this);
    pasteShortcut = new QShortcut(this);
    deleteShortcut = new QShortcut(this);
    refreshShortcut = new QShortcut(this);
    expCollOneShortcut = new QShortcut(this);
    expCollAllShortcut = new QShortcut(this);

    fileModelInit();
    setupWidgets();
    setupToolBar();
    setupMenuBar();
    setupShortCut();

    // system tray
    trayIcon = nullptr;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QVariant useTray = readSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_SYSTRAY);
        qDebug() << QString("useTray valid %1").arg(useTray.isValid());
        if (!useTray.isValid()) {
            useTray = QVariant(true);
            writeSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_SYSTRAY, useTray);
        }

        qDebug() << QString("useTray bool %1").arg(useTray.toBool());
        if (useTray.toBool()) {
            QApplication::setQuitOnLastWindowClosed(false);

            createActions();
            createTrayIcon();
            QIcon icon(QLatin1String(":/resources/icon_app_64.png"));
            trayIcon->setIcon(icon);
            trayIcon->setToolTip("FileManager");
            connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
            trayIcon->show();
        }
    }

//    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::saveWindowInfo);

    loadWindowInfo();
}

MainWindow::~MainWindow()
{
    cutShortcut->deleteLater();
    copyShortcut->deleteLater();
    pasteShortcut->deleteLater();
    deleteShortcut->deleteLater();
    refreshShortcut->deleteLater();
    expCollOneShortcut->deleteLater();
    expCollAllShortcut->deleteLater();

    navDock->deleteLater();
    fileDock->deleteLater();
    fileModel->deleteLater();
}


void MainWindow::fileModelInit()
{
#if DISABLE_FILE_WATCHER
    fileModel->setOptions(QFileSystemModel::DontWatchForChanges);
#endif

    fileModel->setRootPath("");
    /*
     * default QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs
     * add QDir::System will show all shortcut(.lnk file) and system files
     * add QDir::Hidden will show some files that not visible on Windows, these files may be modified by mistake.
     */
    fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::System/* | QDir::Hidden*/);
    fileModel->setReadOnly(false);
}

void MainWindow::setupWidgets()
{
    // central widget
    FileWidget *widget = new FileWidget(CONFIG_GROUP_MAINWIN, fileModel, this);
    connectShortcut(widget);
//    QSplitter *splitter = new QSplitter;
//    splitter->addWidget(widget);
    setCentralWidget(widget);
    connect(widget, &FileWidget::findFiles, this, &MainWindow::onFindFiles);

    // dock
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    // navigation dock
    navDock->setObjectName(OBJECTNAME_NAV_DOCK);
    navDock->setWindowTitle(tr("Navigation Bar"));  // show in the dock
    addDockWidget(Qt::LeftDockWidgetArea, navDock);
    connect(navDock, &NavDockWidget::navDockClicked, widget, &FileWidget::onNavigateBarClicked);
    connect(refreshShortcut, &QShortcut::activated, navDock, &NavDockWidget::refreshTreeView);

    // file dock
    fileDock->setObjectName(OBJECTNAME_FILE_DOCK);
    fileDock->setWindowTitle(tr("File Dock"));
    addDockWidget(Qt::RightDockWidgetArea, fileDock);
    connectShortcut((FileWidget *)fileDock->widget());
    connect((FileWidget *)fileDock->widget(), &FileWidget::findFiles, this, &MainWindow::onFindFiles);

    // find dock
    FindWidget *findWidget = new FindWidget(nullptr, QDir::currentPath(), "*");
    connect(findWidget, &FindWidget::cellActivated, widget, &FileWidget::onItemActivated);
    findDock->setObjectName(OBJECTNAME_FIND_DOCK);
    findDock->setWindowTitle(tr("Find Dock"));
    findDock->setWidget(findWidget);
//    addDockWidget(Qt::AllDockWidgetAreas, findDock);
    addDockWidget(Qt::RightDockWidgetArea, findDock);
//    addDockWidget(Qt::BottomDockWidgetArea, findDock);
}

void MainWindow::setupToolBar()
{
    toolBar = addToolBar(tr("Quick Button"));
    toolBar->setObjectName(OBJECTNAME_TOOLBAR);
//    toolBar->setIconSize(QSize(20, 30));
//    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setIconSize(QSize(15, 20));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    qDebug() << QString("toolBar default menu %1").arg(toolBar->contextMenuPolicy());   // "toolBar default menu 1" Qt::DefaultContextMenu
    toolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolBar, &QToolBar::customContextMenuRequested, this, &MainWindow::toolBarOnTextMenu);


//    foreach (QFileInfo info, QDir::drives()) {
//        QString path = info.absolutePath();     // example "C:/", file's path absolute path. This doesn't include the file name
//        path = QDir::toNativeSeparators(path);  // example "C:\\"
//        toolBar->addAction(fileModel->iconProvider()->icon(info), path);
//    }
//    toolBar->addAction("+");

    connect(toolBar, &QToolBar::actionTriggered, this, &MainWindow::onToolBarActionTriggered);
}

void MainWindow::setupMenuBar()
{
    // file menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *exitAction = fileMenu->addAction(tr("E&xit"), qApp, &QCoreApplication::quit, Qt::QueuedConnection);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setShortcut(Qt::CTRL | Qt::Key_Q);

    // view menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    navDock->toggleViewAction()->setText(tr("Navi&gation Bar"));
    navDock->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_G);
    viewMenu->addAction(navDock->toggleViewAction());

    fileDock->toggleViewAction()->setText(tr("F&ile Dock"));
    fileDock->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_I);
    viewMenu->addAction(fileDock->toggleViewAction());

    findDock->toggleViewAction()->setText(tr("&Find Dock"));
    findDock->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_F);
    viewMenu->addAction(findDock->toggleViewAction());

    viewMenu->addSeparator();

    toolBar->toggleViewAction()->setText(tr("Quick &Button"));
    toolBar->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_B);
    viewMenu->addAction(toolBar->toggleViewAction());

    viewMenu->addSeparator();

//    FileWidget *widget = (FileWidget *)((QSplitter *)centralWidget())->widget(0);
    FileWidget *widget = (FileWidget *)centralWidget();
    QAction *addTabAct = viewMenu->addAction(tr("Add &Tab"), widget, &FileWidget::fileWidgetAddTab);
    addTabAct->setShortcut(Qt::CTRL | Qt::Key_T);

    // options menu
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Options"));

    QMenu *languageMenu = optionsMenu->addMenu(tr("&Language"));
    QAction *zhCNAct = new QAction(LANGUAGE_NAME_CHINESE, this);
    zhCNAct->setCheckable(true);
    connect(zhCNAct, &QAction::triggered, this, &MainWindow::languageZHCN);
    QAction *enUSAct = new QAction(LANGUAGE_NAME_ENGLISH, this);
    enUSAct->setCheckable(true);
    connect(enUSAct, &QAction::triggered, this, &MainWindow::languageENUS);

    QActionGroup *areaActions = new QActionGroup(this);
    areaActions->setExclusive(true);
    areaActions->addAction(zhCNAct);
    areaActions->addAction(enUSAct);
    languageMenu->addActions(areaActions->actions());

    if (appLanguage.compare(LANGUAGE_CHINESE) == 0) {
        zhCNAct->setChecked(true);
    } else {
        enUSAct->setChecked(true);
    }

    // help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
//    aboutAct->setToolTip(tr("Show the application's About box"));
    QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
//    aboutQtAct->setToolTip(tr("Show the Qt library's About box"));
}

void MainWindow::setupShortCut()
{
    cutShortcut->setKey(QKeySequence::Cut);
    cutShortcut->setAutoRepeat(false);

    copyShortcut->setKey(QKeySequence::Copy);
    copyShortcut->setAutoRepeat(false);

    pasteShortcut->setKey(QKeySequence::Paste);
    pasteShortcut->setAutoRepeat(false);

    deleteShortcut->setKey(QKeySequence::Delete);
    deleteShortcut->setAutoRepeat(false);

    refreshShortcut->setKey(QKeySequence::Refresh);
    refreshShortcut->setAutoRepeat(false);

    expCollOneShortcut->setKey(Qt::CTRL | Qt::Key_E);
    expCollOneShortcut->setAutoRepeat(false);

    expCollAllShortcut->setKey(Qt::CTRL | Qt::Key_R);
    expCollAllShortcut->setAutoRepeat(false);
}

void MainWindow::connectShortcut(QWidget *widget)
{
//    qDebug() << QString("connect shortcut ") << (FileWidget *)widget;
    connect(cutShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::cutSelectedItem);
    connect(copyShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::copySelectedItem);
    connect(pasteShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::pasteSelectedItem);
    connect(deleteShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::deleteSelectedItem);
    connect(refreshShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::refreshTreeView);
    connect(expCollOneShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::expandCollapseOne);
    connect(expCollAllShortcut, &QShortcut::activated, (FileWidget *)widget, &FileWidget::expandCollapseAll);
}

void MainWindow::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
//    minimizeAction->setShortcut(Qt::CTRL | Qt::Key_H);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::setVisible(bool visible)
{
    if (trayIcon) {
        minimizeAction->setEnabled(visible);
        maximizeAction->setEnabled(!isMaximized());
        restoreAction->setEnabled(isMaximized() || !visible);
    }
    QMainWindow::setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowInfo();
    if (trayIcon && trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    qDebug() << QString("iconActivated reason %1").arg(reason);
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        break;
    case QSystemTrayIcon::DoubleClick:
        if (!this->isVisible()) {
            this->show();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        ;
    }
}

void MainWindow::loadTranslation()
{
    QString language = readSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_LANGUAGE).toString();
    qDebug() << QString("load language %1").arg(language);
    if (language.compare(LANGUAGE_CHINESE) == 0) {
        // do nothing
    } else if (language.compare(LANGUAGE_ENGLISH) == 0) {
        appLanguage = language;
        return;     // use default text, do not load translator
    } else {
//        QString locale = QLocale::system().name();    // locale zh_CN
        language = QString(LANGUAGE_CHINESE);
        writeSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_LANGUAGE, QVariant(language));
    }

    appLanguage = language;

    QTranslator *translator = new QTranslator;
    QString languageFile = QString(LANGUAGE_PATTERN).arg(language);
    bool loadResult = translator->load(languageFile);
    qDebug() << QString("load result %1, file %2").arg(loadResult).arg(languageFile);
    if (loadResult)
        qApp->installTranslator(translator);
}

void MainWindow::languageChanged(const QString &lang)
{
    if (lang == appLanguage)
        return;

    // update config file
    writeSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_LANGUAGE, QVariant(lang));

    // prompt the user to restart
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Restart Required"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("<p>The language change will take effect after restart.</p>"));
    QPushButton *restartButton = msgBox.addButton(tr("Restart Now"), QMessageBox::ActionRole);
    QPushButton *laterButton = msgBox.addButton(tr("Later"), QMessageBox::ActionRole);
    msgBox.setDefaultButton(restartButton);
    msgBox.setEscapeButton(laterButton);

    int ret = msgBox.exec();
    if (msgBox.clickedButton() == restartButton) {
        // restart application
        qApp->quit();
        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
    } else if (msgBox.clickedButton() == laterButton) {
    }
}

void MainWindow::languageZHCN()
{
    languageChanged(LANGUAGE_CHINESE);
}

void MainWindow::languageENUS()
{
    languageChanged(LANGUAGE_ENGLISH);
}


void MainWindow::toolBarAddAction(bool addDir)
{
    if (toolBarList.count() >= MAX_TOOLBAR_COUNT) {
        return;
    }

    QString path;
    QString dialogName = tr("Add Button");
    if (addDir)
        path = QFileDialog::getExistingDirectory(this, dialogName);
    else
        path = QFileDialog::getOpenFileName(this, dialogName);
    path = QDir::toNativeSeparators(path);

    if (QFileInfo::exists(path) && !toolBarList.contains(path, Qt::CaseInsensitive)) {
        qDebug() << "add quick path: " << path;

        QFileInfo info(path);
        QString text = QDir::toNativeSeparators(info.fileName().isEmpty()?path:info.fileName());
        QAction *newAct = new QAction;
        newAct->setIcon(fileModel->iconProvider()->icon(info));
        newAct->setText(text);
        newAct->setToolTip(path);

        // get the "+" action
        QAction *action = toolBar->actions().at(toolBarList.count());
        toolBar->insertAction(action, newAct);

        toolBarList.append(path);
        if (toolBarList.count() >= MAX_TOOLBAR_COUNT) {
            action->setEnabled(false);
        }
    } else {
        qDebug() << "quick path exist: " << path;
    }
}

void MainWindow::onToolBarActionTriggered(QAction *action)
{
    QString path = action->toolTip();
    if (QFileInfo::exists(path)) {
        qDebug() << QString("tool bar clicked %1").arg(path);
//        ((FileWidget *)centralWidget())->onNavigateBarClicked(path);
        ((FileWidget *)centralWidget())->onItemActivated(path);
    } else {
        toolBarAddAction();
    }
}

void MainWindow::toolBarOnTextMenu(const QPoint &pos)
{
    QMenu menu;
    QAction *action;

    // menu actions
    QAction *addDirAction = nullptr;
    QAction *addFileAction = nullptr;
//    QAction *deleteAction = nullptr;
    QAction *deleteAllAction = nullptr;

    addDirAction = menu.addAction(tr("&Add Directory"));
    if (toolBarList.count() >= MAX_TOOLBAR_COUNT) {
        addDirAction->setEnabled(false);
    }
    addFileAction = menu.addAction(tr("Add &File"));
    if (toolBarList.count() >= MAX_TOOLBAR_COUNT) {
        addFileAction->setEnabled(false);
    }

    menu.addSeparator();

    QMenu *deleteMenu = menu.addMenu(tr("&Delete"));
    deleteAllAction = menu.addAction(tr("D&elete All"));
    if (toolBarList.isEmpty()) {
        deleteMenu->setEnabled(false);
        deleteAllAction->setEnabled(false);
    } else {
        for (int i = 0; i < toolBarList.count(); i++) {
            QString path = toolBarList.at(i);
            QAction *deleteAction = deleteMenu->addAction(path);
            deleteAction->setData(i);
        }
    }


    qDebug() << "toolBarOnTextMenu";

    action = menu.exec(toolBar->mapToGlobal(pos));
    if (!action)
        return;

    // handle all selected items
    if (action == addDirAction) {
        toolBarAddAction();

    } else if (action == addFileAction) {
        toolBarAddAction(false);

    } else if (action == deleteAllAction) {
        toolBar->clear();
        toolBar->addAction("+");
        toolBarList.clear();
//        if (toolBarList.count() < MAX_TOOLBAR_COUNT) {
            QAction *action = toolBar->actions().at(toolBarList.count());
            action->setEnabled(true);
//        }

    } else if (action != nullptr) {
        int index = action->data().toInt();
        qDebug() << QString("delete [%1]: %2").arg(index).arg(action->text());

        toolBar->removeAction(toolBar->actions().at(index));
        toolBarList.removeAt(index);
        if (toolBarList.count() < MAX_TOOLBAR_COUNT) {
            QAction *action = toolBar->actions().at(toolBarList.count());
            action->setEnabled(true);
        }
    }
}

void MainWindow::onFindFiles(const QString &path, const QString &find)
{
    ((FindWidget *)findDock->widget())->setFindInfo(path, find);
    findDock->setVisible(true);
    findDock->raise();
    ((FindWidget *)findDock->widget())->animateFindClick();
}

// show about message
void MainWindow::about()
{
    static const char message[] =
        "<p><b>FileManagerDemo</b></p>"

        "<p>Version:&nbsp;Beta2(x64)</p>"
        "<p>Author:&nbsp;&nbsp;Javier Zhou</p>"
        "<p>Date:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;2021/10/07</p>"

        "<p></p>"
        "<p>Project:&nbsp;&nbsp;<a href=\"https://github.com/Jawez/FileManager\">Github repository</a>"
        "<p>Video:&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"https://www.bilibili.com/video/BV1ng411L7gx\">BiliBili video</a>"
        ;

//    QMessageBox::about(this, tr("About"), message);
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle(tr("About"));
    msgBox->setText(message);
    QPixmap pm(QLatin1String(":/resources/icon_app_64.png"));
    if (!pm.isNull())
        msgBox->setIconPixmap(pm);

    msgBox->exec();
}

void MainWindow::loadWindowInfo()
{
    qDebug() << QString("loadWindowInfo");

    QVariant geometry = readSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_GEOMETRY);
//    qDebug() << geometry;
    if (geometry.isValid()) {
        bool result = restoreGeometry(geometry.toByteArray());
        qDebug() << QString("restoreGeometry result %1").arg(result);
    } else {
        // resize window
        QSize aSize = qGuiApp->primaryScreen()->availableSize();
        qDebug() << aSize;
        resize(aSize * 0.618);
    }
    QVariant state = readSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_STATE);
//    qDebug() << state;
    if (state.isValid()) {
        bool result = restoreState(state.toByteArray());
        qDebug() << QString("restoreState result %1").arg(result);
    } else {
        fileDock->setHidden(true);
        findDock->setHidden(true);
    }

//    QVariant size = readSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_SIZE);
//    qDebug() << size;
//    if (size.isValid()) {
//        resize(size.toSize());
//    } else {
//        // resize window
//        QSize aSize = qGuiApp->primaryScreen()->availableSize();
//        qDebug() << aSize;
//        resize(aSize * 0.618);
//    }

//    QVariant pos = readSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_POS);
//    qDebug() << pos;
//    if (pos.isValid()) {
//        move(pos.toPoint());
//    }

    QStringList pathList = readArraySettings(CONFIG_GROUP_TOOLBAR);
    qDebug() << "pathList: " << pathList;
    if (pathList.isEmpty()) {
        foreach (QFileInfo info, QDir::drives()) {
            QString path = info.absolutePath();     // example "C:/", file's path absolute path. This doesn't include the file name
            path = QDir::toNativeSeparators(path);  // example "C:\\"
            toolBar->addAction(fileModel->iconProvider()->icon(info), path);

            toolBarList.append(path);
        }
    } else {
        foreach (QString path, pathList) {
            path = QDir::toNativeSeparators(path);
//            toolBar->addAction(fileModel->iconProvider()->icon(QFileInfo(path)), path);

            QFileInfo info(path);
            QString text = QDir::toNativeSeparators(info.fileName().isEmpty()?path:info.fileName());
            QAction *newAct = new QAction;
            newAct->setIcon(fileModel->iconProvider()->icon(info));
            newAct->setText(text);
            newAct->setToolTip(path);
            toolBar->addAction(newAct);

            toolBarList.append(path);
        }
    }
//    toolBar->addSeparator();
    toolBar->addAction("+");
}

void MainWindow::saveWindowInfo()
{
    qDebug() << QString("saveWindowInfo") << size() << pos();

    writeSettings(CONFIG_GROUP_DEFAULT, CONFIG_DEF_SYSTRAY, QVariant(trayIcon != nullptr));

    writeSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_GEOMETRY, saveGeometry());
    writeSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_STATE, saveState());
//    writeSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_SIZE, size());
//    writeSettings(CONFIG_GROUP_WINDOW, CONFIG_WIN_POS, pos());

//    QStringList toolBarList;
//    foreach (QAction *act, toolBar->actions()) {
//        QFileInfo info(act->toolTip());
////        if (info.fileName() != "." && info.fileName() != "..") {
//        if (QFileInfo::exists(act->toolTip())) {
//            toolBarList.append(info.absoluteFilePath());
//        }
//    }
//    qDebug() << "toolBarList: " << toolBarList;
    writeArraySettings(CONFIG_GROUP_TOOLBAR, toolBarList);

    ((FileWidget *)centralWidget())->saveFileWidgetInfo();

    navDock->saveDockInfo();
    fileDock->saveDockInfo();
    ((FindWidget *)findDock->widget())->saveFindWidgetInfo();
}

