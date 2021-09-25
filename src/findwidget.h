#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include <QWidget>
#include <QDir>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

#define HEADER_SIZE_DEFAULT         125
#define HEADER_SIZE_NAME            300
#define HEADER_SIZE_MODIFIED        150

class FindWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FindWidget(QWidget *parent = nullptr, const QString &path = "", const QString &find = "*");

    virtual QSize sizeHint() const;
    void setFindInfo(const QString &path, const QString &find);

    void loadFindWidgetInfo();
    void saveFindWidgetInfo();

public slots:
    void animateFindClick();

private slots:
    void browse();
    void find();
    void onCellActivated(int row, int column);
    void contextMenu(const QPoint &pos);

private:
    QStringList findFiles(const QStringList &files, const QString &text);
    void showFiles(const QStringList &paths);
    QComboBox *createComboBox(const QString &text = QString());
    void createFilesTable();

    QComboBox *fileComboBox;
    QComboBox *textComboBox;
    QComboBox *directoryComboBox;
    QLabel *filesFoundLabel;
    QPushButton *findButton;
    QTableWidget *filesTable;

    QDir currentDir;

signals:
    void cellActivated(const QString &name);
};

#endif // FINDWIDGET_H
