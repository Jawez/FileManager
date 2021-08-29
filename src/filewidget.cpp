#include "filewidget.h"

#include <QtDebug>
#include <QApplication>
#include <QStyleFactory>
#include <QGridLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QSplitter>
#include <QLineEdit>
#include <QFileIconProvider>
#include <QDesktopServices>
#include <QClipboard>
#include <QIcon>
#include <QUrl>


FileWidget::FileWidget(QAbstractItemModel *model, QWidget *parent) : QWidget(parent)
{
    // init file model
    FileSystemModel *fileModel = (FileSystemModel *)model;
//    connect(fileModel, &FileSystemModel::fileRenamed, this, &FileWidget::onFileRenamed);
    connect(fileModel, &FileSystemModel::directoryLoaded, this, &FileWidget::onDirectoryLoaded);
    connect(fileModel, &FileSystemModel::dropCompleted, this, &FileWidget::onDropCompleted);
    connect(fileModel, &FileSystemModel::pasteCompleted, this, &FileWidget::onPasteCompleted);

    // init myComputer info
    myComputer.name = fileModel->myComputer(Qt::DisplayRole).toString();
    myComputer.icon = fileModel->myComputer(Qt::DecorationRole).value<QIcon>();
//    myComputer.icon = fileModel->iconProvider()->icon(QFileIconProvider::Computer);


    prevBtn = new QPushButton;
    nextBtn = new QPushButton;
    listBtn = new QPushButton;
    pathBox = new QComboBox;

    proxyModel = new FileFilterProxyModel;
    proxyModel->setSourceModel(fileModel);

    currDir = new QDir;
//    currDir->cd(QDir::toNativeSeparators("C:\\"));
    qDebug() << currDir->absolutePath();

    tabWidget = new QTabWidget;

    statLab = new QLabel("");
    selectionLab = new QLabel("");


    treeViewPath = myComputer.name;


    // widget init
    tabWidgetInit();
    labelInit();
    historyCtrlInit();
    pathBoxInit();
    widgetLayoutInit();

    // add first tab
    fileWidgetAddTab();
}

FileWidget::~FileWidget()
{
    prevBtn->deleteLater();
    nextBtn->deleteLater();
    listBtn->deleteLater();
    pathBox->deleteLater();

    foreach (tabInfo *tab, tabList) {
        // release his member
        historyPath *his = tab->his;
        his->menu->deleteLater();   // delete menu
        his->menu = nullptr;
        his->act.clear();           // clear act list

        delete his;                 // delete history
    }
    // release tabList
    qDeleteAll(tabList);            // delete all tabInfo
    tabList.clear();                // remove all tabInfo
    tabWidget->deleteLater();       // delete tabWidget(auto delete all treeView)

    delete currDir;

    proxyModel->deleteLater();

    statLab->deleteLater();
    selectionLab->deleteLater();

    treeView = nullptr;
    history = nullptr;
}

QSize FileWidget::sizeHint() const
{
    // treeView header width
    int w = HEADER_SIZE_NAME + HEADER_SIZE_DEFAULT + HEADER_SIZE_DEFAULT + HEADER_SIZE_MODIFIED;
    return QSize(w, 500);
}


// reimp the recommended size
class PushButton : public QPushButton
{
public:
    QSize sizeHint() const override;
};

QSize PushButton::sizeHint() const
{
    QSize size = QPushButton::sizeHint();
    size.setWidth(30 + 2);  // width and margin
    return size;
}

void FileWidget::tabWidgetInit()
{
    QTabBar *tabBar = tabWidget->tabBar();

    tabWidget->setMovable(true);
    tabWidget->setTabPosition(QTabWidget::North);   // default
    tabWidget->setTabShape(QTabWidget::Rounded);    // default
    tabWidget->setTabsClosable(true);

    PushButton *addButton = new PushButton;         // be deleted by the tab widget
    addButton->setText("+");
//    addButton->setFixedWidth(30);                   // already reimp sizeHint()
    tabWidget->setCornerWidget(addButton, Qt::TopRightCorner);  // default Qt::TopRightCorner

    connect(addButton, &QAbstractButton::clicked, this, &FileWidget::onTabBarAddTab);

//    connect(tabBar, &QTabBar::tabBarClicked, this, &FileWidget::onTabBarClicked);       // handle in currentChanged()'s slot
    connect(tabBar, &QTabBar::currentChanged, this, &FileWidget::onCurrentChanged);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &FileWidget::onTabCloseRequested);
}

void FileWidget::addViewTab(QTreeView *view)
{
    tabInfo *info = new tabInfo;
    historyPath *his = new historyPath;
    QMenu *menu = new QMenu;
    connect(menu, &QMenu::triggered, this, &FileWidget::onRecordTriggered);

    his->pos = 0;
    his->menu = menu;
    his->path = myComputer.name;
    info->path = myComputer.name;
    info->his = his;

    treeViewPath = myComputer.name;

    tabList.append(info);

    history = his;
    listBtn->setMenu(menu);

    // this will trigger onCurrentChanged()
    int index = tabWidget->addTab(view, myComputer.icon, myComputer.name);
    tabWidget->setCurrentIndex(index);

//    qDebug() << view->parent();   // QStackedWidget(0x1888fc0, name = "qt_tabwidget_stackedwidget")
}

void FileWidget::addTreeView()
{
    TreeView *view = new TreeView;
    addViewTab(view);

    treeView = view;
//    treeView->setModel(fileModel);
    treeView->setModel(proxyModel);

    // sort settings
    treeView->setSortingEnabled(true);
    treeView->sortByColumn(0, Qt::AscendingOrder);  // starts with 'AAA' ends with 'ZZZ' in Latin-1 locales

    // interactive settings
    treeView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);          // default
    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setDropIndicatorShown(true);
    treeView->setDragDropMode(QAbstractItemView::DragDrop);     // move target on FileSystemModel, not copy

    // view settings
    treeView->setStyle(QStyleFactory::create("Fusion"));
    treeView->setTextElideMode(Qt::ElideRight);     // default, set text elide mode
