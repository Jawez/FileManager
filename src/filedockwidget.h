#ifndef FILEDOCKWIDGET_H
#define FILEDOCKWIDGET_H

#include <QDockWidget>

#include "filesystemmodel.h"


class FileDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    FileDockWidget(QAbstractItemModel *model);

    void loadDockInfo();
    void saveDockInfo();
};

#endif // FILEDOCKWIDGET_H
