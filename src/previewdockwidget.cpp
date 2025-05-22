#include "previewdockwidget.h"
#include "settings.h"
#include "filewidget.h"

#include <QGridLayout>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTextStream>
#include <QPixmapCache>
#include <QResizeEvent>

#include <QtDebug>

#define MAX_READ_LEN        1024
#define TEXT_STREAM_CODEC   "UTF-8"

PreviewDockWidget::PreviewDockWidget()
{
    label = new QLabel(this);
    label->setBackgroundRole(QPalette::Base);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
//    label->setScaledContents(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    pixmap = new QPixmap();
    label->setPixmap(*pixmap);

    setWidget(label);
}

void PreviewDockWidget::loadDockInfo()
{
//    qDebug() << QString("loadDockInfo");

    QVariant hidden = readSettings(CONFIG_GROUP_PREVIEWDOCK, CONFIG_DOCK_HIDE);
    if (hidden.isValid()) {
        setHidden(hidden.toBool());
    } else {
        setHidden(true);
    }
}

void PreviewDockWidget::saveDockInfo()
{
//    qDebug() << QString("saveDockInfo");

//    writeSettings(CONFIG_GROUP_PREVIEWDOCK, CONFIG_DOCK_HIDE, isHidden());
}

void PreviewDockWidget::preview(const QString &fileName)
{
    if (isHidden()) {
        previewFileName = fileName;
        return;
    }
    if (currentPreviewFileName == fileName) {
        return;
    }


    qDebug() << QString("preview %1").arg(fileName);

    currentPreviewFileName = "";
    label->clear();
    label->setText(tr("No preview"));

    QMimeDatabase mimeDatabase;
    QMimeType mime = mimeDatabase.mimeTypeForFile(fileName);
    if (mime.name().startsWith("image/")) {
        currentPreviewFileName = fileName;

        QPixmapCache::clear();
        pixmap->load(fileName);
        previewPixmap(size());
    }
    else if (mime.name().startsWith("text/")) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            currentPreviewFileName = fileName;
            QTextStream in(&file);
            in.setCodec(TEXT_STREAM_CODEC);
            label->setText(in.read(MAX_READ_LEN));
            file.close();
        }
    }
}

void PreviewDockWidget::previewPixmap(const QSize &size)
{
    QSize s(size.width() - 2, size.height() - 24);
    if (s.isValid()) {
        label->setPixmap(pixmap->scaled(s, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else {
        label->setPixmap(pixmap->scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    label->adjustSize();
}

void PreviewDockWidget::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);

    if (!label->pixmap().isNull()) {
        previewPixmap(event->size());
    }
}

void PreviewDockWidget::showEvent(QShowEvent *event)
{
    QDockWidget::showEvent(event);

    if (!previewFileName.isEmpty()) {
        preview(previewFileName);
        previewFileName = "";
    }
}
