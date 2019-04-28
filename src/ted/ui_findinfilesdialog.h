/********************************************************************************
** Form generated from reading UI file 'findinfilesdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FINDINFILESDIALOG_H
#define UI_FINDINFILESDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FindInFilesDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *findLabel;
    QLineEdit *findLineEdit;
    QLabel *fileTypesLabel;
    QLineEdit *typesLineEdit;
    QLabel *directoryLabel;
    QLabel *caseSensitiveLabel;
    QCheckBox *casedCheckBox;
    QLabel *recursiveLabel;
    QCheckBox *recursiveCheckBox;
    QWidget *widget;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *dirLineEdit;
    QToolButton *dirToolButton;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QListWidget *resultsListWidget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *cancelButton;
    QPushButton *findButton;

    void setupUi(QDialog *FindInFilesDialog)
    {
        if (FindInFilesDialog->objectName().isEmpty())
            FindInFilesDialog->setObjectName(QStringLiteral("FindInFilesDialog"));
        FindInFilesDialog->resize(752, 474);
        verticalLayout = new QVBoxLayout(FindInFilesDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        findLabel = new QLabel(FindInFilesDialog);
        findLabel->setObjectName(QStringLiteral("findLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, findLabel);

        findLineEdit = new QLineEdit(FindInFilesDialog);
        findLineEdit->setObjectName(QStringLiteral("findLineEdit"));

        formLayout->setWidget(0, QFormLayout::FieldRole, findLineEdit);

        fileTypesLabel = new QLabel(FindInFilesDialog);
        fileTypesLabel->setObjectName(QStringLiteral("fileTypesLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, fileTypesLabel);

        typesLineEdit = new QLineEdit(FindInFilesDialog);
        typesLineEdit->setObjectName(QStringLiteral("typesLineEdit"));

        formLayout->setWidget(1, QFormLayout::FieldRole, typesLineEdit);

        directoryLabel = new QLabel(FindInFilesDialog);
        directoryLabel->setObjectName(QStringLiteral("directoryLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, directoryLabel);

        caseSensitiveLabel = new QLabel(FindInFilesDialog);
        caseSensitiveLabel->setObjectName(QStringLiteral("caseSensitiveLabel"));

        formLayout->setWidget(3, QFormLayout::LabelRole, caseSensitiveLabel);

        casedCheckBox = new QCheckBox(FindInFilesDialog);
        casedCheckBox->setObjectName(QStringLiteral("casedCheckBox"));

        formLayout->setWidget(3, QFormLayout::FieldRole, casedCheckBox);

        recursiveLabel = new QLabel(FindInFilesDialog);
        recursiveLabel->setObjectName(QStringLiteral("recursiveLabel"));

        formLayout->setWidget(4, QFormLayout::LabelRole, recursiveLabel);

        recursiveCheckBox = new QCheckBox(FindInFilesDialog);
        recursiveCheckBox->setObjectName(QStringLiteral("recursiveCheckBox"));

        formLayout->setWidget(4, QFormLayout::FieldRole, recursiveCheckBox);

        widget = new QWidget(FindInFilesDialog);
        widget->setObjectName(QStringLiteral("widget"));
        horizontalLayout_2 = new QHBoxLayout(widget);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        dirLineEdit = new QLineEdit(widget);
        dirLineEdit->setObjectName(QStringLiteral("dirLineEdit"));
        dirLineEdit->setReadOnly(true);

        horizontalLayout_2->addWidget(dirLineEdit);

        dirToolButton = new QToolButton(widget);
        dirToolButton->setObjectName(QStringLiteral("dirToolButton"));

        horizontalLayout_2->addWidget(dirToolButton);


        formLayout->setWidget(2, QFormLayout::FieldRole, widget);


        verticalLayout->addLayout(formLayout);

        groupBox = new QGroupBox(FindInFilesDialog);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        resultsListWidget = new QListWidget(groupBox);
        resultsListWidget->setObjectName(QStringLiteral("resultsListWidget"));

        gridLayout->addWidget(resultsListWidget, 0, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        cancelButton = new QPushButton(FindInFilesDialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));
        cancelButton->setEnabled(false);

        horizontalLayout->addWidget(cancelButton);

        findButton = new QPushButton(FindInFilesDialog);
        findButton->setObjectName(QStringLiteral("findButton"));

        horizontalLayout->addWidget(findButton);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(FindInFilesDialog);
        QObject::connect(findButton, SIGNAL(clicked()), FindInFilesDialog, SLOT(find()));
        QObject::connect(dirToolButton, SIGNAL(clicked()), FindInFilesDialog, SLOT(browseForDir()));
        QObject::connect(resultsListWidget, SIGNAL(itemClicked(QListWidgetItem*)), FindInFilesDialog, SLOT(showResult(QListWidgetItem*)));
        QObject::connect(cancelButton, SIGNAL(clicked()), FindInFilesDialog, SLOT(cancel()));

        findButton->setDefault(true);


        QMetaObject::connectSlotsByName(FindInFilesDialog);
    } // setupUi

    void retranslateUi(QDialog *FindInFilesDialog)
    {
        FindInFilesDialog->setWindowTitle(QApplication::translate("FindInFilesDialog", "Find in Files", Q_NULLPTR));
        findLabel->setText(QApplication::translate("FindInFilesDialog", "Find", Q_NULLPTR));
        fileTypesLabel->setText(QApplication::translate("FindInFilesDialog", "File types", Q_NULLPTR));
        directoryLabel->setText(QApplication::translate("FindInFilesDialog", "Directory", Q_NULLPTR));
        caseSensitiveLabel->setText(QApplication::translate("FindInFilesDialog", "Case sensitive", Q_NULLPTR));
        casedCheckBox->setText(QString());
        recursiveLabel->setText(QApplication::translate("FindInFilesDialog", "Recursive", Q_NULLPTR));
        dirToolButton->setText(QApplication::translate("FindInFilesDialog", "...", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("FindInFilesDialog", "Find results", Q_NULLPTR));
        cancelButton->setText(QApplication::translate("FindInFilesDialog", "Stop", Q_NULLPTR));
        findButton->setText(QApplication::translate("FindInFilesDialog", "Find All", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class FindInFilesDialog: public Ui_FindInFilesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FINDINFILESDIALOG_H
