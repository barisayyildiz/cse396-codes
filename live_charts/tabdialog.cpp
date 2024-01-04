#include "tabdialog.h"
#include <QVBoxLayout>

TabDialog::TabDialog(QWidget *parent)
    : QDialog(parent)
{
    tabWidget = new QTabWidget;
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     //! [1] //! [3]
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    //setWindowTitle(tr("Tab Dialog"));
}

void TabDialog::addTab(QWidget *widget, const QString name)
{
    tabWidget->addTab(widget, name);
}
