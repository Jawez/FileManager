#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>

#include <QFileSystemModel>
#include <QHeaderView>
#include <QTreeView>

#include "filesystemmodel.h"
#include "filefilterproxymodel.h"
#include "treeview.h"


class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    DockWidget(QAbstractItemModel *model);
    ~DockWidget();

    virtual QSize sizeHint() const;

    void refreshTreeView();

private:
    FileSystemModel *fileModel;     // can not delete here
    FileFilterProxyModel *proxyModel;
    TreeView *treeView;

    void fileModelInit();
    void treeViewInit();

private slots:
    void onExpanded(const QModelIndex &index);
    void onTreeViewClicked(const QModelIndex &index);

signals:
    void dockWidgetClicked(const QString path);
};

#endif // DOCKWIDGET_H