//    treeView->setRootIsDecorated(false);    // do not show expand button
//    treeView->setIndentation(20);       // indentation

    treeView->setUniformRowHeights(true);           // all items has same height. Enables the view to do some optimizations

    // header settings
    treeView->header()->setFirstSectionMovable(true);
    treeView->header()->setSectionsMovable(true);
    treeView->header()->setStretchLastSection(false);

    treeView->header()->setDefaultSectionSize(HEADER_SIZE_DEFAULT);     // only affects sections that (QHeaderView::)Interactive or Fixed
    treeView->header()->setSectionResizeMode(QHeaderView::Interactive);         // default, user can resize the section
    treeView->header()->resizeSection(0, HEADER_SIZE_NAME);             // Name column
    treeView->header()->resizeSection(3, HEADER_SIZE_MODIFIED);         // Data Modified column

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &FileWidget::contextMenu);

    connect(treeView, &QTreeView::expanded, this, &FileWidget::onExpanded);
    connect(treeView, &TreeView::treeViewGotFocus, this, &FileWidget::refreshTreeView);

//    connect(treeView->header(), &QHeaderView::sectionClicked, this, &FileWidget::onSectionClicked);
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileWidget::onSelectionChanged);

//    connect(treeView, &QTreeView::clicked, this, &FileWidget::onTreeViewClicked);
    connect(treeView, &QTreeView::doubleClicked, this, &FileWidget::onTreeViewDoubleClicked);
}

void FileWidget::labelInit()
{
    statLab->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    statLab->setMaximumWidth(STATUS_LAB_WIDTH_ITEM);
}

void FileWidget::historyCtrlInit()
{
    listBtn->setFixedWidth(HISTORY_WIDTH_MENUBTN);

    prevBtn->setIcon(QIcon(":/resources/icon_prev.png"));
    prevBtn->setFixedWidth(HISTORY_WIDTH_BUTTON);
    nextBtn->setIcon(QIcon(":/resources/icon_next.png"));
    nextBtn->setFixedWidth(HISTORY_WIDTH_BUTTON);
    connect(prevBtn, &QAbstractButton::clicked, this, &FileWidget::onPrevClicked);
    connect(nextBtn, &QAbstractButton::clicked, this, &FileWidget::onNextClicked);
}

void FileWidget::pathBoxInit()
{
    pathBox->setEditable(true);

    pathBox->addItem(myComputer.icon, myComputer.name);
    foreach (QFileInfo info, QDir::drives()) {
        QString path = info.absolutePath();     // example "C:/", file's path absolute path. This doesn't include the file name
        path = QDir::toNativeSeparators(path);  // example "C:\\"
        pathBox->addItem(proxyModel->iconProvider()->icon(info), path);
    }

//    connect(pathBox->lineEdit(), &QLineEdit::returnPressed, this, &FileWidget::onComboBoxReturnPressed);
    connect(pathBox, &QComboBox::activated, this, &FileWidget::onComboBoxActivated);
//    connect(pathBox, &QComboBox::currentIndexChanged, this, &FileWidget::onComboBoxIndexChanged);
}

void FileWidget::widgetLayoutInit()
{
    QGridLayout *gridLayout = new QGridLayout;
    this->setLayout(gridLayout);

    gridLayout->setContentsMargins(0, 1, 0, 2);
    gridLayout->setSpacing(1);

    // row 0
    gridLayout->addWidget(prevBtn, 0, 0);
    gridLayout->addWidget(nextBtn, 0, 1);
    gridLayout->addWidget(listBtn, 0, 2);
    gridLayout->addWidget(pathBox, 0, 3);

    // row 1
    gridLayout->addWidget(tabWidget, 1, 0, 1, 4);

    // row 2
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    QWidget *statusBar = new QWidget;
    statusBar->setLayout(hBoxLayout);
    hBoxLayout->addWidget(statLab);
    hBoxLayout->addWidget(selectionLab);
    hBoxLayout->setContentsMargins(1, 1, 1, 1);
    gridLayout->addWidget(statusBar, 2, 0, 1, 4);
}


static bool dirIsDotAndDotDot(const QString &dir)
{
    return (dir == "." || dir == "..");
}

static inline void openFile(const QString &fileName)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}


static void displayNotificationMessage(const QString &title, const QString &text, QMessageBox::Icon icon)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(icon);
    msgBox.addButton(FileWidget::tr("&Ok"), QMessageBox::ActionRole);
    msgBox.exec();
}

static void displayUnknownPathMessage(const QString &path)
{
    displayNotificationMessage(FileWidget::tr("Unknown path"),
                               FileWidget::tr("<p>Unknown path:</p>%1").arg(path),
                               QMessageBox::Information);
}

void FileWidget::onComboBoxReturnPressed()
{
    QString path = pathBox->currentText();
    qDebug() << QString("onComboBoxReturnPressed %1").arg(path);

}

void FileWidget::onComboBoxActivated(int index)
{
    QString path = pathBox->itemText(index);
    qDebug() << QString("onComboBoxActivated index %1, path %2").arg(index).arg(path);

    if (!QFileInfo::exists(path)) {
        if (path.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
            showMyComputer();
            updateRecord();
        } else {
            if (path.compare("\x62\x79", Qt::CaseInsensitive) == 0) {
                QMessageBox::about(this, "", "\x5A\x4A\x57");
            } else if (path.compare("\x63\x6F\x64\x65", Qt::CaseInsensitive) == 0) {
//                QMessageBox::about(this, "", "\x5A\x4A\x57");
            } else {
                displayUnknownPathMessage(path);
            }

            // remove invalid path
            pathBox->removeItem(index);

            QString currPath = treeViewPath;
            pathBox->setCurrentIndex(pathBox->findText(currPath));
        }
        return;
    }

    QFileInfo info(path);
    if (info.isDir()) {
        updateCurrentPath(path);
        updateRecord();
    } else {    // file
//        displayNotificationMessage(tr("Error path"), tr("<p>Error path:</p>%1").arg(path), QMessageBox::Information);
        openFile(path);
        pathBox->removeItem(index);
    }
}

void FileWidget::onComboBoxIndexChanged(int index)
{
    QString path = pathBox->itemText(index);
    qDebug() << QString("onComboBoxIndexChanged index %1, path %2").arg(index).arg(path);

}


void FileWidget::hisPathSwitch(const QString &path)
{
    history->path = path;
    updateCurrentPath(path);
}

