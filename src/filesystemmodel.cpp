#include "filesystemmodel.h"
#include "progressdialog.h"
#include "settings.h"

#include <QtDebug>
#include <QFuture>
#include <QFileInfo>
#include <QModelIndex>
#include <QApplication>
#include <QProgressDialog>
#include <QtConcurrent>
#include <QMessageBox>
#include <QPushButton>
#include <QMimeData>
#include <QUrl>

#define MAX_COLUMN_NUMBER 6


FileSystemModel::FileSystemModel()
{
//    connect(qApp->clipboard(), &QClipboard::changed, this, &FileSystemModel::onClipboardChanged);

    // init mime info for cut, copy ...
    modelMimeData = nullptr;
    modelMimeAct = Qt::IgnoreAction;
}

FileSystemModel::~FileSystemModel()
{
    this->cleanMimeData();
}

// handle shortcut info
QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this)
        return QVariant();

    if (role == Qt::SizeHintRole) {
//        qDebug() << QFileSystemModel::data(index, Qt::SizeHintRole).toSize();   // QSize(-1, -1)
        return QSize(-1, ITEM_DEFAULT_HEIGHT);
    }

    if (role == Qt::EditRole || role == Qt::DisplayRole) {
        QFileInfo info = fileInfo(index);
        switch (index.column()) {
        case 0:         // displayName
            if (info.isShortcut()) {
//                qDebug() << QString("displayName %1").arg(info.completeBaseName());
                return QVariant(info.completeBaseName());
            }
            break;
        case 1:         // size
            if (info.isShortcut()) {
                QFileInfo *shortcut = new QFileInfo(info.absoluteFilePath());
//                qDebug() << QString("size %1").arg(shortcut->size());
                return QVariant(shortcut->size());
            }
            break;
        case 2:         // type
            break;
        case 3:         // time
//            if (info.isShortcut()) {
//                QFileInfo *shortcut = new QFileInfo(info.absoluteFilePath());
////                qDebug() << QString("time %1").arg(shortcut->birthTime());
//                return QVariant(shortcut->birthTime());
//            }
            break;
        }
    }

    return QFileSystemModel::data(index, role);
}

// handle translation
QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            // ### TODO oh man this is ugly and doesn't even work all the way!
            // it is still 2 pixels off
            QImage pixmap(16, 1, QImage::Format_ARGB32_Premultiplied);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case 0: returnValue = tr("Name");
        break;
    case 1: returnValue = tr("Size");
        break;
    case 2: returnValue =
#ifdef Q_OS_MAC
                tr("Kind", "Match OS X Finder");
#else
                tr("Type", "All other platforms");
#endif
        break;
    // Windows   - Type
    // OS X      - Kind
    // Konqueror - File Type
    // Nautilus  - Type
    case 3: returnValue = tr("Date Modified");
        break;
    case 4: returnValue = tr("Row");
        break;
    case 5: returnValue = tr("Date Created");
        break;
    case 6: returnValue = tr("Number");
        break;
    default: return QVariant();
    }
    return returnValue;
}

static bool dirIsDotAndDotDot(const QString &dir)
{
    return (dir == "." || dir == "..");
}

