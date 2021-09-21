#ifndef NAVDOCKWIDGET_H
#define NAVDOCKWIDGET_H

#include <QDockWidget>

#include <QFileSystemModel>
#include <QHeaderView>
#include <QTreeView>

#include "filesystemmodel.h"
#include "filefilterproxymodel.h"
#include "treeview.h"


class NavDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    NavDockWidget(QAbstractItemModel *model);
    ~NavDockWidget();

    virtual QSize sizeHint() const;

    void refreshTreeView();
    void loadDockInfo();
    void saveDockInfo();

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
    void navDockClicked(const QString path);
};

#endif // NAVDOCKWIDGET_H
