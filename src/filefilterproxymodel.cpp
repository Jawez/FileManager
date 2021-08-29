#include "filefilterproxymodel.h"

#include <QtDebug>
#include <QDateTime>


FileFilterProxyModel::FileFilterProxyModel()
{
    useFilter = false;
    sortColumn = 0;
}


// filter
void FileFilterProxyModel::enableFilter(bool enable)
{
    useFilter = enable;
}

// only keep dir
bool FileFilterProxyModel::filterAcceptsRow(int sourceRow,
                                              const QModelIndex &sourceParent) const
{
    if (!useFilter)
        return true;

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();
    QFileInfo info = model->fileInfo(index);

    if (info.fileName() == "." || info.fileName() == "..") {
        return false;
    }

    return (info.isDir() && !info.isShortcut());
}


// sort
void FileFilterProxyModel::sort(int column, Qt::SortOrder order)
{
//    qDebug() << QString("sort %1").arg(column);
    sortColumn = column;
    QSortFilterProxyModel::sort(column, order);
}

bool FileFilterProxyModel::nameCompare(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();
    QFileInfo leftInfo = model->fileInfo(source_left);
    QFileInfo rightInfo = model->fileInfo(source_right);

    // place drives before directories
    bool l = model->type(source_left) == "Drive";
    bool r = model->type(source_right) == "Drive";
    if (l ^ r)
        return l;
    if (l && r)     // both Drive
        return naturalCompare.compare(leftInfo.filePath(), rightInfo.filePath()) < 0;

    // place directories before files
    bool left = leftInfo.isDir();
    bool right = rightInfo.isDir();
    if (left ^ right)
        return left;

    return naturalCompare.compare(leftInfo.fileName(), rightInfo.fileName()) < 0;
}

bool FileFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();
    QFileInfo leftInfo = model->fileInfo(source_left);
    QFileInfo rightInfo = model->fileInfo(source_right);

    switch (sortColumn) {
    case 0: {
        return nameCompare(source_left, source_right);
    }
    case 1: {
        qint64 sizeDifference = leftInfo.size() - rightInfo.size();
        if (sizeDifference == 0)    // use nameCompare if the left equal to the right
            return nameCompare(source_left, source_right);

        return sizeDifference < 0;
    }
    case 2: {
        int compare = naturalCompare.compare(model->type(source_left), model->type(source_right));
        if (compare == 0)
            return nameCompare(source_left, source_right);

        return compare < 0;
    }
    case 3: {
        if (leftInfo.lastModified() == rightInfo.lastModified())
            return nameCompare(source_left, source_right);

        return leftInfo.lastModified() < rightInfo.lastModified();
    }
    default:
        return false;
    }
}


// QFileSystemModel
QFileSystemModel *FileFilterProxyModel::srcModel()
{
    return (QFileSystemModel *)sourceModel();
}

QFileIconProvider *FileFilterProxyModel::iconProvider() const
{
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();

    return model->iconProvider();
}

// return proxy model's index
QModelIndex FileFilterProxyModel::proxyIndex(const QString &path, int column) const
{
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();

    return mapFromSource(model->index(path, column));
}

// pIndex is proxy model's index
QFileInfo FileFilterProxyModel::fileInfo(const QModelIndex &pIndex) const
{
    QFileSystemModel *model = (QFileSystemModel *)sourceModel();

    return model->fileInfo(mapToSource(pIndex));
}