void FileWidget::removeInvalidHisAction(QAction *action)
{
    QString path = action->text();
    // remove invalid action
    int count = history->act.count();
    for (int i = 0; i < count; i++) {
        if (action == history->act.at(i)) {
            displayUnknownPathMessage(path);

            // remove in pathBox
            int textIndex = pathBox->findText(path);
            if (textIndex != -1) {
                pathBox->removeItem(textIndex);
            }

//            qDebug() << QString("history pos %1, i %2").arg(history->pos).arg(i);

            // remove in history menu
            history->menu->removeAction(action);
            history->act.removeAt(i);
            if (history->pos >= i) {
                history->pos--;
            }

            // if the previous equal the next, remove one of them
            if (i > 0 && i < count - 1) {
                QAction *prevAct = history->act.at(i - 1);
                QAction *nextAct = history->act.at(i);
                qDebug() << QString("prev %1, i - 1 %2").arg(prevAct->text()).arg(i - 1);
                qDebug() << QString("next %1, i %2").arg(prevAct->text()).arg(i);
                if (prevAct->text() == nextAct->text()) {
                    if (history->pos < i) {
                        history->menu->removeAction(nextAct);
                        history->act.removeAt(i);
                        qDebug() << QString("history pos %1, remove next").arg(history->pos);
                    } else {
                        history->menu->removeAction(prevAct);
                        history->act.removeAt(i - 1);
                        history->pos--;
                        qDebug() << QString("history pos %1, remove prev").arg(history->pos);
                    }
                }
            }
            return;
        }
    }
}

void FileWidget::onRecordTriggered(QAction *action)
{
    QString path = action->text();
    if (!QFileInfo::exists(path)) {
        if (path.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
//            showMyComputer();
        } else {
            removeInvalidHisAction(action);
            return;
        }
    }

    hisPathSwitch(path);

    qDebug() << QString("onRecordTriggered pos %1").arg(history->pos);

    // cancel previous checked
    history->act.at(history->pos - 1)->setChecked(false);

    // update history position
    int count = 0;
    foreach (QAction *act, history->act) {
        count++;
        if (act == action) {
            history->pos = count;
//            qDebug() << QString("new pos %1").arg(history->pos);
            break;
        }
    }
}

void FileWidget::updateRecord()
{
    QString currPath = treeViewPath;
//    qDebug() << QString("updateRecord current path %1").arg(currPath);

    if (history->pos > 0 && history->path.compare(currPath) == 0) {
        qDebug() << QString("path %1 not change, his pos %2").arg(currPath).arg(history->pos);
        return;
    }

#if USE_INSERT_HISTORY_MENU
    QAction *act = new QAction;     // be deleted by the QMenu
    if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
        act->setIcon(myComputer.icon);
        act->setText(myComputer.name);
    } else {
        QFileInfo info;
        info.setFile(currPath);

        act->setIcon(proxyModel->iconProvider()->icon(info));
        act->setText(currPath);
    }

    // current action is not the first action or the last action, use insertAction
    if (history->pos > 0 && history->pos < history->act.count()) {
        history->menu->insertAction(history->act.at(history->pos), act);
    } else {
        history->menu->addAction(act);
    }
#else
    QAction *act;
    if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
        act = history->menu->addAction(myComputer.icon, myComputer.name);
    } else {
        QFileInfo info;
        info.setFile(currPath);

        act = history->menu->addAction(proxyModel->iconProvider()->icon(info), currPath);
    }
#endif
    // mark current act
    act->setCheckable(true);
    act->setChecked(true);
    if (history->pos > 0) {
        history->act.at(history->pos - 1)->setChecked(false);
    }

    history->path = currPath;
#if USE_INSERT_HISTORY_MENU
    // insert act to menu, need use insertAction
    history->act.insert(history->pos, act);
    history->pos++;
#else
    // append act to the end of menu
    history->act.append(act);
    history->pos = history->act.count();
#endif
//    qDebug() << QString("updateRecord pos %1").arg(history->pos);

    if (history->act.count() > MAX_HISTORY_COUNT) {
#if USE_INSERT_HISTORY_MENU
        // if there are actions after the action at pos, remove them
        if (history->pos < history->act.count()) {
            history->menu->removeAction(history->act.last());
            history->act.removeLast();
        }
        else
#endif
        {
            history->pos--;

            history->menu->removeAction(history->act.first());
            history->act.removeFirst();
        }
    }
}

void FileWidget::showMyComputer()
{
    qDebug() << QString("show myComputer");

    // update comboBox
    pathBox->setCurrentIndex(0);

    // update tree view
    QModelIndex index = proxyModel->proxyIndex(myComputer.name);  // invalid index and index.parent()
    treeView->setRootIndex(index);  // set invalid index here

    // update tab view
    int tabIndex = tabWidget->currentIndex();
    treeViewPath = myComputer.name;
    tabList.at(tabIndex)->path = treeViewPath;
    tabWidget->setTabText(tabIndex, myComputer.name);
    tabWidget->setTabIcon(tabIndex, myComputer.icon);

    // update status
    statLab->setText(tr(" %n item(s)", "number of the item", QDir::drives().count()));
    selectionLab->setText("");
}

void FileWidget::updateSelectionInfo()
{
    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
//    int count = selected.indexes().count();   // contain all column's index
    int count = indexes.count();
    qDebug() << QString("updateSelectionInfo %1").arg(count);
    if (count > 0) {
        qDebug() << QString(indexes.first().data().toString());
        QString text = QString("| %1").arg(tr("%n item(s) selected", "number of selected item", count));
        selectionLab->setText(text);
    } else {
        selectionLab->setText("");
    }
}

void FileWidget::updateStatusBar()
{
    QString currPath = treeViewPath;
    if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
        qDebug() << QString("my computer, do not update item info");
        return;     // return if located at myComputer
    }

    int count = currDir->entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::System/* | QDir::Hidden*/).count();
    QString text = tr(" %n item(s)", "number of the item", count);
    statLab->setText(text);

    updateSelectionInfo();
}

