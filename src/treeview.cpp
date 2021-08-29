#include "treeview.h"

#include <QtDebug>
#include <QFocusEvent>


TreeView::TreeView()
{

}

void TreeView::focusInEvent(QFocusEvent *event)
{
    QAbstractItemView::focusInEvent(event);

    Qt::FocusReason reason = event->reason();
//    qDebug() << QString("focusInEvent %1, reason %2").arg(event->gotFocus()).arg(event->reason());
    switch (reason) {
    case Qt::MouseFocusReason:
    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason:
    case Qt::ActiveWindowFocusReason:
        emit treeViewGotFocus();
        break;
    case Qt::OtherFocusReason:          // QTabWidget switch tab
    case Qt::PopupFocusReason:
    case Qt::ShortcutFocusReason:
    case Qt::MenuBarFocusReason:
    default:
        break;
    }
}

void TreeView::focusOutEvent(QFocusEvent *event)
{
    QAbstractItemView::focusOutEvent(event);

//    qDebug() << QString("focusOutEvent %1, reason %2").arg(event->lostFocus()).arg(event->reason());
}
