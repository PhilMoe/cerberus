/********************************************************************************
** Form generated from reading UI file 'prefsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PREFSDIALOG_H
#define UI_PREFSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "colorswatch.h"

QT_BEGIN_NAMESPACE

class Ui_PrefsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *grpMisc;
    QFormLayout *formLayout_5;
    QLabel *themeLabel;
    QComboBox *themeWidget;
    QGroupBox *grpOptions;
    QGridLayout *gridLayout_2;
    QFormLayout *formLayout_3;
    QLabel *familyLabel;
    QFontComboBox *fontComboBox;
    QLabel *pointSizeLabel;
    QSpinBox *fontSizeWidget;
    QLabel *tabSizeLabel_2;
    QSpinBox *tabSizeWidget;
    QLabel *highlightLabel;
    QCheckBox *highlightCaretRowWidget;
    QLabel *smoothFontsLabel;
    QCheckBox *smoothFontsWidget;
    QLabel *showLineNumbersLabel;
    QCheckBox *showLineNumbersWidget;
    QLabel *sortCodeBrowserLabel;
    QCheckBox *sortCodeBrowserWidget;
    QGroupBox *grpColors;
    QGridLayout *gridLayout;
    QFormLayout *formLayout;
    QLabel *backgroundColorLabel;
    ColorSwatch *backgroundColorWidget;
    QLabel *defaultColorLabel;
    ColorSwatch *defaultColorWidget;
    QLabel *highlightColorLabel;
    ColorSwatch *highlightColorWidget;
    QLabel *numbersColorLabel;
    ColorSwatch *numbersColorWidget;
    QLabel *stringsColorLabel;
    ColorSwatch *stringsColorWidget;
    ColorSwatch *identifiersColorWidget;
    QLabel *keywordsColorLabel;
    ColorSwatch *keywordsColorWidget;
    QLabel *commentsColorLabel;
    ColorSwatch *commentsColorWidget;
    QLabel *identifiersColorLabel;
    QGroupBox *grpTools;
    QGridLayout *gridLayout_4;
    QFormLayout *formLayout_4;
    QLabel *cerberusLabel;
    QLabel *blitzMaxLabel;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLineEdit *cerberusPathWidget;
    QToolButton *cerberusPathButton;
    QWidget *widget_2;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *blitzmaxPathWidget;
    QToolButton *blitzmaxPathButton;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *btnLoadColors;
    QPushButton *btnSaveColors;
    QSpacerItem *horizontalSpacer;
    QPushButton *okayButton;

    void setupUi(QDialog *PrefsDialog)
    {
        if (PrefsDialog->objectName().isEmpty())
            PrefsDialog->setObjectName(QStringLiteral("PrefsDialog"));
        PrefsDialog->resize(396, 581);
        PrefsDialog->setStyleSheet(QStringLiteral(""));
        verticalLayout = new QVBoxLayout(PrefsDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        grpMisc = new QGroupBox(PrefsDialog);
        grpMisc->setObjectName(QStringLiteral("grpMisc"));
        formLayout_5 = new QFormLayout(grpMisc);
        formLayout_5->setObjectName(QStringLiteral("formLayout_5"));
        themeLabel = new QLabel(grpMisc);
        themeLabel->setObjectName(QStringLiteral("themeLabel"));

        formLayout_5->setWidget(0, QFormLayout::LabelRole, themeLabel);

        themeWidget = new QComboBox(grpMisc);
        themeWidget->setObjectName(QStringLiteral("themeWidget"));

        formLayout_5->setWidget(0, QFormLayout::FieldRole, themeWidget);


        verticalLayout->addWidget(grpMisc);

        grpOptions = new QGroupBox(PrefsDialog);
        grpOptions->setObjectName(QStringLiteral("grpOptions"));
        gridLayout_2 = new QGridLayout(grpOptions);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        formLayout_3 = new QFormLayout();
        formLayout_3->setObjectName(QStringLiteral("formLayout_3"));
        formLayout_3->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        familyLabel = new QLabel(grpOptions);
        familyLabel->setObjectName(QStringLiteral("familyLabel"));

        formLayout_3->setWidget(0, QFormLayout::LabelRole, familyLabel);

        fontComboBox = new QFontComboBox(grpOptions);
        fontComboBox->setObjectName(QStringLiteral("fontComboBox"));

        formLayout_3->setWidget(0, QFormLayout::FieldRole, fontComboBox);

        pointSizeLabel = new QLabel(grpOptions);
        pointSizeLabel->setObjectName(QStringLiteral("pointSizeLabel"));

        formLayout_3->setWidget(1, QFormLayout::LabelRole, pointSizeLabel);

        fontSizeWidget = new QSpinBox(grpOptions);
        fontSizeWidget->setObjectName(QStringLiteral("fontSizeWidget"));
        fontSizeWidget->setValue(12);

        formLayout_3->setWidget(1, QFormLayout::FieldRole, fontSizeWidget);

        tabSizeLabel_2 = new QLabel(grpOptions);
        tabSizeLabel_2->setObjectName(QStringLiteral("tabSizeLabel_2"));

        formLayout_3->setWidget(2, QFormLayout::LabelRole, tabSizeLabel_2);

        tabSizeWidget = new QSpinBox(grpOptions);
        tabSizeWidget->setObjectName(QStringLiteral("tabSizeWidget"));

        formLayout_3->setWidget(2, QFormLayout::FieldRole, tabSizeWidget);

        highlightLabel = new QLabel(grpOptions);
        highlightLabel->setObjectName(QStringLiteral("highlightLabel"));

        formLayout_3->setWidget(4, QFormLayout::LabelRole, highlightLabel);

        highlightCaretRowWidget = new QCheckBox(grpOptions);
        highlightCaretRowWidget->setObjectName(QStringLiteral("highlightCaretRowWidget"));

        formLayout_3->setWidget(4, QFormLayout::FieldRole, highlightCaretRowWidget);

        smoothFontsLabel = new QLabel(grpOptions);
        smoothFontsLabel->setObjectName(QStringLiteral("smoothFontsLabel"));

        formLayout_3->setWidget(3, QFormLayout::LabelRole, smoothFontsLabel);

        smoothFontsWidget = new QCheckBox(grpOptions);
        smoothFontsWidget->setObjectName(QStringLiteral("smoothFontsWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(smoothFontsWidget->sizePolicy().hasHeightForWidth());
        smoothFontsWidget->setSizePolicy(sizePolicy);
        smoothFontsWidget->setLayoutDirection(Qt::LeftToRight);

        formLayout_3->setWidget(3, QFormLayout::FieldRole, smoothFontsWidget);

        showLineNumbersLabel = new QLabel(grpOptions);
        showLineNumbersLabel->setObjectName(QStringLiteral("showLineNumbersLabel"));

        formLayout_3->setWidget(6, QFormLayout::LabelRole, showLineNumbersLabel);

        showLineNumbersWidget = new QCheckBox(grpOptions);
        showLineNumbersWidget->setObjectName(QStringLiteral("showLineNumbersWidget"));

        formLayout_3->setWidget(6, QFormLayout::FieldRole, showLineNumbersWidget);

        sortCodeBrowserLabel = new QLabel(grpOptions);
        sortCodeBrowserLabel->setObjectName(QStringLiteral("sortCodeBrowserLabel"));

        formLayout_3->setWidget(7, QFormLayout::LabelRole, sortCodeBrowserLabel);

        sortCodeBrowserWidget = new QCheckBox(grpOptions);
        sortCodeBrowserWidget->setObjectName(QStringLiteral("sortCodeBrowserWidget"));

        formLayout_3->setWidget(7, QFormLayout::FieldRole, sortCodeBrowserWidget);


        gridLayout_2->addLayout(formLayout_3, 1, 0, 1, 1);


        verticalLayout->addWidget(grpOptions);

        grpColors = new QGroupBox(PrefsDialog);
        grpColors->setObjectName(QStringLiteral("grpColors"));
        grpColors->setFlat(false);
        gridLayout = new QGridLayout(grpColors);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(9, -1, 9, -1);
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        backgroundColorLabel = new QLabel(grpColors);
        backgroundColorLabel->setObjectName(QStringLiteral("backgroundColorLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, backgroundColorLabel);

        backgroundColorWidget = new ColorSwatch(grpColors);
        backgroundColorWidget->setObjectName(QStringLiteral("backgroundColorWidget"));

        formLayout->setWidget(0, QFormLayout::FieldRole, backgroundColorWidget);

        defaultColorLabel = new QLabel(grpColors);
        defaultColorLabel->setObjectName(QStringLiteral("defaultColorLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, defaultColorLabel);

        defaultColorWidget = new ColorSwatch(grpColors);
        defaultColorWidget->setObjectName(QStringLiteral("defaultColorWidget"));
        defaultColorWidget->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));

        formLayout->setWidget(1, QFormLayout::FieldRole, defaultColorWidget);

        highlightColorLabel = new QLabel(grpColors);
        highlightColorLabel->setObjectName(QStringLiteral("highlightColorLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, highlightColorLabel);

        highlightColorWidget = new ColorSwatch(grpColors);
        highlightColorWidget->setObjectName(QStringLiteral("highlightColorWidget"));

        formLayout->setWidget(2, QFormLayout::FieldRole, highlightColorWidget);

        numbersColorLabel = new QLabel(grpColors);
        numbersColorLabel->setObjectName(QStringLiteral("numbersColorLabel"));

        formLayout->setWidget(3, QFormLayout::LabelRole, numbersColorLabel);

        numbersColorWidget = new ColorSwatch(grpColors);
        numbersColorWidget->setObjectName(QStringLiteral("numbersColorWidget"));

        formLayout->setWidget(3, QFormLayout::FieldRole, numbersColorWidget);

        stringsColorLabel = new QLabel(grpColors);
        stringsColorLabel->setObjectName(QStringLiteral("stringsColorLabel"));

        formLayout->setWidget(4, QFormLayout::LabelRole, stringsColorLabel);

        stringsColorWidget = new ColorSwatch(grpColors);
        stringsColorWidget->setObjectName(QStringLiteral("stringsColorWidget"));

        formLayout->setWidget(4, QFormLayout::FieldRole, stringsColorWidget);

        identifiersColorWidget = new ColorSwatch(grpColors);
        identifiersColorWidget->setObjectName(QStringLiteral("identifiersColorWidget"));

        formLayout->setWidget(5, QFormLayout::FieldRole, identifiersColorWidget);

        keywordsColorLabel = new QLabel(grpColors);
        keywordsColorLabel->setObjectName(QStringLiteral("keywordsColorLabel"));

        formLayout->setWidget(6, QFormLayout::LabelRole, keywordsColorLabel);

        keywordsColorWidget = new ColorSwatch(grpColors);
        keywordsColorWidget->setObjectName(QStringLiteral("keywordsColorWidget"));

        formLayout->setWidget(6, QFormLayout::FieldRole, keywordsColorWidget);

        commentsColorLabel = new QLabel(grpColors);
        commentsColorLabel->setObjectName(QStringLiteral("commentsColorLabel"));

        formLayout->setWidget(7, QFormLayout::LabelRole, commentsColorLabel);

        commentsColorWidget = new ColorSwatch(grpColors);
        commentsColorWidget->setObjectName(QStringLiteral("commentsColorWidget"));

        formLayout->setWidget(7, QFormLayout::FieldRole, commentsColorWidget);

        identifiersColorLabel = new QLabel(grpColors);
        identifiersColorLabel->setObjectName(QStringLiteral("identifiersColorLabel"));

        formLayout->setWidget(5, QFormLayout::LabelRole, identifiersColorLabel);


        gridLayout->addLayout(formLayout, 0, 0, 1, 1);


        verticalLayout->addWidget(grpColors);

        grpTools = new QGroupBox(PrefsDialog);
        grpTools->setObjectName(QStringLiteral("grpTools"));
        gridLayout_4 = new QGridLayout(grpTools);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        formLayout_4 = new QFormLayout();
        formLayout_4->setObjectName(QStringLiteral("formLayout_4"));
        cerberusLabel = new QLabel(grpTools);
        cerberusLabel->setObjectName(QStringLiteral("cerberusLabel"));

        formLayout_4->setWidget(0, QFormLayout::LabelRole, cerberusLabel);

        blitzMaxLabel = new QLabel(grpTools);
        blitzMaxLabel->setObjectName(QStringLiteral("blitzMaxLabel"));

        formLayout_4->setWidget(1, QFormLayout::LabelRole, blitzMaxLabel);

        widget = new QWidget(grpTools);
        widget->setObjectName(QStringLiteral("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        cerberusPathWidget = new QLineEdit(widget);
        cerberusPathWidget->setObjectName(QStringLiteral("cerberusPathWidget"));
        cerberusPathWidget->setLayoutDirection(Qt::LeftToRight);
        cerberusPathWidget->setReadOnly(true);

        horizontalLayout->addWidget(cerberusPathWidget);

        cerberusPathButton = new QToolButton(widget);
        cerberusPathButton->setObjectName(QStringLiteral("cerberusPathButton"));

        horizontalLayout->addWidget(cerberusPathButton);


        formLayout_4->setWidget(0, QFormLayout::FieldRole, widget);

        widget_2 = new QWidget(grpTools);
        widget_2->setObjectName(QStringLiteral("widget_2"));
        horizontalLayout_2 = new QHBoxLayout(widget_2);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        blitzmaxPathWidget = new QLineEdit(widget_2);
        blitzmaxPathWidget->setObjectName(QStringLiteral("blitzmaxPathWidget"));
        blitzmaxPathWidget->setReadOnly(true);

        horizontalLayout_2->addWidget(blitzmaxPathWidget);

        blitzmaxPathButton = new QToolButton(widget_2);
        blitzmaxPathButton->setObjectName(QStringLiteral("blitzmaxPathButton"));

        horizontalLayout_2->addWidget(blitzmaxPathButton);


        formLayout_4->setWidget(1, QFormLayout::FieldRole, widget_2);


        gridLayout_4->addLayout(formLayout_4, 0, 0, 1, 1);


        verticalLayout->addWidget(grpTools);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        btnLoadColors = new QPushButton(PrefsDialog);
        btnLoadColors->setObjectName(QStringLiteral("btnLoadColors"));

        horizontalLayout_3->addWidget(btnLoadColors);

        btnSaveColors = new QPushButton(PrefsDialog);
        btnSaveColors->setObjectName(QStringLiteral("btnSaveColors"));

        horizontalLayout_3->addWidget(btnSaveColors);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        okayButton = new QPushButton(PrefsDialog);
        okayButton->setObjectName(QStringLiteral("okayButton"));
        okayButton->setStyleSheet(QStringLiteral(""));

        horizontalLayout_3->addWidget(okayButton);


        verticalLayout->addLayout(horizontalLayout_3);

        QWidget::setTabOrder(fontComboBox, fontSizeWidget);
        QWidget::setTabOrder(fontSizeWidget, tabSizeWidget);
        QWidget::setTabOrder(tabSizeWidget, cerberusPathButton);
        QWidget::setTabOrder(cerberusPathButton, blitzmaxPathButton);
        QWidget::setTabOrder(blitzmaxPathButton, cerberusPathWidget);
        QWidget::setTabOrder(cerberusPathWidget, blitzmaxPathWidget);

        retranslateUi(PrefsDialog);
        QObject::connect(stringsColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(keywordsColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(defaultColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(numbersColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(backgroundColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(identifiersColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(commentsColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(fontComboBox, SIGNAL(currentFontChanged(QFont)), PrefsDialog, SLOT(onFontChanged(QFont)));
        QObject::connect(fontSizeWidget, SIGNAL(valueChanged(int)), PrefsDialog, SLOT(onFontSizeChanged(int)));
        QObject::connect(cerberusPathButton, SIGNAL(clicked()), PrefsDialog, SLOT(onBrowseForPath()));
        QObject::connect(blitzmaxPathButton, SIGNAL(clicked()), PrefsDialog, SLOT(onBrowseForPath()));
        QObject::connect(tabSizeWidget, SIGNAL(valueChanged(int)), PrefsDialog, SLOT(onTabSizeChanged(int)));
        QObject::connect(highlightColorWidget, SIGNAL(colorChanged()), PrefsDialog, SLOT(onColorChanged()));
        QObject::connect(smoothFontsWidget, SIGNAL(toggled(bool)), PrefsDialog, SLOT(onSmoothFontsChanged(bool)));
        QObject::connect(okayButton, SIGNAL(clicked()), PrefsDialog, SLOT(accept()));
        QObject::connect(highlightCaretRowWidget, SIGNAL(toggled(bool)), PrefsDialog, SLOT(onHighlightCaretRowChanged(bool)));
        QObject::connect(showLineNumbersWidget, SIGNAL(toggled(bool)), PrefsDialog, SLOT(onShowLineNumbersChanged(bool)));
        QObject::connect(themeWidget, SIGNAL(currentTextChanged(QString)), PrefsDialog, SLOT(onThemeChanged(QString)));
        QObject::connect(sortCodeBrowserWidget, SIGNAL(toggled(bool)), PrefsDialog, SLOT(onSortCodeBrowserChanged(bool)));
        QObject::connect(btnSaveColors, SIGNAL(clicked()), PrefsDialog, SLOT(onSaveThemeColor()));
        QObject::connect(btnLoadColors, SIGNAL(clicked()), PrefsDialog, SLOT(onLoadThemeColor()));

        QMetaObject::connectSlotsByName(PrefsDialog);
    } // setupUi

    void retranslateUi(QDialog *PrefsDialog)
    {
        PrefsDialog->setWindowTitle(QApplication::translate("PrefsDialog", "IDE Options", 0));
        grpMisc->setTitle(QApplication::translate("PrefsDialog", "Misc", 0));
        themeLabel->setText(QApplication::translate("PrefsDialog", "Theme", 0));
        grpOptions->setTitle(QApplication::translate("PrefsDialog", "Code Editor Options", 0));
        familyLabel->setText(QApplication::translate("PrefsDialog", "Font Family", 0));
        pointSizeLabel->setText(QApplication::translate("PrefsDialog", "Font Size", 0));
        tabSizeLabel_2->setText(QApplication::translate("PrefsDialog", "Tab size", 0));
        highlightLabel->setText(QApplication::translate("PrefsDialog", "Highlight:", 0));
        highlightCaretRowWidget->setText(QApplication::translate("PrefsDialog", "Caret Row", 0));
        smoothFontsLabel->setText(QApplication::translate("PrefsDialog", "Smooth fonts", 0));
        showLineNumbersLabel->setText(QApplication::translate("PrefsDialog", "Show line numbers", 0));
        showLineNumbersWidget->setText(QString());
        sortCodeBrowserLabel->setText(QApplication::translate("PrefsDialog", "Sort Code Browser", 0));
        sortCodeBrowserWidget->setText(QString());
        grpColors->setTitle(QApplication::translate("PrefsDialog", "Code Editor Colors", 0));
        backgroundColorLabel->setText(QApplication::translate("PrefsDialog", "Background", 0));
        defaultColorLabel->setText(QApplication::translate("PrefsDialog", "Default", 0));
        highlightColorLabel->setText(QApplication::translate("PrefsDialog", "Highlight", 0));
        numbersColorLabel->setText(QApplication::translate("PrefsDialog", "Numbers", 0));
        stringsColorLabel->setText(QApplication::translate("PrefsDialog", "Strings", 0));
        keywordsColorLabel->setText(QApplication::translate("PrefsDialog", "Keywords", 0));
        commentsColorLabel->setText(QApplication::translate("PrefsDialog", "Comments", 0));
        identifiersColorLabel->setText(QApplication::translate("PrefsDialog", "Identifiers", 0));
        grpTools->setTitle(QApplication::translate("PrefsDialog", "Tool Paths", 0));
        cerberusLabel->setText(QApplication::translate("PrefsDialog", "Cerberus", 0));
        blitzMaxLabel->setText(QApplication::translate("PrefsDialog", "BlitzMax", 0));
        cerberusPathButton->setText(QApplication::translate("PrefsDialog", "...", 0));
        blitzmaxPathButton->setText(QApplication::translate("PrefsDialog", "...", 0));
        btnLoadColors->setText(QApplication::translate("PrefsDialog", "Load Colors", 0));
        btnSaveColors->setText(QApplication::translate("PrefsDialog", "Save Colors", 0));
        okayButton->setText(QApplication::translate("PrefsDialog", "Okay", 0));
    } // retranslateUi

};

namespace Ui {
    class PrefsDialog: public Ui_PrefsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PREFSDIALOG_H