void FileWidget::updateCurrentPath(const QString &dir)
{
    if (dir == treeViewPath) {
        qDebug() << QString("current path %1 not change").arg(dir);
        return;
    }

    qDebug() << QString("updateCurrentPath dir %1").arg(dir);

    if (dir.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
        showMyComputer();
        return;
    }

    bool result = currDir->cd(dir);
    if (result == false) {
        qDebug() << QString("cd path failed");
        return;
    }

    // add path to comboBox
    QString currPath = QDir::toNativeSeparators(currDir->absolutePath());
    int textIndex = pathBox->findText(currPath);
    if (textIndex == -1) {
        QFileInfo info(currPath);
        pathBox->addItem(proxyModel->iconProvider()->icon(info), currPath);
    } else {
        // supplement path icon
        QVariant variant = pathBox->itemData(textIndex, Qt::DecorationRole);
        if (variant == QVariant::Invalid) {
            qDebug() << QString("pathBox index %1 no icon").arg(textIndex);
            QFileInfo info(currPath);
            pathBox->setItemIcon(textIndex, proxyModel->iconProvider()->icon(info));
        }
    }

    // update comboBox, pathBox->findText(currPath) >= 0
    pathBox->setCurrentIndex(pathBox->findText(currPath));

    QModelIndex index = proxyModel->proxyIndex(currPath);
    int tabIndex = tabWidget->currentIndex();
//    qDebug() << QString("current tab index %1").arg(tabIndex);
    // update view path
    treeViewPath = currPath;
    // update page path
    tabList.at(tabIndex)->path = treeViewPath;
    // update tab info
    tabWidget->setTabText(tabIndex, proxyModel->fileName(index));
    tabWidget->setTabIcon(tabIndex, proxyModel->iconProvider()->icon(proxyModel->fileInfo(index)));

    // refresh
    updateTreeView(currPath);

    // clear selection info
    treeView->selectionModel()->clearSelection();

    // update status
    updateStatusBar();
}

void FileWidget::updateCurrentTab(int index)
{
//    qDebug() << QString("updateCurrentTab index %1").arg(index);

    // update tab
    tabWidget->setCurrentIndex(index);

    // update treeView pointer
    QString currPath = tabList.at(index)->path;
    treeViewPath = currPath;
    qDebug() << QString("tab path %1").arg(currPath);
    treeView = (TreeView *)tabWidget->widget(index);
    // do not sort treeView here

    // update history pointer
    history = tabList.at(index)->his;
    listBtn->setMenu(history->menu);

    // update path
    bool result = currDir->cd(currPath);
    if (result == false) {
        qDebug() << QString("tab cd failed");
        if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0)
            showMyComputer();
        return;
    }

    // update comboBox, pathBox->findText(currPath) >= 0
    pathBox->setCurrentIndex(pathBox->findText(currPath));

    // refresh
    updateTreeView(currPath);

    // update status
    updateStatusBar();
}

void FileWidget::fileWidgetAddTab()
{
    if (tabWidget->count() >= MAX_TAB_COUNT) {
        return;
    }

    addTreeView();
    showMyComputer();
    updateRecord();

    qDebug() << QString("tab count %1").arg(tabWidget->count());
    if (tabWidget->count() >= MAX_TAB_COUNT) {
        PushButton *addButton = (PushButton *)tabWidget->cornerWidget(Qt::TopRightCorner);
        addButton->setEnabled(false);
    }
}


// private slot
void FileWidget::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    qDebug() << QString("onFileRenamed %1, %2->%3").arg(path).arg(oldName).arg(newName);

    QString tempRoot = QString("%1/%2").arg(path).arg(newName);
    // used to update the view of the proxy model, file model's view auto update
    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(path);
//    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(tempRoot);
}

void FileWidget::onDirectoryLoaded(const QString &path)
{
    QString currPath = QDir::fromNativeSeparators(treeViewPath);
//    qDebug() << QString("onDirectoryLoaded %1, currPath %2").arg(path, currPath);

    if (path == currPath)
        updateStatusBar();
}


void FileWidget::onTabBarAddTab()
{
    fileWidgetAddTab();
}

// replace by onCurrentChanged()
void FileWidget::onTabBarClicked(int index)
{
    qDebug() << QString("onTabBarClicked index %1").arg(index);

    // index out of range
    if (index < 0)
        return;

    // clicked tab to switch page
    if (index < tabList.count()) {
        updateCurrentTab(index);
    } else {    // else if (index >= tabWidget->tabBar()->count() - 1)
        fileWidgetAddTab();
    }
}


void FileWidget::onCurrentChanged(int index)
{
    qDebug() << QString("onCurrentChanged index %1").arg(index);

    // index out of range or only one treeview
    if (index < 0 || tabList.count() < 2)
        return;

    // clicked tab to switch page
    if (index < tabList.count()) {
        updateCurrentTab(index);
    } else {
        updateCurrentTab(tabList.count() - 1);
    }
}

void FileWidget::onTabCloseRequested(int index)
{
    qDebug() << QString("onTabCloseRequested index %1").arg(index);

    // index out of range or only one treeview
    if (index < 0 || tabList.count() < 2)
        return;

    // tabIndex <= tabList.count() - 1
    if (index < tabList.count()) {
        // close tab
        tabInfo *info = tabList.at(index);
        tabList.removeAt(index);            // remove tabInfo
        info->his->menu->deleteLater();     // delete menu
        info->his->menu = nullptr;
        info->his->act.clear();             // clear act list
        delete (historyPath *)info->his;    // delete history
        delete (tabInfo *)info;             // delete tabInfo

//        delete (TreeView *)tabWidget->widget(index);   // auto removeTab, but not safe, use deleteLater()
//        tabWidget->widget(index)->deleteLater();      // not delete immediately
        TreeView *view = (TreeView *)tabWidget->widget(index);
        // if currentIndex bigger than index, it will minus 1 after removeTab
        tabWidget->removeTab(index);        // remove treeView
        view->deleteLater();                // delete treeView

        int newIndex = tabWidget->currentIndex();
        // index must less than tabList.count()
        if (newIndex >= tabList.count()) {
            newIndex = tabList.count() - 1;
        }

        updateCurrentTab(newIndex);

        qDebug() << QString("tab count %1").arg(tabWidget->count());
        if (tabWidget->count() < MAX_TAB_COUNT) {
            PushButton *addButton = (PushButton *)tabWidget->cornerWidget(Qt::TopRightCorner);
            addButton->setEnabled(true);
        }
    }
}


void FileWidget::onExpanded(const QModelIndex &index)
{
    QString path = proxyModel->fileInfo(index).absoluteFilePath();
    qDebug() << QString("onExpanded %1").arg(path);

    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(path);
}

void FileWidget::onSectionClicked(int logicalIndex)
{
    qDebug() << QString("onSectionClicked %1").arg(logicalIndex);
}

void FileWidget::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    updateSelectionInfo();
}

void FileWidget::onTreeViewClicked(const QModelIndex &index)
{

}

