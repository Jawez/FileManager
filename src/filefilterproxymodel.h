#ifndef FILEFILTERPROXYMODEL_H
#define FILEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QCollator>


class FileFilterProxyModel : public QSortFilterProxyModel
{
//    Q_OBJECT

public:
    FileFilterProxyModel();

    void enableFilter(bool enable);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // QFileSystemModel
    QFileSystemModel *srcModel();
    QFileIconProvider *iconProvider() const;
    QModelIndex proxyIndex(const QString &path, int column = 0) const;
    QFileInfo fileInfo(const QModelIndex &pIndex) const;
    inline QString fileName(const QModelIndex &index) const;

protected:
    // filter
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    // sort
    bool nameCompare(const QModelIndex &source_left, const QModelIndex &source_right) const;
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    bool useFilter;

    QCollator naturalCompare;
    int sortColumn;
};

inline QString FileFilterProxyModel::fileName(const QModelIndex &aindex) const
{ return aindex.data(Qt::DisplayRole).toString(); }

#endif // FILEFILTERPROXYMODEL_H
