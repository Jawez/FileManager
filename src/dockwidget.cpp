#include "dockwidget.h"

#include <QtDebug>
#include <QSortFilterProxyModel>
#include <QStyleFactory>


DockWidget::DockWidget(QAbstractItemModel *model)
{
    fileModel = (FileSystemModel *)model;

    proxyModel = new FileFilterProxyModel;
    treeView = new TreeView;

    fileModelInit();
    treeViewInit();

    setWidget(treeView);
}

DockWidget::~DockWidget()
{
    treeView->deleteLater();
    proxyModel->deleteLater();
    fileModel = nullptr;
}

QSize DockWidget::sizeHint() const
{
    return QSize(200, -1);
}


void DockWidget::fileModelInit()
{
    proxyModel->setSourceModel(fileModel);
    proxyModel->enableFilter(true);

//    fileModel = new QFileSystemModel;
//    fileModel->setRootPath("");
//    fileModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
//    fileModel->setReadOnly(false);
}

void DockWidget::treeViewInit()
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

    connect(treeView, &QTreeView::expanded, this, &DockWidget::onExpanded);
    connect(treeView, &TreeView::treeViewGotFocus, this, &DockWidget::refreshTreeView);

    connect(treeView, &QTreeView::clicked, this, &DockWidget::onTreeViewClicked);

    // view settings
    treeView->setStyle(QStyleFactory::create("Fusion"));
    treeView->hideColumn(1);    //  treeView->setColumnHidden(1, true);
    treeView->hideColumn(2);
    treeView->hideColumn(3);

    // header settings
    treeView->header()->hide();
}


void DockWidget::onTreeViewClicked(const QModelIndex &index)
{
    QFileInfo info = proxyModel->fileInfo(index);
    if (info.isDir()) {
        emit dockWidgetClicked(info.absoluteFilePath());
    }
}

void DockWidget::onExpanded(const QModelIndex &index)
{
    QString path = proxyModel->fileInfo(index).absoluteFilePath();
//    qDebug() << QString("dock onExpanded %1").arg(path);

    ((FileSystemModel *)proxyModel->srcModel())->refreshDir(path);
}

void DockWidget::refreshTreeView()
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