void FileWidget::onTreeViewDoubleClicked(const QModelIndex &index)
{
    QString target;
    QFileInfo info = proxyModel->fileInfo(index);
    qDebug() << QString("open %1").arg(info.filePath());

    if (info.isShortcut()) {
        // handle shortcut
        if (!info.exists()) {
            QModelIndex srcIndex = proxyModel->mapToSource(index);
            ((FileSystemModel *)proxyModel->srcModel())->showShortcutInfo(srcIndex);
            return;;
        }
        target = info.symLinkTarget();
    } else {
        target = info.absoluteFilePath();
    }

    // If the file is a symlink, this function returns true if the target is a directory (not the symlink)
    if (info.isDir()) {
        updateCurrentPath(target);
        updateRecord();
    } else {    //    else if (info.isFile())
        openFile(target);
    }
}

void FileWidget::onPrevClicked()
{
    if (history->pos <= 1)
        return;

    int i = history->pos;

    // cancel previous checked
    history->act.at(i - 1)->setChecked(false);

    // update history position
    history->pos--;
    history->act.at(i - 2)->setChecked(true);

    QString nextPath = history->act.at(i - 2)->text();
    qDebug() << QString("onPrevClicked pos %1, i %2, path %3") \
                .arg(history->pos).arg(i).arg(nextPath);

    hisPathSwitch(nextPath);
}

void FileWidget::onNextClicked()
{
    if (history->pos >= history->act.count())
        return;

    int i = history->pos;

    // cancel previous checked
    history->act.at(i - 1)->setChecked(false);

    // update history position
    history->pos++;
    history->act.at(i)->setChecked(true);

    QString nextPath = history->act.at(i)->text();
    qDebug() << QString("onPrevClicked pos %1, i %2, path %3") \
                .arg(history->pos).arg(i).arg(nextPath);

    hisPathSwitch(nextPath);
}


// return dirPath with the '/' at the end, or ""
QString FileWidget::dirPathFromIndex(const QModelIndex &index)
{
    QString path = QString("");

    if (index.isValid()) {
        QFileInfo info = proxyModel->fileInfo(index);
        if (info.exists() && !info.isShortcut()) {
            if (info.isDir())
                path = QString("%1/").arg(info.absoluteFilePath());
            else    // for create shortcut context menu
                path = QString("%1/").arg(info.absolutePath());   // no fileName
        } else {
            return path;     // do nothing when selected a file or shortcut
        }
    } else {
        // handle the case of My computer
        QString currPath = treeViewPath;
        if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
            qDebug() << QString("my computer, cancel the operation");
            return path;     // return if located at myComputer
        }

        path = QString("%1/").arg(currDir->absolutePath());
    }

    return path;
}


void FileWidget::openIndexes(const QModelIndexList &indexes)
{
    bool addTab = false;
    foreach (QModelIndex index, indexes) {
        QFileInfo info = proxyModel->fileInfo(index);
        if (info.exists() && info.isDir()) {
            // add new tab if there are more than one dir
            if (addTab == true) {
                fileWidgetAddTab();
            } else {
                addTab = true;
            }
        }

        // open action
        onTreeViewDoubleClicked(index);
    }
}

void FileWidget::createFolder(const QString &name, const QModelIndex &index)
{
    QString targetPath = dirPathFromIndex(index);
    if (targetPath.isEmpty())
        return;

    QString targetName = QString("%1%2").arg(targetPath, name);
    for (int i = 1; QFileInfo::exists(targetName); i++) {
        targetName = QString("%1%2 (%3)").arg(targetPath, name).arg(i);
    }

//    bool result = currDir->mkdir(name);   // creates sub-directory
    bool result = currDir->mkpath(targetName);
    qDebug() << QString("createFolder result %1, dir %2").arg(result).arg(targetName);
    if (result) {
        // start editing the item if it is created success
        QModelIndex editIndex = proxyModel->proxyIndex(targetName);
        treeView->setCurrentIndex(editIndex);
        treeView->edit(editIndex);
    }
}

void FileWidget::createTextFile(const QString &name, const QModelIndex &index)
{
    QString targetPath = dirPathFromIndex(index);
    if (targetPath.isEmpty())
        return;

    QString targetName = QString("%1%2.txt").arg(targetPath, name);
    for (int i = 1; QFileInfo::exists(targetName); i++) {
        targetName = QString("%1%2 (%3).txt").arg(targetPath, name).arg(i);
    }

    // create file
    QFile file(targetName);
    bool result = file.open(QIODevice::NewOnly);
    qDebug() << QString("createTextFile result %1, file %2").arg(result).arg(targetName);
    if (result) {
        file.close();
        // start editing the item if it is created success
        QModelIndex editIndex = proxyModel->proxyIndex(targetName);
        treeView->setCurrentIndex(editIndex);
        treeView->edit(editIndex);
    }
}

bool FileWidget::isDirVisible(const QString &dir)
{
    QModelIndex index;
    // handle the case of My computer
    QString currPath = treeViewPath;
//    qDebug() << QString("treeViewPath %1, currDir->absolutePath() %2").arg(treeViewPath, currDir->absolutePath());
    if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
        index = proxyModel->index(0, 0);
    } else {
        QModelIndex parentIndex = proxyModel->proxyIndex(currDir->absolutePath());
        QFileInfo info = proxyModel->fileInfo(parentIndex);
//        qDebug() << QString("curr %1, isDir %2").arg(info.absoluteFilePath()).arg(info.isDir());
        if (info.isDir() && !info.isShortcut()) {
            if (dir == info.absoluteFilePath()) {
                qDebug() << QString("dir %1").arg(dir);
                return true;
            }
        }

        index = proxyModel->index(0, 0, parentIndex);
    }
    int row = 0;
    while (index.isValid()) {
        QString fileName = index.data().toString();
        if (dirIsDotAndDotDot(fileName)) {
            qDebug() << QString(".. continue");
            index = index.siblingAtRow(++row);
            continue;
        }

        QFileInfo info = proxyModel->fileInfo(index);
//        qDebug() << QString("curr %1, isDir %2").arg(info.absoluteFilePath()).arg(info.isDir());
        if (info.isDir() && !info.isShortcut()) {
            if (dir == info.absoluteFilePath()) {
                qDebug() << QString("dir %1").arg(dir);
                return true;
            }

            // whether dir is child of the item, add '/' to makesure dir contain the complete item name
            if (dir.contains(info.absoluteFilePath() + "/")) {
                qDebug() << QString("dir contains, isExpanded %1").arg(treeView->isExpanded(index));
                if (treeView->isExpanded(index)) {
                    row = 0;
                    index = proxyModel->index(0, 0, index);
                    continue;
                } else {
                    qDebug() << QString("dir %1 not visible").arg(dir);
                    return false;
                }
            }
        }

//        index = treeView->indexBelow(index);
        index = index.siblingAtRow(++row);
    }

    qDebug() << QString("dir %1 not visible").arg(dir);
    return false;
}

