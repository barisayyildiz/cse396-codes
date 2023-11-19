#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QDialogButtonBox>

class TabDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TabDialog(QWidget *parent = nullptr);
    void addTab(QWidget *widget, const QString name);

private:
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
};

#endif // TABDIALOG_H
