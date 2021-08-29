#include "mainwindow.h"
#include "filewidget.h"

#include <QtDebug>
#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QProcess>
#include <QAction>
#include <QScreen>
#include <QDockWidget>
#include <QMessageBox>
#include <QSplitter>
#include <QMenuBar>
#include <QWidget>
#include <QMenu>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    appLanguage = LANGUAGE_CHINESE;
    loadTranslation();

    setWindowTitle(tr("FileManagerDemo"));

    // resize window
    QSize aSize = qGuiApp->primaryScreen()->availableSize();
    qDebug() << aSize;
    resize(aSize * 0.618);

    fileModel = new FileSystemModel();
    fileDock = new QDockWidget();
    dirDock = new DockWidget(fileModel);

    cutShortcut = new QShortcut(this);
    copyShortcut = new QShortcut(this);
    pasteShortcut = new QShortcut(this);
    deleteShortcut = new QShortcut(this);
    refreshShortcut = new QShortcut(this);
    expCollOneShortcut = new QShortcut(this);
    expCollAllShortcut = new QShortcut(this);

    fileModelInit();
    setupWidgets();
    setupMenus();
    setupShortCut();

    // connect slots
    connect(refreshShortcut, &QShortcut::activated, dirDock, &DockWidget::refreshTreeView);

//    FileWidget *widget = (FileWidget *)((QSplitter *)centralWidget())->widget(0);
    FileWidget *widget = (FileWidget *)centralWidget();
    connect(dirDock, &DockWidget::dockWidgetClicked, widget, &FileWidget::onNavigateBarClicked);
    connectShortcut(widget);
    widget = (FileWidget *)fileDock->widget();
    connectShortcut(widget);
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

    dirDock->deleteLater();
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
    fileModel->setFilter(QDir::AllEntries | QDir::NoDot | QDir::AllDirs | QDir::System/* | QDir::Hidden*/);
    fileModel->setReadOnly(false);
}

void MainWindow::setupWidgets()
{
    // central widget
    FileWidget *widget = new FileWidget(fileModel, this);
//    QSplitter *splitter = new QSplitter;
//    splitter->addWidget(widget);
    setCentralWidget(widget);

    // dock
    dirDock->setWindowTitle(tr("Navigation Bar"));  // show in the dock
    addDockWidget(Qt::LeftDockWidgetArea, dirDock);

    // file dock
    FileWidget *fileWidget = new FileWidget(fileModel, this);
    fileDock->setWindowTitle(tr("File Dock"));
    fileDock->setWidget(fileWidget);
    addDockWidget(Qt::RightDockWidgetArea, fileDock);
    fileDock->close();      // default close, close() invalid before addDockWidget()

    connect(fileDock, &QDockWidget::visibilityChanged, fileWidget, &FileWidget::onVisibilityChanged);

}

void MainWindow::setupMenus()
{
    // file menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *exitAction = fileMenu->addAction(tr("E&xit"), qApp, &QCoreApplication::quit);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setShortcut(Qt::CTRL | Qt::Key_Q);

    // view menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    dirDock->toggleViewAction()->setText(tr("Navi&gation Bar"));
    dirDock->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_G);
    viewMenu->addAction(dirDock->toggleViewAction());

    fileDock->toggleViewAction()->setText(tr("&File Dock"));
    fileDock->toggleViewAction()->setShortcut(Qt::CTRL | Qt::Key_I);
    viewMenu->addAction(fileDock->toggleViewAction());

//    FileWidget *widget = (FileWidget *)((QSplitter *)centralWidget())->widget(0);
    FileWidget *widget = (FileWidget *)centralWidget();
    QAction *addTabAct = viewMenu->addAction(tr("Add &Tab"), widget, &FileWidget::fileWidgetAddTab);
    addTabAct->setShortcut(Qt::CTRL | Qt::Key_T);

    // options menu
    QMenu *optionsMenu = menuBar()->addMenu(tr("&Options"));

    QMenu *languageMenu = optionsMenu->addMenu(tr("&Language"));
    QAction *zhCNAct = new QAction(QString("简体中文"), this);
    zhCNAct->setCheckable(true);
    connect(zhCNAct, &QAction::triggered, this, &MainWindow::languageZHCN);
    QAction *enUSAct = new QAction(QString("English"), this);
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

void MainWindow::loadTranslation()
{
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);

    // default "General" section
    QString language = settings.value("language", QVariant(LANGUAGE_CHINESE)).toString();
    qDebug() << QString("load language %1").arg(language);
    if (language.compare(LANGUAGE_CHINESE) == 0) {
        // do nothing
    } else if (language.compare(LANGUAGE_ENGLISH) == 0) {
        appLanguage = language;
        return;     // use default text, do not load translator
    } else {
//        QString locale = QLocale::system().name();    // locale zh_CN
        language = QString(LANGUAGE_CHINESE);
        settings.setValue("language", QVariant(language));
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
    QString configFilePath = QCoreApplication::applicationDirPath() + CONFIG_FILE;
    QSettings settings(configFilePath, QSettings::IniFormat);
    settings.setValue("language", QVariant(lang));

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


// show about message
void MainWindow::about()
{
    static const char message[] =
        "<p><b>FileManagerDemo</b></p>"

        "<p>This is a demo of the file manager.</p>"
        "<p></p>"
        "<p>author Javier Zhou</p>"
        "<p>date   2021/08/21</p>"
        ;

    QMessageBox::about(this, tr("About"), message);
}

void MainWindow::aboutQt()
{
    qApp->aboutQt();
}