void FileWidget::onDropCompleted(const QString &dir, const QString &target)
{
    qDebug() << QString("onDropCompleted %1, %2").arg(QDir::cleanPath(dir), target);

//    bool hasCursor = treeView->geometry().contains(treeView->mapFromGlobal(QCursor::pos()));
//    qDebug() << QString("tree view hasCursor %1").arg(hasCursor) << treeView;

//    if (!hasCursor)
//        return;

    if (!treeView->isVisible() || !isDirVisible(QDir::cleanPath(dir))) {
        qDebug() << QString("item not visible") << treeView;
        return;
    }

    // expand treeView for drag drop
    treeView->expand(proxyModel->proxyIndex(dir));
    qDebug() << QString("expand %1").arg(dir);

    // auto select item
    treeView->setCurrentIndex(proxyModel->proxyIndex(target));
}

void FileWidget::onPasteCompleted(const QString &dir, const QString &target)
{
    qDebug() << QString("onPasteCompleted %1, %2").arg(QDir::cleanPath(dir), target);

//    if (!treeView->hasFocus())
//        return;

    if (!treeView->isVisible() || !isDirVisible(QDir::cleanPath(dir))) {
        qDebug() << QString("item not visible") << treeView;
        return;
    }

//    // copy paste auto expand when setCurrentIndex()
//    treeView->expand(proxyModel->proxyIndex(dir));
//    qDebug() << QString("expand %1").arg(dir);

    // set an item as the current item without selecting it:
    treeView->selectionModel()->setCurrentIndex(proxyModel->proxyIndex(target), QItemSelectionModel::NoUpdate);
}


void FileWidget::expandCollapseIndex(const QModelIndex &index, bool expand)
{
    if (index.isValid()) {
        int row = index.row();
        QModelIndex siblingIndex = index;
        while (siblingIndex.isValid()) {
//            qDebug() << QString("siblingIndex %1").arg(siblingIndex.data().toString());
            if (proxyModel->srcModel()->isDir(proxyModel->mapToSource(siblingIndex))) {
                if (expand) {
                    treeView->expand(siblingIndex);
//                    qDebug() << QString("expand %1").arg(siblingIndex.siblingAtColumn(0).data().toString());
                } else {
                    treeView->collapse(siblingIndex);
//                    qDebug() << QString("collapse %1").arg(siblingIndex.siblingAtColumn(0).data().toString());
                }
            };
            siblingIndex = siblingIndex.siblingAtRow(++row);
        }
    }
}

void FileWidget::expandCollapseOne()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);

    QModelIndex index;
    if (indexes.isEmpty()) {    // blank space
        return;
    } else {                    // selected item
        index = indexes.first();
    }

    if (!treeView->isExpanded(index))
        treeView->expand(index);
    else
        treeView->collapse(index);
}

void FileWidget::expandCollapseAll()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);

    QModelIndex index;
    if (indexes.isEmpty()) {    // blank space
        // handle the case of My computer
        QString currPath = treeViewPath;
        if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
            qDebug() << QString("my computer");
            index = proxyModel->index(0, 0);
        } else {
            QModelIndex parentIndex = proxyModel->proxyIndex(currDir->absolutePath());
            index = proxyModel->index(0, 0, parentIndex);
        }
    } else {                    // selected item
        index = indexes.first().sibling(0, 0);
    }
    expandCollapseIndex(index, !treeView->isExpanded(index));

//    treeView->expandAll();
//    treeView->collapseAll();
}


void FileWidget::setMimeDataAction(const QModelIndexList &indexes, Qt::DropAction action)
{
    qDebug() << QString("set mime data %1").arg(indexes.count());
    if (indexes.isEmpty()) {
        return;
    }

    QModelIndexList srcIndexes;
    for (int i = 0; i < indexes.count(); i++) {
        QModelIndex srcIndex = proxyModel->mapToSource(indexes.at(i));
        srcIndexes.append(srcIndex);
    }
    qDebug() << QString("set mime data src %1").arg(indexes.count());

    ((FileSystemModel *)proxyModel->srcModel())->setMimeData(srcIndexes, action);
}

void FileWidget::mimeDataAction(const QModelIndex &index, Qt::DropAction action)
{
    QString to = QDir::fromNativeSeparators(dirPathFromIndex(index));
    ((FileSystemModel *)proxyModel->srcModel())->handleMimeData(to, action);
}


void FileWidget::cutSelectedItem()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    qDebug() << QString("cut selected indexes %1").arg(indexes.count());

    setMimeDataAction(indexes, Qt::MoveAction);
}

void FileWidget::copySelectedItem()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    qDebug() << QString("copy selected indexes %1").arg(indexes.count());

    setMimeDataAction(indexes, Qt::CopyAction);
}

void FileWidget::pasteSelectedItem()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    qDebug() << QString("paste selected indexes %1").arg(indexes.count());
    if (indexes.count() > 1) {
        return;
    }

    Qt::DropAction mimeAct = ((FileSystemModel *)proxyModel->srcModel())->getMimeAct();
    if (mimeAct == Qt::MoveAction || mimeAct == Qt::CopyAction) {
        if (indexes.isEmpty())      // clicked blank space
            mimeDataAction();
        else {
            QModelIndex index = indexes.first();
            // handle "." and ".."
            QString fileName = index.siblingAtColumn(0).data().toString();
            qDebug() << QString("paste to %1").arg(fileName);
            if (dirIsDotAndDotDot(fileName)) {
                qDebug() << QString(".. return");
                return;
            }

            QFileInfo info = proxyModel->fileInfo(index);
            // do nothing when selected shortcut
            if (!info.isShortcut()) {
                if (info.isDir()) {
                    mimeDataAction(indexes.first());   // only set index for dir
                } else {
                    // the dirPathFromIndex() in mimeDataAction() will return files absolutePath(), no filename
                    mimeDataAction(indexes.first());
                }
            }
        }
    }
}