// handle drag data
QMimeData *FileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it) {
        if ((*it).column() == 0) {
            // handle special index, for example ".", ".." and Drive
            qDebug() << QString("mime name %1").arg((*it).data().toString());
            // displayName
            QString fileName = (*it).data().toString();
            if (dirIsDotAndDotDot(fileName)) {
                qDebug() << QString(".. continue");
                continue;
            }
            // type
            if ((*it).siblingAtColumn(2).data().toString() == "Drive") {
                qDebug() << QString("Drive continue");
                continue;
            }

//            // QFileSystemModel::filePath() with .lnk for file shortcut, no .lnk for dir shortcut
//            urls << QUrl::fromLocalFile(filePath(*it));

            // QFileInfo::filePath() with .lnk for all shortcuts
            QString path = this->fileInfo(*it).filePath();
            QFileInfo info(path);
            if (info.isShortcut() && !info.exists()) {
                qDebug() << QString("isShortcut not exists continue");
//                showShortcutWarning(*it);     // can not show message box when drag drop
                continue;
            }

            urls << QUrl::fromLocalFile(path);
            qDebug() << QString("mime %1").arg(path);
        }
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

// handle drop data
bool FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (!parent.isValid() || isReadOnly())
        return false;

    // move target by default
    action = Qt::MoveAction;
    qDebug() << QString("modelDropMimeData action %1").arg(action);

#if USE_CUSTOM_DROP
    QString to = QDir::fromNativeSeparators(QString("%1/").arg(filePath(parent)));
    handleMimeDataPrivate(data, action, to, true);
    return true;
#else
    bool success = true;
    QString to = filePath(parent) + QDir::separator();

    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    switch (action) {
    case Qt::CopyAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            qDebug() << QString("drop %1").arg(path);
            success = QFile::copy(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::LinkAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            qDebug() << QString("drop %1").arg(path);
            success = QFile::link(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    case Qt::MoveAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            qDebug() << QString("drop %1").arg(path);
            success = QFile::rename(path, to + QFileInfo(path).fileName()) && success;
        }
        break;
    default:
        return false;
    }

    return success;
#endif  // USE_CUSTOM_DROP
}


void FileSystemModel::refreshDir(const QString &dir)
{
//    qDebug() << QString("refreshDir %1").arg(dir);
    if (dir.isEmpty())
        return;

    if (QFileInfo(dir).isDir() && !QFileInfo(dir).isShortcut()) {
        QString root = this->rootPath();
        this->setRootPath(dir);
        this->setRootPath(root);
    }
}


QStringList FileSystemModel::dirEntryList(const QString &path)
{
    QStringList list;
//    QStringList filter;
//    QDirIterator it(path, filter, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs | QDir::System/* | QDir::Hidden*/, QDirIterator::Subdirectories);

    list << QFileInfo(path).absoluteFilePath();
    while (it.hasNext())
        list << it.next();

//    qDebug() << QString("dirEntryList list %1: ").arg(list.count()) << list;

    return list;
}

static bool isSamePath(const QString &path1, const QString &path2)
{
    return (QDir::fromNativeSeparators(path1) == QDir::fromNativeSeparators(path2));
}

bool FileSystemModel::moveTarget(const QString &srcPath, const QString &to, const QString &targetPath)
{
    // cut
    bool result = QFile::rename(srcPath, targetPath);
    if (!result) {
        qDebug() << QString("rename failed, try to copy");

        // try to copy target, then remove old file
        result = copyTarget(srcPath, to, targetPath);
        if (result) {
            this->remove(this->index(srcPath));         // remove old file
        } else {
            if (this->index(targetPath).isValid())
                this->remove(this->index(targetPath));  // remove target if copy failed
        }
    }

    qDebug() << QString("move target result %1").arg(result);

    return result;
}

// this function will block when copy large file
bool FileSystemModel::copyTarget(const QString &srcPath, const QString &to, const QString &targetPath)
{
    bool result = false;

    QFileInfo info(srcPath);
    if (!info.exists())
        return false;

    // handle target
    if (info.isDir() && !info.isShortcut()) {
        // list all sub target
        QStringList srcList = dirEntryList(info.absoluteFilePath());
        if (srcList.isEmpty()) {
            return false;
        }
        QStringList targetList = srcList;
        targetList.replaceInStrings(info.absoluteFilePath(), targetPath);   // replace src dir to target dir

        // mkdir or copy
        for (int i = 0; i < targetList.count(); i++) {
            QString copyResult;

            QFileInfo srcInfo(srcList.at(i));
            if (srcInfo.isDir() && !srcInfo.isShortcut()) {
                bool result = QDir().mkpath(targetList.at(i));

                copyResult = QString("mkpath %1 %2").arg(result).arg(targetList.at(i));
            } else {
                bool result = QFile::copy(srcList.at(i), targetList.at(i));

                copyResult = QString("copy %1 %2->%3").arg(result).arg(srcList.at(i), targetList.at(i));
                copyResult += QString("; %1 %2").arg(srcInfo.isDir(), !srcInfo.isShortcut());
            }
//                        qDebug() << copyResult;
        }

        result = QFileInfo::exists(targetPath);
    } else {
        // copy
        result = QFile::copy(srcPath, targetPath);
    }

    qDebug() << QString("copy target result %1").arg(result);

    return result;
}

bool FileSystemModel::handleTargetConcurrent(const QString &srcPath, const QString &to, const QString &targetPath,
                                             Qt::DropAction action, QWidget *dialog)
{
    bool result = false;

    ProgressDialog *progress = (ProgressDialog *)dialog;

//    qDebug() << QString("handleTargetConcurrent action %1").arg(action);

    QFuture<bool> future;
    switch (action) {
    case Qt::MoveAction:
        future = QtConcurrent::run(this, &FileSystemModel::moveTarget, srcPath, to, targetPath);
        break;
    case Qt::CopyAction:
        future = QtConcurrent::run(this, &FileSystemModel::copyTarget, srcPath, to, targetPath);
        break;
    default:
        return false;
    }

    while (future.isRunning()) {
        QCoreApplication::processEvents();
        if (progress) {
            if (progress->isCancel()) {
                progress->waitStop(tr("Please wait for a moment..."));
            }
        }
    }
    result = future.result();
//    qDebug() << QString("future result %1").arg(result);

    if (progress) {
        if (progress->wasCanceled()) {
            progress->cancel();
            qDebug() << QString("cancel progress");
        }
    }

    return result;
}

bool FileSystemModel::deleteTarget(const QString &fileName)
{
    bool result = false;
    QFileInfo info(fileName);
    qDebug() << QString("deleteTarget %1, %2").arg(info.exists()).arg(info.absolutePath());
    if (info.exists() || info.isShortcut()) {
        QString path = info.absolutePath();
        result = QFile::moveToTrash(fileName);
        qDebug() << QString("delete %1 result %2").arg(fileName).arg(result);
#if DISABLE_FILE_WATCHER
        this->refreshDir(path);
#endif
    }

    return result;
}

void FileSystemModel::cleanMimeData()
{
    qDebug() << QString("clean mimeData");
    if (modelMimeData != nullptr) {
        delete modelMimeData;
        modelMimeData = nullptr;
    }
    modelMimeAct = Qt::IgnoreAction;
}

void FileSystemModel::setMimeData(const QModelIndexList &indexes, Qt::DropAction action)
{
    if (indexes.isEmpty())
        return;

    if (action != Qt::MoveAction && action != Qt::CopyAction/* && action != Qt::LinkAction*/) {
        qDebug() << QString("set mimeData action %1 error, return").arg(action);
        return;
    }

    modelMimeData = this->mimeData(indexes);
    modelMimeAct = action;

    qApp->clipboard()->setMimeData(this->mimeData(indexes), QClipboard::Clipboard);
}

Qt::DropAction FileSystemModel::getMimeAct()
{
    return modelMimeAct;
}

void FileSystemModel::handleMimeData(const QString &to, Qt::DropAction action)
{
    const QMimeData *data = qApp->clipboard()->mimeData(QClipboard::Clipboard);
    if (data == nullptr || data->urls().isEmpty()){   // data always not null
        qDebug() << "no mimeData";
        return;
    }
    if (modelMimeData == nullptr || data->urls() != modelMimeData->urls()) {
        qDebug() << "mimeData changed: " << data << modelMimeData;
        action = Qt::CopyAction;
    }

    if (action == Qt::IgnoreAction)
        action = modelMimeAct;

    if (action == Qt::IgnoreAction)
        handleMimeDataPrivate(data, Qt::CopyAction, to);
    else
        handleMimeDataPrivate(data, action, to);
}

// private
bool FileSystemModel::handleMimeDataPrivate(const QMimeData *data, Qt::DropAction action, const QString &to, bool isDrop)
{
    qDebug() << QString("handleMimeDataPrivate action %1, to %2").arg(action).arg(to);
    if (action != Qt::MoveAction && action != Qt::CopyAction && action != Qt::LinkAction) {
        return false;
    }

    if (data == nullptr)
        return false;

    if (to.isEmpty())
        return false;

    QList<QUrl> urls = data->urls();
    qDebug() << QString("urls nums %1, to %2").arg(urls.count()).arg(to);
    if (urls.isEmpty())
        return false;

    int currFile = 0;
    int numFiles = urls.count();
    bool showProgress = false;  // !isDrop;
    ProgressDialog *progress = nullptr;

    bool handleResult = false;
    QList<QUrl>::const_iterator it = urls.constBegin();
    for (; it != urls.constEnd(); ++it) {
        QString srcPath = (*it).toLocalFile();
        QString targetPath = srcPath;

        QFileInfo info(srcPath);

        QString labelText = tr("Handling file number %1 of %2...").arg(currFile).arg(numFiles);
        if (showProgress) {
            // update progress dialog
            progress->setProgress(labelText, currFile);
            QCoreApplication::processEvents();
            if (progress->wasCanceled()) {
                qDebug() << QString("wasCanceled");
                break;
            }
        }

        bool result = false;
        switch (action) {
        case Qt::MoveAction: {
            targetPath = to + info.fileName();    // with .lnk for all shortcuts

            if (isSamePath(srcPath, targetPath)) {
                qDebug() << QString("move path not change %1").arg(targetPath);
                continue;
            }

            // not allow move parent folder to child folder
            if (info.isDir() && targetPath.contains(srcPath + "/")) {
                qDebug() << QString("targetPath %1 contain srcPath %2").arg(targetPath, srcPath);
                continue;
            }

            if (!showProgress) {
                if (!showMoveConfirmBox(data, to)) {
                    qDebug() << QString("user cancel move");
                    return false;
                }
//                qDebug() << QString("user confirm continue");

                showProgress = true;

                // init progress dialog
                progress = new ProgressDialog("", tr("Abort"), 0, numFiles, Qt::ApplicationModal);
                progress->setProgress(labelText, currFile);
            }

            // target already exist, not handle shortcut(can not ensure whether shortcut exist)
            if (QFileInfo::exists(targetPath)) {
                if (!showReplaceBox(targetPath))
                    continue;
            }

#if !DISABLE_FILE_WATCHER
            // remove model watcher
            if (this->removeDirWatcher(srcPath))
#endif
            {
//                result = moveTarget(srcPath, to, targetPath);
                result = handleTargetConcurrent(srcPath, to, targetPath, action, progress);
            }
            break;
        }
        case Qt::CopyAction: {
            targetPath = QDir::fromNativeSeparators(to + info.fileName());

            // target already exist, not handle shortcut(can not ensure whether shortcut exist)
            if (QFileInfo::exists(targetPath)) {
                qDebug() << QString("src %1, target %2").arg(srcPath, targetPath);

                // not the same path, alert user
                if (!isSamePath(srcPath, targetPath)) {
                    if (!showReplaceBox(targetPath))
                        continue;
                }
            }
            // rename when targetPath exist
            // if targetPath exist, then srcPath must equal to targetPath now(isSamePath(srcPath, targetPath))
            for (int i = 1; QFileInfo::exists(targetPath); i++) {
                QString name;
                //: system file name
                if (info.completeSuffix().isEmpty())   // dir or special file
                    name = tr("%1 - copy (%2)").arg(info.baseName()).arg(i);
                else
                    name = tr("%1 - copy (%2).%3").arg(info.baseName()).arg(i).arg(info.completeSuffix());
                targetPath = to + name;
            }

            if (!showProgress) {
                showProgress = true;

                progress = new ProgressDialog("", tr("Abort"), 0, numFiles, Qt::ApplicationModal);
                progress->setProgress(labelText, currFile);
            }

//            result = copyTarget(srcPath, to, targetPath);
            result = handleTargetConcurrent(srcPath, to, targetPath, action, progress);
            break;
        }
        case Qt::LinkAction: {
            // get symLinkTarget
            if (info.isShortcut()) {
                srcPath = info.symLinkTarget();
                info.setFile(srcPath);
            }

            targetPath = to + info.fileName() + QString(" - shortcut.lnk");

            // rename
            for (int i = 1; QFileInfo::exists(targetPath); i++) {
                QString name;
                //: system file name
                name = tr("%1 - shortcut (%2).lnk").arg(info.fileName()).arg(i); // example:text.txt - shortcut (1)
                targetPath = to + name;
            }

            if (!showProgress) {
                showProgress = true;

                progress = new ProgressDialog("", tr("Abort"), 0, numFiles, Qt::ApplicationModal);
                progress->setProgress(labelText, currFile);
            }

            // link
            result = QFile::link(srcPath, targetPath);
            break;
        }
        default:
            qDebug() << QString("action error, break");
            break;
        }

        currFile++;

#if DISABLE_FILE_WATCHER
        // refresh dir
        this->refreshDir(to);
        QString from = info.absolutePath();
        if (!isSamePath(to, from))
            this->refreshDir(from);
#endif

        qDebug() << QString("%1 to %2, result %3").arg(srcPath, targetPath).arg(result);
        if (result) {
            if (isDrop)
                emit dropCompleted(to, targetPath);
            else
                emit pasteCompleted(to, targetPath);

            handleResult = true;
        }
    }

    if (showProgress && !progress->wasCanceled() && currFile >= numFiles) {
        QString labelText = tr("Handling file number %1 of %1...").arg(numFiles);
        progress->setProgress(labelText, numFiles);
    }

    if (action == Qt::MoveAction && handleResult) {
        this->cleanMimeData();
        qApp->clipboard()->clear(QClipboard::Clipboard);
    }

    if (progress) {
        progress->deleteLater();
    }

    return handleResult;
}



void FileSystemModel::onClipboardChanged(QClipboard::Mode mode)
{
    qDebug() << QString("onClipboardChanged %1").arg(mode);

//    qDebug() << "data changed: " << qApp->clipboard()->mimeData(QClipboard::Clipboard) << modelMimeData;
//    qDebug() << qApp->clipboard()->mimeData(QClipboard::Clipboard)->formats();
//    qDebug() << qApp->clipboard()->mimeData(QClipboard::Clipboard)->text();
//    qDebug() << qApp->clipboard()->mimeData(QClipboard::Clipboard)->urls();
}

// return false if cancel clicked
bool FileSystemModel::showMoveConfirmBox(const QMimeData *data, const QString &to)
{
    QString allSrcPath;
    QList<QUrl> urls = data->urls();
    foreach (QUrl url, urls) {
        QString path = url.toLocalFile();
        allSrcPath.append(QString("\"%1\"<br>").arg(QDir::toNativeSeparators(path)));
    }
    const QString message =  tr("<p>Move the following <b>%1</b> targets to \"%2\":</p>"
                                "%3")
                                .arg(urls.count()).arg(QDir::toNativeSeparators(to), allSrcPath);

    QMessageBox msgBox;
    //: message box for move targets
    msgBox.setWindowTitle(tr("Move targets"));
    msgBox.setText(message);
//    msgBox.setIcon(QMessageBox::Information);     // setIcon() will cause a beep when it shows

    QPushButton *yesButton = msgBox.addButton(tr("&Yes"), QMessageBox::ActionRole);
    QPushButton *noButton = msgBox.addButton(tr("&No"), QMessageBox::ActionRole);
    msgBox.setDefaultButton(yesButton);
    msgBox.setEscapeButton(noButton);

    int ret = msgBox.exec();
//    qDebug() << QString("msgBox ret %1").arg(ret);
    if (msgBox.clickedButton() == yesButton) {
    } else if (msgBox.clickedButton() == noButton) {
        qDebug() << QString("user canceled the move");
        return false;
    }

    return true;
}

bool FileSystemModel::showReplaceBox(const QString &path)
{
    bool replace = false;

    const QString message =  tr("<p>Target already existed.</p>"
                                "<p>\"%1\"</p>"
                                "<p>Do you want to replace the old one?</p>")
                                .arg(path);

    QMessageBox msgBox;
    //: message box for replace old files for copy
    msgBox.setWindowTitle(tr("Replace or skip"));
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);

    QPushButton *yesButton = msgBox.addButton(tr("&Yes"), QMessageBox::ActionRole);
    QPushButton *noButton = msgBox.addButton(tr("&No"), QMessageBox::ActionRole);
    msgBox.setDefaultButton(yesButton);
    msgBox.setEscapeButton(noButton);

    int ret = msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        this->deleteTarget(path);
        replace = true;
    } else if (msgBox.clickedButton() == noButton) {
    }

    return replace;
}

