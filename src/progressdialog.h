#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QProgressDialog>

class ProgressDialog : public QProgressDialog
{
public:
    ProgressDialog(const QString &labelText, const QString &cancelButtonText,
                   int minimum, int maximum, Qt::WindowModality windowModality = Qt::NonModal);

    void setProgressRange(int minimum, int maximum);
    void setProgressButtonText(const QString &cancelButtonText);
    bool setProgress(const QString &labelText, int value);
    void waitStop(const QString &labelText);
    void resetProgress();

    bool isCancel() const;

private:
    bool waitCancel;

    int curr;
    int mini;
    int max;
    QString buttonText;
};

#endif // PROGRESSDIALOG_H
