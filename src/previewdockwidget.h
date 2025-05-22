#ifndef PREVIEWDOCKWIDGET_H
#define PREVIEWDOCKWIDGET_H

#include <QDockWidget>
#include <QLabel>
#include <QPixmap>


class PreviewDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    PreviewDockWidget();

    void loadDockInfo();
    void saveDockInfo();

    void preview(const QString &fileName);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

private:
    QString previewFileName = "";
    QString currentPreviewFileName = "";
    QLabel *label;
    QPixmap *pixmap;

    void previewPixmap(const QSize &size);
};

#endif // PREVIEWDOCKWIDGET_H
