#include "findwidget.h"
#include "settings.h"

#include <QtWidgets>


enum { absoluteFileNameRole = Qt::UserRole + 1 };

static inline QString fileNameOfItem(const QTableWidgetItem *item)
{
    return item->data(absoluteFileNameRole).toString();
}

FindWidget::FindWidget(QWidget *parent, const QString &path, const QString &find) : QWidget(parent)
{
    setWindowTitle(tr("Find Files"));
    QPushButton *browseButton = new QPushButton(tr("&Browse..."), this);
    connect(browseButton, &QAbstractButton::clicked, this, &FindWidget::browse);
    findButton = new QPushButton(tr("Fin&d"), this);
    connect(findButton, &QAbstractButton::clicked, this, &FindWidget::find);

    fileComboBox = createComboBox(find);
    fileComboBox->setToolTip(tr("Matching a given file name or wildcard. For example, use \"file*\" to match \"file.txt\"."));
    connect(fileComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &FindWidget::animateFindClick);
    textComboBox = createComboBox();
    textComboBox->setToolTip(tr("Search for files containing a specified string (if filled in)."));
    connect(textComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &FindWidget::animateFindClick);
    directoryComboBox = createComboBox(QDir::toNativeSeparators(path));
    connect(directoryComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &FindWidget::animateFindClick);

    filesFoundLabel = new QLabel;

    createFilesTable();

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(new QLabel(tr("Name/Wildcard:")), 0, 0);
    mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
    mainLayout->addWidget(new QLabel(tr("Containing text:")), 1, 0);
    mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
    mainLayout->addWidget(new QLabel(tr("In directory:")), 2, 0);
    mainLayout->addWidget(directoryComboBox, 2, 1);
//    mainLayout->addWidget(directoryComboBox, 2, 1, 1, 2);
    mainLayout->addWidget(browseButton, 2, 2);
    mainLayout->addWidget(filesTable, 3, 0, 1, 3);
    mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 2);
    mainLayout->addWidget(findButton, 4, 2);

    loadFindWidgetInfo();
}

QSize FindWidget::sizeHint() const
{
    return QSize(700, 500);
}

void FindWidget::setFindInfo(const QString &path, const QString &find)
{
//    fileComboBox->addItem(find);
//    directoryComboBox->addItem(path);
    fileComboBox->lineEdit()->setText(find);
    textComboBox->lineEdit()->setText("");
    directoryComboBox->lineEdit()->setText(path);
}

void FindWidget::browse()
{
    QString directory = directoryComboBox->lineEdit()->text();
    if (!QFileInfo::exists(directory)) {
        directory = QDir::currentPath();
    }

    directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find Files"), directory));
    if (!directory.isEmpty()) {
        if (directoryComboBox->findText(directory) == -1)
            directoryComboBox->addItem(directory);
        directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
    }
}

static void updateComboBox(QComboBox *comboBox)
{
    if (comboBox->findText(comboBox->currentText()) == -1)
        comboBox->addItem(comboBox->currentText());
}

void FindWidget::find()
{
    filesTable->setRowCount(0);

    QString fileName = fileComboBox->currentText();
    QString text = textComboBox->currentText();
    QString path = QDir::cleanPath(directoryComboBox->currentText());
    if (path.isEmpty()) {
        qDebug() << "find path empty";
        return;
    }
    currentDir = QDir(path);

    updateComboBox(fileComboBox);
    updateComboBox(textComboBox);
    updateComboBox(directoryComboBox);

    qDebug() << QString("find fileName: %1, text %2, path %3").arg(fileName, text, path);

    QStringList filter;
    if (!fileName.isEmpty())
        filter << fileName;
//    QDirIterator it(path, filter, QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QDirIterator it(path, filter, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList files;
    while (it.hasNext())
        files << it.next();
    if (!text.isEmpty())
        files = findFiles(files, text);
    files.sort();
    showFiles(files);
}

void FindWidget::animateFindClick()
{
    findButton->animateClick();
}