void FileWidget::deleteSelectedItem()
{
    if (!treeView->hasFocus())
        return;

    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    qDebug() << QString("delete selected indexes %1").arg(indexes.count());

    foreach (QModelIndex index, indexes) {
        // handle "." and ".."
        QString fileName = index.siblingAtColumn(0).data().toString();
        qDebug() << QString("delete %1").arg(fileName);
        if (dirIsDotAndDotDot(fileName)) {
            qDebug() << QString(".. continue");
            continue;
        }
        // type
        if (index.siblingAtColumn(2).data().toString() == "Drive") {
            qDebug() << QString("Drive continue");
            continue;
        }

        QString path = proxyModel->fileInfo(index).absoluteFilePath();
#if !DISABLE_FILE_WATCHER
        // remove model watcher
        if (((FileSystemModel *)proxyModel->srcModel())->removeDirWatcher(path))
#endif
        {
            ((FileSystemModel *)proxyModel->srcModel())->deleteTarget(path);
        }

        refreshTreeView();  // refresh the shortcut's dir of the deleted target
    }
}

void FileWidget::refreshExpandedFolder(const QString &dir)
{
    qDebug() << QString("refreshExpandedFolder %1").arg(dir);
    if (dir.isEmpty())
        return;

    FileSystemModel *fileModel = (FileSystemModel *)proxyModel->srcModel();

    // reset root path, make the model fetch files or directories
    QString root = fileModel->rootPath();
    fileModel->setRootPath("");

    QModelIndex index = proxyModel->proxyIndex(dir);
    if (!index.isValid()) {
        index = proxyModel->index(0, 0);
    }
    qDebug() << QString("index path %1").arg(proxyModel->fileInfo(index).absoluteFilePath());

    while (index.isValid()) {
        if (treeView->isExpanded(index)) {
            QFileInfo info = proxyModel->fileInfo(index);
            if (!dirIsDotAndDotDot(info.fileName())) {
                qDebug() << QString("expanded %1, %2").arg(treeView->isExpanded(index)).arg(info.absoluteFilePath());
                fileModel->setRootPath(info.absoluteFilePath());
            }
        }
        index = treeView->indexBelow(index);
    }

    fileModel->setRootPath(root);
}

void FileWidget::refreshTreeView()
{
    if (!treeView->isVisible()) {
//        qDebug() << QString("treeView not visible") << treeView;
        return;
    }

    qDebug() << QString("refresh");
    updateTreeView(treeViewPath);

    // clear selection info
    treeView->selectionModel()->clearSelection();
}

void FileWidget::updateTreeView(const QString &dir)
{
    if (dir.isEmpty())
        return;

    // reset root path, fetch files or directories
    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(dir);
    refreshExpandedFolder(dir);

    // update page area(tree view and tab)
    QModelIndex index = proxyModel->proxyIndex(dir);
    treeView->setRootIndex(index);
    treeView->sortByColumn(0, Qt::AscendingOrder);
    treeView->update();
}