void FileSystemModel::showShortcutInfo(const QModelIndex &index)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Shortcut info"));

    QString path = QDir::toNativeSeparators(QString("%1/%2").arg(this->fileInfo(index).absolutePath(), this->fileName(index)));
    QString message = QDir::toNativeSeparators(this->fileInfo(index).symLinkTarget());
    if (this->fileInfo(index).exists()) {
//        msgBox.setIcon(QMessageBox::Information);
        message = tr("<p><b>Shortcut:</b></p>%1"
                     "<p>Target exists:</p>%2").arg(path, message);
    } else {
        msgBox.setIcon(QMessageBox::Information);
        message = tr("<p><b>Invalid shortcut:</b></p>%1"
                     "<p>Target not exists:</p>%2").arg(path, message);
    }
    msgBox.setText(message);

    QPushButton *deleteButton = msgBox.addButton(tr("&Delete Shortcut"), QMessageBox::ActionRole);
    QPushButton *okButton = msgBox.addButton(tr("&Ok"), QMessageBox::ActionRole);
//    QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
    msgBox.setDefaultButton(deleteButton);
    msgBox.setEscapeButton(okButton);

    int ret = msgBox.exec();
    if (msgBox.clickedButton() == deleteButton) {
        QString path = this->fileInfo(index).absoluteFilePath();
        this->deleteTarget(path);
    } else if (msgBox.clickedButton() == okButton) {
    }
}

