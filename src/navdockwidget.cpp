#include "navdockwidget.h"
#include "settings.h"

#include <QtDebug>
#include <QSortFilterProxyModel>
#include <QStyleFactory>


NavDockWidget::NavDockWidget(QAbstractItemModel *model)
{
    fileModel = (FileSystemModel *)model;

    proxyModel = new FileFilterProxyModel;
    treeView = new TreeView;

    fileModelInit();
    treeViewInit();

    setWidget(treeView);

    loadDockInfo();
}

NavDockWidget::~NavDockWidget()
{
    treeView->deleteLater();
    proxyModel->deleteLater();
    fileModel = nullptr;
}

QSize NavDockWidget::sizeHint() const
{
    return QSize(200, -1);
}


void NavDockWidget::fileModelInit()
{
    proxyModel->setSourceModel(fileModel);
    proxyModel->enableFilter(true);

//    fileModel = new QFileSystemModel;
//    fileModel->setRootPath("");
//    fileModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
//    fileModel->setReadOnly(false);
}

void NavDockWidget::treeViewInit()
{
    treeView->setModel(proxyModel);

    treeView->setSortingEnabled(true);
    treeView->sortByColumn(0, Qt::AscendingOrder);
//    proxyModel->sort(-1, Qt::AscendingOrder);

    // interactive settings
    treeView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    treeView->setDragEnabled(true);
    treeView->setAcceptDrops(true);
    treeView->setDropIndicatorShown(true);
    treeView->setDragDropMode(QAbstractItemView::DragDrop);     // move target on FileSystemModel, not copy

    connect(treeView, &QTreeView::expanded, this, &NavDockWidget::onExpanded);
    connect(treeView, &TreeView::treeViewGotFocus, this, &NavDockWidget::refreshTreeView);

    connect(treeView, &QTreeView::clicked, this, &NavDockWidget::onTreeViewClicked);

    // view settings
//    treeView->setStyle(QStyleFactory::create("Fusion"));
    treeView->hideColumn(1);    //  treeView->setColumnHidden(1, true);
    treeView->hideColumn(2);
    treeView->hideColumn(3);

    // header settings
    treeView->header()->hide();
}


void NavDockWidget::onTreeViewClicked(const QModelIndex &index)
{
    QFileInfo info = proxyModel->fileInfo(index);
    if (info.isDir()) {
        emit navDockClicked(info.absoluteFilePath());
    }
}

void NavDockWidget::onExpanded(const QModelIndex &index)
{
    QString path = proxyModel->fileInfo(index).absoluteFilePath();
//    qDebug() << QString("dock onExpanded %1").arg(path);

    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(path);
}

void NavDockWidget::refreshTreeView()
{
    // reset root path, make the model fetch files or directories
    QString root = fileModel->rootPath();
    fileModel->setRootPath("");

    QModelIndex index = proxyModel->index(0, 0);
    while (index.isValid()) {
//        qDebug() << QString("dock index %1").arg(index.data().toString());
        if (treeView->isExpanded(index)) {
            QFileInfo info = proxyModel->fileInfo(index);
            if (info.fileName() != "." && info.fileName() != "..") {
//                qDebug() << QString("dock expanded %1, %2").arg(treeView->isExpanded(index)).arg(info.absoluteFilePath());
                fileModel->setRootPath(info.absoluteFilePath());
            }
        }
        index = treeView->indexBelow(index);
    }

    fileModel->setRootPath(root);
}

void NavDockWidget::loadDockInfo()
{
//    qDebug() << QString("loadDockInfo");

    QStringList dirList = readArraySettings(CONFIG_GROUP_NAVDOCK);
//    qDebug() << dirList;
    foreach (QString dir, dirList) {
        treeView->expand(proxyModel->proxyIndex(dir));
    }

//    QVariant hidden = readSettings(CONFIG_GROUP_NAVDOCK, CONFIG_DOCK_HIDE);
//    if (hidden.isValid()) {
//        setHidden(hidden.toBool());
//    }
}

void NavDockWidget::saveDockInfo()
{
//    qDebug() << QString("saveDockInfo");

    QStringList dirList;
    QModelIndex index = proxyModel->index(0, 0);
    while (index.isValid()) {
        if (treeView->isExpanded(index)) {
            QFileInfo info = proxyModel->fileInfo(index);
            if (info.fileName() != "." && info.fileName() != "..") {
                dirList.append(info.absoluteFilePath());
            }
        }
        index = treeView->indexBelow(index);
    }

//    qDebug() << dirList;

    writeArraySettings(CONFIG_GROUP_NAVDOCK, dirList);

//    writeSettings(CONFIG_GROUP_NAVDOCK, CONFIG_DOCK_HIDE, isHidden());
}