void FileWidget::contextMenu(const QPoint &pos)
{
//    if (treeView->indexAt(pos).isValid()) {
//        // directory or file
//    } else {
//        // blank space
//    }

    QMenu menu;
    QAction *action;

    // menu actions
    QAction *openAction = nullptr;
    QAction *copyPathAction = nullptr;

    QAction *expandAction = nullptr;
    QAction *collapseAction = nullptr;
    QAction *expandAllAction = nullptr;
    QAction *collapseAllAction = nullptr;

    QAction *cutAction = nullptr;
    QAction *copyAction = nullptr;

    QAction *symlinkAction = nullptr;
    QAction *deleteAction = nullptr;
//    QAction *pDeleteAction = nullptr;
    QAction *renameAction = nullptr;
    QAction *linkInfoAction = nullptr;

    // blank space actions
    QAction *refreshAction = nullptr;

    QAction *pasteAction = nullptr;
    QAction *pasteSymlinkAction = nullptr;
//    QAction *undoAction = nullptr;

    QAction *newFolderAction = nullptr;
    QAction *newTextAction = nullptr;

//    QAction *propertyAction = nullptr;


    QModelIndexList indexes = treeView->selectionModel()->selectedRows(0);
    qDebug() << QString("context menu selected indexes %1").arg(indexes.count());

    // add actions and menus
    if (indexes.count() < 1) {          // blank space
        refreshAction = menu.addAction(tr("R&efresh"));
//        refreshAction->setShortcut(QKeySequence::Refresh);    // display the shortcut

        // handle the case of My computer
        QString currPath = treeViewPath;
        if (currPath.compare(myComputer.name, Qt::CaseInsensitive) != 0) {  // not myComputer
            expandAllAction = menu.addAction(tr("E&xpand All"));
            collapseAllAction = menu.addAction(tr("Coll&apse All"));

            menu.addSeparator();
            pasteAction = menu.addAction(tr("&Paste"));
            pasteSymlinkAction = menu.addAction(tr("Paste &Shortcut"));
//            undoAction = menu.addAction(tr("&Undo"));
            menu.addSeparator();
            QMenu *newMenu = menu.addMenu(tr("Ne&w"));
            newFolderAction = newMenu->addAction(tr("&Folder"));
            menu.addSeparator();
            newTextAction = newMenu->addAction(tr("&Text"));
            menu.addSeparator();
//            propertyAction = menu.addAction(tr("Prope&rty"));
        }

    } else if (indexes.count() > 1) {   // more than one selected items
        // remove "." and ".."
        for (int i = indexes.count() - 1; i >= 0; i--) {
            QString fileName = indexes.at(i).siblingAtColumn(0).data().toString();
            if (dirIsDotAndDotDot(fileName)) {
                indexes.removeAt(i);
                qDebug() << QString("remove .. at %1, count %2").arg(i).arg(indexes.count());
            }
        }

        openAction = menu.addAction(tr("&Open"));

        QString currPath = treeViewPath;
        if (currPath.compare(myComputer.name, Qt::CaseInsensitive) != 0) {  // not myComputer
            menu.addSeparator();
            cutAction = menu.addAction(tr("Cu&t"));
            copyAction = menu.addAction(tr("&Copy"));
            menu.addSeparator();
            deleteAction = menu.addAction(tr("&Delete"));
//            pDeleteAction = menu.addAction(tr("Permanently D&elete"));
        }

    } else {                            // only one selected item
        QFileInfo info = proxyModel->fileInfo(indexes.first());
        qDebug() << QString("path %1 isDir %2, isFile %3, isShortcut %4, isRoot %5") \
                    .arg(info.absoluteFilePath()) \
                    .arg(info.isDir()).arg(info.isFile()).arg(info.isShortcut()).arg(info.isRoot());
        qDebug() << QString("Filename %1, sibling0 %2") \
                    .arg(info.fileName(), indexes.first().siblingAtColumn(0).data().toString());

        openAction = menu.addAction(tr("&Open"));
        copyPathAction = menu.addAction(tr("Copy Pat&h"));

        if (info.isShortcut()) {        // shortcut
            if (info.exists()) {
                menu.addSeparator();
                cutAction = menu.addAction(tr("Cu&t"));
                copyAction = menu.addAction(tr("&Copy"));
            }
            menu.addSeparator();
            deleteAction = menu.addAction(tr("&Delete"));
            menu.addSeparator();
            linkInfoAction = menu.addAction(tr("Shortcut &Info"));

        } else if (info.isRoot()) {     // drive
            // do nothing

        } else {                        // dir or file
            // handle ".."
            if (!dirIsDotAndDotDot(info.fileName())) {
                menu.addSeparator();
                if (info.isDir()) {
                    expandAction = menu.addAction(tr("Expa&nd"));
                    collapseAction = menu.addAction(tr("Colla&pse"));
                }
                expandAllAction = menu.addAction(tr("E&xpand All"));
                collapseAllAction = menu.addAction(tr("Coll&apse All"));
                menu.addSeparator();
                cutAction = menu.addAction(tr("Cu&t"));
                copyAction = menu.addAction(tr("&Copy"));
                if (info.isDir()) { // for dir item
                    menu.addSeparator();
                    pasteAction = menu.addAction(tr("&Paste"));
                    pasteSymlinkAction = menu.addAction(tr("Paste &Shortcut"));
                } else {            // for file item
                    symlinkAction = menu.addAction(tr("Create &Shortcut"));
                }
                menu.addSeparator();
                deleteAction = menu.addAction(tr("&Delete"));
//                pDeleteAction = menu.addAction(tr("Permanently D&elete"));
                renameAction = menu.addAction(tr("Rena&me"));
                if (info.isDir()) {
                    menu.addSeparator();
                    QMenu *newMenu = menu.addMenu(tr("Ne&w"));
                    newFolderAction = newMenu->addAction(tr("&Folder"));
                    menu.addSeparator();
                    newTextAction = newMenu->addAction(tr("&Text"));
                }
            }

//            menu.addSeparator();
//            propertyAction = menu.addAction(tr("Prope&rty"));
        }
    }


    action = menu.exec(treeView->mapToGlobal(pos));
    if (!action)
        return;

    // handle all selected items
    if (action == openAction) {
        openIndexes(indexes);

    } else if (action == copyPathAction) {
        QFileInfo info = proxyModel->fileInfo(indexes.first());  // only handle the first one
        QString path;
        if (info.isShortcut()) {
            path = QDir::toNativeSeparators(info.symLinkTarget());
        } else {
            path = QDir::toNativeSeparators(info.absoluteFilePath());
        }
//        QGuiApplication::clipboard()->setText(path);
        qApp->clipboard()->setText(path);
        qDebug() << QString("path %1").arg(path);

    } else if (action == expandAction) {
        treeView->expand(indexes.first());      // only handle the first one

    } else if (action == collapseAction) {
        treeView->collapse(indexes.first());    // only handle the first one

    } else if (action == expandAllAction || action == collapseAllAction) {
        QModelIndex index;
        if (indexes.isEmpty()) {    // blank space
            // handle the case of My computer
            QString currPath = treeViewPath;
            if (currPath.compare(myComputer.name, Qt::CaseInsensitive) == 0) {
                qDebug() << QString("my computer");
                index = proxyModel->index(0, 0);
            } else {
                QModelIndex parentIndex = proxyModel->proxyIndex(currDir->absolutePath());
                index = proxyModel->index(0, 0, parentIndex);
            }
        } else {                    // selected item
            index = indexes.first().sibling(0, 0);
        }
        expandCollapseIndex(index, action == expandAllAction);

    } else if (action == cutAction) {
        setMimeDataAction(indexes, Qt::MoveAction);

    } else if (action == copyAction) {
        setMimeDataAction(indexes, Qt::CopyAction);

    } else if (action == symlinkAction) {   // only for one item situation
        setMimeDataAction(indexes, Qt::CopyAction);
        mimeDataAction(indexes.first(), Qt::LinkAction);

    } else if (action == deleteAction) {
        deleteSelectedItem();

//    } else if (action == pDeleteAction) {
//        foreach (QModelIndex index, indexes) {
//            QModelIndex srcIndex = proxyModel->mapToSource(index);
////            QDir::removeRecursively()
////            bool result = proxyModel->srcModel()->rmdir(srcIndex);   // return false if the dir containing files, dir must be empty
//            bool result = proxyModel->srcModel()->remove(srcIndex);  // remove file or removeRecursively dir
//            qDebug() << QString("remove result %1, name %2").arg(result).arg(proxyModel->fileName(index));
//        }
//#if DISABLE_FILE_WATCHER
//        refreshExpandedFolder(treeViewPath);
//#endif

    } else if (action == renameAction) {
        // TODO batch rename
        QModelIndex editIndex = indexes.first().siblingAtColumn(0); // only handle the first one
        treeView->setCurrentIndex(editIndex);
        treeView->edit(editIndex);

    } else if (action == linkInfoAction) {
        QModelIndex srcIndex = proxyModel->mapToSource(indexes.first());
        ((FileSystemModel *)proxyModel->srcModel())->showShortcutInfo(srcIndex);    // only handle the first one

    } else if (action == refreshAction) {
        refreshTreeView();

    } else if (action == pasteAction) {
        if (indexes.count() == 1)
            mimeDataAction(indexes.first());
        else    // blank space
            mimeDataAction();

    } else if (action == pasteSymlinkAction) {
        if (indexes.count() == 1)
            mimeDataAction(indexes.first(), Qt::LinkAction);
        else    // blank space
            mimeDataAction(QModelIndex(), Qt::LinkAction);

//    } else if (action == undoAction) {

    } else if (action == newFolderAction) {
        //: new system folder
        QString name = tr("New Folder");
        if (indexes.count() == 1)
            createFolder(name, indexes.first());
        else
            createFolder(name);

    } else if (action == newTextAction) {
        //: new system file
        QString name = tr("New File");
        if (indexes.count() == 1)
            createTextFile(name, indexes.first());
        else
            createTextFile(name);

//    } else if (action == propertyAction) {

    }
}


void FileWidget::onVisibilityChanged(bool visible)
{
    qDebug() << QString("onVisibilityChanged %1").arg(visible);

    if (visible) {
        refreshTreeView();
    }
}

void FileWidget::onNavigateBarClicked(const QString &path)
{
    updateCurrentPath(path);
    updateRecord();
}