void FileSystemModel::showShortcutWarning(const QModelIndex &index) const
{
    if (this->fileInfo(index).exists() || !this->fileInfo(index).isShortcut()) {
        qDebug() << QString("target exist or is not shortcut, no warning");
        return;
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Invalid shortcut"));

        QString path = QDir::toNativeSeparators(QString("%1/%2").arg(this->fileInfo(index).absolutePath(), this->fileName(index)));
        QString target = QDir::toNativeSeparators(this->fileInfo(index).symLinkTarget());
        msgBox.setIcon(QMessageBox::Information);
        QString message = tr("<p><b>Invalid shortcut:</b></p>%1"
                             "<p>Target not exists:</p>%2").arg(path, target);
        msgBox.setText(message);

        QPushButton *okButton = msgBox.addButton(tr("&Ok"), QMessageBox::ActionRole);
        msgBox.setDefaultButton(okButton);

        msgBox.exec();
    }
}


static void displayNotificationMessage(const QString &title, const QString &text, QMessageBox::Icon icon)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(icon);
    msgBox.addButton(FileSystemModel::tr("&Ok"), QMessageBox::ActionRole);
    msgBox.exec();
}

static void displayFolderInUseMessage(const QString &dir)
{
    const QString message =
        FileSystemModel::tr("<p>Operation failed, some folders or files in this folder have been opened in another program.</p>"
                            "<p>Please close these folders or files and try again.</p>"
                            "%1")
                             .arg(dir);

    displayNotificationMessage(FileSystemModel::tr("Folder is in use"),
                               message,
                               QMessageBox::Information);
}

