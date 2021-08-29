#include "progressdialog.h"

#include <QtDebug>
#include <QPushButton>


ProgressDialog::ProgressDialog(const QString &labelText, const QString &cancelButtonText,
                               int minimum, int maximum, Qt::WindowModality windowModality)
{
    waitCancel = false;

    buttonText = cancelButtonText;
    mini = minimum;
    max = maximum;

    curr = mini;

    setLabelText(labelText);
    setCancelButtonText(cancelButtonText);
    setRange(mini, max);
    if (windowModality != Qt::NonModal) {
        setWindowModality(windowModality);
    }
}

void ProgressDialog::setProgressRange(int minimum, int maximum)
{
    mini = minimum;
    max = maximum;

    curr = mini;

    setRange(mini, max);
}

void ProgressDialog::setProgressButtonText(const QString &cancelButtonText)
{
    buttonText = cancelButtonText;
    setCancelButtonText(cancelButtonText);
}

bool ProgressDialog::setProgress(const QString &labelText, int value)
{
    if (waitCancel) {
        return false;
    }

    curr = value;

    setLabelText(labelText);
    setValue(value);

    if (!isVisible()) {
        show();
    }

    return true;
}

void ProgressDialog::waitStop(const QString &labelText)
{
    waitCancel = true;

    setCancelButton(nullptr);   // hide cancel button

    setRange(mini, max);
    setValue(curr);
    setLabelText(labelText);

    show();
}

void ProgressDialog::resetProgress()
{
    waitCancel = false;

    curr = mini;

    setLabelText("");
    setCancelButtonText(buttonText);
    setRange(mini, max);
}


bool ProgressDialog::isCancel() const
{
    return QProgressDialog::wasCanceled() && !waitCancel;
}