QStringList FindWidget::findFiles(const QStringList &files, const QString &text)
{
    QProgressDialog progressDialog(this);
    progressDialog.setCancelButtonText(tr("&Cancel"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find Files"));

    QMimeDatabase mimeDatabase;
    QStringList foundFiles;

    for (int i = 0; i < files.size(); ++i) {
        progressDialog.setValue(i);
        progressDialog.setLabelText(tr("Searching file number %1 of %n...", nullptr, files.size()).arg(i));
        QCoreApplication::processEvents();

        if (progressDialog.wasCanceled())
            break;

        const QString fileName = files.at(i);
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileName);
        if (mimeType.isValid() && !mimeType.inherits(QStringLiteral("text/plain"))) {
//            qWarning() << "Not searching binary file " << QDir::toNativeSeparators(fileName);
            continue;
        }
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QString line;
            QTextStream in(&file);
            while (!in.atEnd()) {
                if (progressDialog.wasCanceled())
                    break;
                line = in.readLine();
                if (line.contains(text, Qt::CaseInsensitive)) {
                    foundFiles << files[i];
                    break;
                }
            }
        }
    }
    return foundFiles;
}

void FindWidget::showFiles(const QStringList &paths)
{
    for (const QString &filePath : paths) {
        const QString toolTip = QDir::toNativeSeparators(filePath);
        const QString relativePath = QDir::toNativeSeparators(currentDir.relativeFilePath(filePath));
        const qint64 size = QFileInfo(filePath).size();
        QTableWidgetItem *fileNameItem = new QTableWidgetItem(relativePath);
        fileNameItem->setData(absoluteFileNameRole, QVariant(filePath));
        fileNameItem->setToolTip(toolTip);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(QLocale().formattedDataSize(size));
        sizeItem->setData(absoluteFileNameRole, QVariant(filePath));
        sizeItem->setToolTip(toolTip);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        filesTable->setItem(row, 0, fileNameItem);
        filesTable->setItem(row, 1, sizeItem);
    }
    filesFoundLabel->setText(tr("%n file(s) found (Double click on a file to open it)", nullptr, paths.size()));
    filesFoundLabel->setWordWrap(true);
}

QComboBox *FindWidget::createComboBox(const QString &text)
{
    QComboBox *comboBox = new QComboBox;
    comboBox->setEditable(true);
    comboBox->addItem(text);
    comboBox->setInsertPolicy(QComboBox::InsertAtTop);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    return comboBox;
}

void FindWidget::createFilesTable()
{
    filesTable = new QTableWidget(0, 2);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList labels;
    labels << tr("Filename") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->verticalHeader()->setDefaultSectionSize(ITEM_DEFAULT_HEIGHT);
    filesTable->setShowGrid(false);

    filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(filesTable, &QTableWidget::customContextMenuRequested,
            this, &FindWidget::contextMenu);
    connect(filesTable, &QTableWidget::cellActivated,
            this, &FindWidget::onCellActivated);
}

void FindWidget::onCellActivated(int row, int column)
{
    const QTableWidgetItem *item = filesTable->item(row, 0);
    qDebug() << QString("onCellActivated %1").arg(fileNameOfItem(item));
    emit cellActivated(fileNameOfItem(item));
}

void FindWidget::contextMenu(const QPoint &pos)
{
    const QTableWidgetItem *item = filesTable->itemAt(pos);
    if (!item)
        return;
    QMenu menu;
    QAction *openAction = menu.addAction(tr("&Open"));
    QAction *openDirAction = menu.addAction(tr("Open Containing &Folder"));

    QAction *action = menu.exec(filesTable->mapToGlobal(pos));
    if (!action)
        return;

    const QString fileName = fileNameOfItem(item);
    if (action == openAction) {
        emit cellActivated(fileNameOfItem(item));
    } else if (action == openDirAction) {
        QString filename = fileNameOfItem(item);
        emit cellActivated(QFileInfo(filename).absolutePath());
    }
}

void FindWidget::loadFindWidgetInfo()
{
//    qDebug() << QString("loadFindWidgetInfo");

    QStringList findList = readArraySettings(CONFIG_GROUP_FIND);
//        qDebug() << findList;
    if (findList.count() < 3) {
        qDebug() << QString("findList.count() error %1").arg(findList.count());
        return;
    }

    fileComboBox->lineEdit()->setText(findList.at(0));
    textComboBox->lineEdit()->setText(findList.at(1));
    directoryComboBox->lineEdit()->setText(findList.at(2));
}

void FindWidget::saveFindWidgetInfo()
{
//    qDebug() << QString("saveFindWidgetInfo");

    QStringList findList = {fileComboBox->currentText(), textComboBox->currentText(), directoryComboBox->currentText()};
    writeArraySettings(CONFIG_GROUP_FIND, findList);
}
