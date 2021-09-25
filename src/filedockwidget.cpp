#include "filedockwidget.h"
#include "settings.h"
#include "filewidget.h"

#include <QtDebug>


FileDockWidget::FileDockWidget(QAbstractItemModel *model)
{
    FileWidget *fileWidget = new FileWidget(CONFIG_GROUP_FILEDOCK, model, this);
    connect(this, &QDockWidget::visibilityChanged, fileWidget, &FileWidget::onVisibilityChanged);

    setWidget(fileWidget);

//    loadDockInfo();       // use saveState() and restoreState() instead
}

void FileDockWidget::loadDockInfo()
{
//    qDebug() << QString("loadDockInfo");

    QVariant hidden = readSettings(CONFIG_GROUP_FILEDOCK, CONFIG_DOCK_HIDE);
    if (hidden.isValid()) {
        setHidden(hidden.toBool());
    } else {
        setHidden(true);
    }
}

void FileDockWidget::saveDockInfo()
{
//    qDebug() << QString("saveDockInfo");

    ((FileWidget *)this->widget())->saveFileWidgetInfo();
//    writeSettings(CONFIG_GROUP_FILEDOCK, CONFIG_DOCK_HIDE, isHidden());
}