// use QFileSystemModel::setData() remove watcher, then rename to srcName
bool FileSystemModel::removeDirWatcher(const QString &dir)
{
    if (dir.isEmpty()) {
        return false;
    }

    QFileInfo info(dir);
    if (info.isDir() && !info.isShortcut()) {
        QString name = info.fileName();
        // rename to .../_name
        QString tempName = QString("_%1").arg(name);
        QString tempPath = QString("%1/%2").arg(info.absolutePath(), tempName);

        // prevent tempName already exist, rename to .../i_name
        for (int i = 1; QFileInfo::exists(tempPath); i++) {
            tempName = QString("%1_%2").arg(i).arg(name);
            tempPath = QString("%1/%2").arg(info.absolutePath(), tempName);
        }

        if (QFileSystemModel::setData(this->index(dir), QVariant(tempName))) {
            qDebug() << QString("setData success %1->%2").arg(name, tempName);
            // rename to .../name(restore the srcName)
            if (QFile::rename(tempPath, dir)) {
                qDebug() << QString("rename success %1").arg(dir);
                return true;
            }
        }
    } else {
        return true;    // return true if it is not a dir
    }

    displayFolderInUseMessage(dir);

    return false;
}

static void displayRenameFailedMessage(const QString &newName, const QString &hint)
{
    const QString message = FileSystemModel::tr("<b>The name \"%1\" cannot be used.</b>").arg(newName);

    displayNotificationMessage(FileSystemModel::tr("Rename failed"),
                               QString("%1<br>%2").arg(message).arg(hint),
                               QMessageBox::Information);
}

