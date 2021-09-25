#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QClipboard>

#define USE_CUSTOM_DROP         1
#define DISABLE_FILE_WATCHER    1


class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT    // for signals
public:
    FileSystemModel();
    ~FileSystemModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;

    void refreshDir(const QString &dir);

    QStringList dirEntryList(const QString &path);
    bool moveTarget(const QString &srcPath, const QString &to, const QString &targetPath);
    bool copyTarget(const QString &srcPath, const QString &to, const QString &targetPath);
    bool handleTargetConcurrent(const QString &srcPath, const QString &to, const QString &targetPath,
                                Qt::DropAction action, QWidget *dialog = nullptr);
    bool deleteTarget(const QString &fileName);

    void cleanMimeData();
    void setMimeData(const QModelIndexList &indexes, Qt::DropAction action);
    Qt::DropAction getMimeAct();
    void handleMimeData(const QString &to, Qt::DropAction action = Qt::IgnoreAction);

    bool showMoveConfirmBox(const QMimeData *data, const QString &to);
    bool showReplaceBox(const QString &path);
    void showShortcutInfo(const QModelIndex &index);
    void showShortcutWarning(const QModelIndex &index) const;

    bool removeDirWatcher(const QString &dir);

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    bool handleMimeDataPrivate(const QMimeData *data, Qt::DropAction action, const QString &to, bool isDrop = false);

    QMimeData *modelMimeData;
    Qt::DropAction modelMimeAct;

private slots:
    void onClipboardChanged(QClipboard::Mode mode);

signals:
    void dropCompleted(const QString &dir, const QString &target);
    void pasteCompleted(const QString &dir, const QString &target);
};

#endif // FILESYSTEMMODEL_H
