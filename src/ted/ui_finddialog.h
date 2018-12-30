/********************************************************************************
** Form generated from reading UI file 'finddialog.ui'
**
** Created by: Qt User Interface Compiler version 5.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FINDDIALOG_H
#define UI_FINDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_FindDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *findText;
    QLabel *label_2;
    QLineEdit *replaceText;
    QLabel *caseSensitiveLabel;
    QCheckBox *caseSensitive;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *replaceAll;
    QPushButton *replace;
    QPushButton *findNext;

    void setupUi(QDialog *FindDialog)
    {
        if (FindDialog->objectName().isEmpty())
            FindDialog->setObjectName(QStringLiteral("FindDialog"));
        FindDialog->resize(761, 148);
        verticalLayout = new QVBoxLayout(FindDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label = new QLabel(FindDialog);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        findText = new QLineEdit(FindDialog);
        findText->setObjectName(QStringLiteral("findText"));

        formLayout->setWidget(0, QFormLayout::FieldRole, findText);

        label_2 = new QLabel(FindDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        replaceText = new QLineEdit(FindDialog);
        replaceText->setObjectName(QStringLiteral("replaceText"));

        formLayout->setWidget(1, QFormLayout::FieldRole, replaceText);

        caseSensitiveLabel = new QLabel(FindDialog);
        caseSensitiveLabel->setObjectName(QStringLiteral("caseSensitiveLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, caseSensitiveLabel);

        caseSensitive = new QCheckBox(FindDialog);
        caseSensitive->setObjectName(QStringLiteral("caseSensitive"));

        formLayout->setWidget(2, QFormLayout::FieldRole, caseSensitive);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        replaceAll = new QPushButton(FindDialog);
        replaceAll->setObjectName(QStringLiteral("replaceAll"));
        replaceAll->setAutoDefault(false);

        horizontalLayout->addWidget(replaceAll);

        replace = new QPushButton(FindDialog);
        replace->setObjectName(QStringLiteral("replace"));
        replace->setAutoDefault(false);

        horizontalLayout->addWidget(replace);

        findNext = new QPushButton(FindDialog);
        findNext->setObjectName(QStringLiteral("findNext"));

        horizontalLayout->addWidget(findNext);


        verticalLayout->addLayout(horizontalLayout);

        QWidget::setTabOrder(findText, replaceText);
        QWidget::setTabOrder(replaceText, caseSensitive);

        retranslateUi(FindDialog);
        QObject::connect(findNext, SIGNAL(clicked()), FindDialog, SLOT(onFindNext()));
        QObject::connect(replace, SIGNAL(clicked()), FindDialog, SLOT(onReplace()));
        QObject::connect(replaceAll, SIGNAL(clicked()), FindDialog, SLOT(onReplaceAll()));

        findNext->setDefault(true);


        QMetaObject::connectSlotsByName(FindDialog);
    } // setupUi

    void retranslateUi(QDialog *FindDialog)
    {
        FindDialog->setWindowTitle(QApplication::translate("FindDialog", "Find/Replace", Q_NULLPTR));
        label->setText(QApplication::translate("FindDialog", "Find", Q_NULLPTR));
        label_2->setText(QApplication::translate("FindDialog", "Replace", Q_NULLPTR));
        caseSensitiveLabel->setText(QApplication::translate("FindDialog", "Case sensitive", Q_NULLPTR));
        replaceAll->setText(QApplication::translate("FindDialog", "Replace All", Q_NULLPTR));
        replace->setText(QApplication::translate("FindDialog", "Replace", Q_NULLPTR));
        findNext->setText(QApplication::translate("FindDialog", "Find Next", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class FindDialog: public Ui_FindDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FINDDIALOG_H