// rename target
bool FileSystemModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    // data check in QFileSystemModel::setData()
    if (!idx.isValid()
        || idx.column() != 0
        || role != Qt::EditRole
        || (flags(idx) & Qt::ItemIsEditable) == 0) {
        return false;
    }

    QString newName = value.toString();
    QString oldName = idx.data().toString();
    if (newName == oldName)
        return true;

    const QString parentPath = filePath(parent(idx));

    if (newName.isEmpty() || QDir::toNativeSeparators(newName).contains(QDir::separator())) {
        QString hint = tr("<p>Invalid filename.</p>"
                          "<p>Try using another name, with fewer characters or no punctuation marks(can't use %1).</p>")
                         .arg("\\ / : * ? \" &lt; &gt; |");   // \ / : * ? " < > |
        displayRenameFailedMessage(newName, hint);
        return false;
    }

    // make sure target not exist
    QString srcPath = QString("%1/%2").arg(parentPath, oldName);
    QString targetPath = QString("%1/%2").arg(parentPath, newName);
    if (QFileInfo::exists(targetPath)) {
        QString hint = tr("<p>Filename already exists.</p><p>Try using another name.</p>");
        displayRenameFailedMessage(newName, hint);
        return false;
    }

//    qDebug() << QString("FileSystemModel setData");

#if DISABLE_FILE_WATCHER
//    bool result = QFileSystemModel::setData(idx, value, role);
    bool result = QDir(parentPath).rename(oldName, newName);
    qDebug() << QString("setData result %1").arg(result);
    if (result) {
        const QString parentPath = filePath(parent(idx));
        // used to update the view of the proxy model, file model's view auto update
        this->refreshDir(parentPath);
    } else {
        QString hint = tr("<p>Please makesure the target is closed.</p>"
                          "<p>Try using another name, with fewer characters or no punctuation marks(can't use \\/:*?\"<>|).</p>");
        displayRenameFailedMessage(newName, hint);
    }
    return result;
#else
    QString tempName = QString("_%1").arg(newName);
    QString tempPath = QString("%1/%2").arg(parentPath, tempName);
    // prevent tempName already exist, rename to .../i_name
    for (int i = 1; QFileInfo::exists(tempPath); i++) {
        tempName = QString("%1_%2").arg(i).arg(newName);
        tempPath = QString("%1/%2").arg(parentPath, tempName);
    }

    // rename to tempName, then rename to targetName
    if (QFileSystemModel::setData(idx, QVariant(tempName), role)) {
        bool result = QDir(parentPath).rename(tempName, newName);
        qDebug() << QString("rename %1, %2->%3").arg(result).arg(tempName, newName);
        // if rename failed, then restore the oldName
        if (!result) {
            QDir(parentPath).rename(tempName, oldName);
        }
        return result;
    }

    return false;
#endif  // DISABLE_FILE_WATCHER
}
