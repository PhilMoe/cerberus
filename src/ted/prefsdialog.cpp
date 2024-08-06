//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
// TODO: Add comments.
#include "prefsdialog.h"
#include "prefs.h"
#include "std.h"
#include "ui_prefsdialog.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

//----------------------------------------------------------------------------------------------------------------------
// PrefsDialog: IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------
PrefsDialog::PrefsDialog(QWidget *parent)
    : QDialog(parent), _ui(new Ui::PrefsDialog), _prefs(Prefs::prefs()), _used(false)
{
    _ui->setupUi(this);

}

PrefsDialog::~PrefsDialog()
{
    delete _ui;
}

//----------------------------------------------------------------------------------------------------------------------
// PrefsDialog: PUBLIC MEMBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void PrefsDialog::readSettings()
{
    QSettings settings;

    if(settings.value("settingsVersion").toInt() < 2)

        return;

    settings.beginGroup("prefsDialog");
    restoreGeometry(settings.value("geometry").toByteArray());
    settings.endGroup();
}

void PrefsDialog::writeSettings()
{
    QSettings settings;
    settings.beginGroup("prefsDialog");

    if(_used)
        settings.setValue("geometry", saveGeometry());

    settings.endGroup();
}

// Start the dialog
int PrefsDialog::exec()
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    QDialog::show();

    if(!_used) {
        restoreGeometry(saveGeometry());
        _used = true;
    }
    _themeSignal = false;

    _ui->fontComboBox->setCurrentFont(QFont(_prefs->getString("fontFamily"), _prefs->getInt("fontSize")));
    _ui->fontSizeWidget->setValue(_prefs->getInt("fontSize"));
    _ui->tabSizeWidget->setValue(_prefs->getInt("tabSize"));
    _ui->smoothFontsWidget->setChecked(_prefs->getBool("smoothFonts"));

    _ui->backgroundColorWidget->setColor(_prefs->getColor("backgroundColor"));
    _ui->lineNumberColorWidget->setColor(_prefs->getColor("lineNumberColor"));
    _ui->console1ColorWidget->setColor(_prefs->getColor("console1Color"));
    _ui->console2ColorWidget->setColor(_prefs->getColor("console2Color"));
    _ui->console3ColorWidget->setColor(_prefs->getColor("console3Color"));
    _ui->console4ColorWidget->setColor(_prefs->getColor("console4Color"));
    _ui->defaultColorWidget->setColor(_prefs->getColor("defaultColor"));
    _ui->numbersColorWidget->setColor(_prefs->getColor("numbersColor"));
    _ui->stringsColorWidget->setColor(_prefs->getColor("stringsColor"));
    _ui->identifiersColorWidget->setColor(_prefs->getColor("identifiersColor"));
    _ui->keywordsColorWidget->setColor(_prefs->getColor("keywordsColor"));
    _ui->keywords2ColorWidget->setColor(_prefs->getColor("keywords2Color"));
    _ui->commentsColorWidget->setColor(_prefs->getColor("commentsColor"));
    _ui->highlightColorWidget->setColor(_prefs->getColor("highlightColor"));
    _ui->tabSizeWidget->setValue(_prefs->getInt("tabSize"));

    _ui->cerberusPathWidget->setText(_prefs->getString("cerberusPath"));
    _ui->blitzmaxPathWidget->setText(_prefs->getString("blitzmaxPath"));
    _ui->highlightCaretRowWidget->setChecked(_prefs->getBool("highlightCurrLine"));
    _ui->highlightCaretWordWidget->setChecked(_prefs->getBool("highlightCurrWord"));
    _ui->highlightBracketsWidget->setChecked(_prefs->getBool("highlightBrackets"));
    _ui->showLineNumbersWidget->setChecked(_prefs->getBool("showLineNumbers"));
    _ui->sortCodeBrowserWidget->setChecked(_prefs->getBool("sortCodeBrowser"));
    _ui->tabs4spacesWidget->setChecked(_prefs->getBool("tabs4spaces"));
    _ui->capitalizeAPIWidget->setChecked(_prefs->getBool("capitalizeAPI"));

    _ui->themeWidget->clear();

    QDir recoredDir(appPath + "/themes/");
    QStringList allFiles = recoredDir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs |
                           QDir::Files, QDir::DirsFirst);
    foreach(const QString &str, allFiles)
        _ui->themeWidget->addItem(str);

    int index = _ui->themeWidget->findText(_prefs->getString("theme"));
    _ui->themeWidget->setCurrentIndex(index);
    _themeSignal = true;

    return QDialog::exec();
}

//----------------------------------------------------------------------------------------------------------------------
// PrefsDialog: PUBLIC MEMBER SLOTS
//----------------------------------------------------------------------------------------------------------------------
// Save the current theme colours
void PrefsDialog::onSaveThemeColor()
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Store theme colors", "Do you want to save/overwrite the theme's editor colors?",
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes) {
        QString theme = "";
        theme = _prefs->getString("theme");

        QSettings themeColor(appPath + "/themes/" + theme + "/" + theme + ".ini", QSettings::IniFormat);
        themeColor.beginGroup("Colors");
        themeColor.setValue("backgroundColor", Prefs::prefs()->getColor("backgroundColor"));
        themeColor.setValue("lineNumberColor", Prefs::prefs()->getColor("lineNumberColor"));
        themeColor.setValue("console1Color", Prefs::prefs()->getColor("console1Color"));
        themeColor.setValue("console2Color", Prefs::prefs()->getColor("console2Color"));
        themeColor.setValue("console3Color", Prefs::prefs()->getColor("console3Color"));
        themeColor.setValue("console4Color", Prefs::prefs()->getColor("console4Color"));
        themeColor.setValue("defaultColor", Prefs::prefs()->getColor("defaultColor"));
        themeColor.setValue("numbersColor", Prefs::prefs()->getColor("numbersColor"));
        themeColor.setValue("stringsColor", Prefs::prefs()->getColor("stringsColor"));
        themeColor.setValue("identifiersColor", Prefs::prefs()->getColor("identifiersColor"));
        themeColor.setValue("keywordsColor", Prefs::prefs()->getColor("keywordsColor"));
        themeColor.setValue("keywords2Color", Prefs::prefs()->getColor("keywords2Color"));
        themeColor.setValue("commentsColor", Prefs::prefs()->getColor("commentsColor"));
        themeColor.setValue("highlightColor", Prefs::prefs()->getColor("highlightColor"));
        themeColor.endGroup();
    }
}

// Load the current theme colours
void PrefsDialog::onLoadThemeColor()
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Load theme colors", "Do you want to load the theme's editor colors?",
                                  QMessageBox::Yes | QMessageBox::No);
    if(reply == QMessageBox::Yes) {

        QString theme = "";
        theme = _prefs->getString("theme");

        QSettings themeColor(appPath + "/themes/" + theme + "/" + theme + ".ini", QSettings::IniFormat);
        _ui->backgroundColorWidget->setColor(themeColor.value("Colors/backgroundColor").toString());
        _ui->lineNumberColorWidget->setColor(themeColor.value("Colors/lineNumberColor").toString());
        _ui->console1ColorWidget->setColor(themeColor.value("Colors/console1Color").toString());
        _ui->console2ColorWidget->setColor(themeColor.value("Colors/console2Color").toString());
        _ui->console3ColorWidget->setColor(themeColor.value("Colors/console3Color").toString());
        _ui->console4ColorWidget->setColor(themeColor.value("Colors/console4Color").toString());
        _ui->defaultColorWidget->setColor(themeColor.value("Colors/defaultColor").toString());
        _ui->numbersColorWidget->setColor(themeColor.value("Colors/numbersColor").toString());
        _ui->stringsColorWidget->setColor(themeColor.value("Colors/stringsColor").toString());
        _ui->identifiersColorWidget->setColor(themeColor.value("Colors/identifiersColor").toString());
        _ui->keywordsColorWidget->setColor(themeColor.value("Colors/keywordsColor").toString());
        _ui->keywords2ColorWidget->setColor(themeColor.value("Colors/keywords2Color").toString());
        _ui->commentsColorWidget->setColor(themeColor.value("Colors/commentsColor").toString());
        _ui->highlightColorWidget->setColor(themeColor.value("Colors/highlightColor").toString());
    }
}

// Change editor settings
void PrefsDialog::onFontChanged(const QFont &font)
{
    _prefs->setValue("fontFamily", font.family());
}

void PrefsDialog::onFontSizeChanged(int size)
{
    _prefs->setValue("fontSize", size);
}

void PrefsDialog::onTabSizeChanged(int size)
{
    _prefs->setValue("tabSize", size);
}

void PrefsDialog::onSmoothFontsChanged(bool state)
{
    _prefs->setValue("smoothFonts", state);
}

void PrefsDialog::onHighlightCaretRowChanged(bool state)
{
    _prefs->setValue("highlightCurrLine", state);
}

void PrefsDialog::onHighlightCaretWordChanged(bool state)
{
    _prefs->setValue("highlightCurrWord", state);
}

void PrefsDialog::onHighlightBracketsChanged(bool state)
{
    _prefs->setValue("highlightBrackets", state);
}

void PrefsDialog::onShowLineNumbersChanged(bool state)
{
    _prefs->setValue("showLineNumbers", state);
}

void PrefsDialog::onTabs4SpacesChanged(bool state)
{
    _prefs->setValue("tabs4spaces", state);
}

void PrefsDialog::onCapitalizeAPIChanged(bool state)
{
    _prefs->setValue("capitalizeAPI", state);
}

void PrefsDialog::onSortCodeBrowserChanged(bool state)
{
    _prefs->setValue("sortCodeBrowser", state);
}

// Colour button change
void PrefsDialog::onColorChanged()
{
    ColorSwatch *swatch = qobject_cast<ColorSwatch *>(sender());
    if(!swatch)
        return;

    QString name = swatch->objectName();
    int i = name.indexOf("Widget");
    if(i == -1)
        return;
    name = name.left(i);

    _prefs->setValue(name.toStdString().c_str(), swatch->color());
}

// Select application directory paths
void PrefsDialog::onBrowseForPath()
{
    if(sender() == _ui->cerberusPathButton) {

        QString path = QFileDialog::getExistingDirectory(this, "Select Cerberus directory", "",
                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(path.isEmpty())
            return;
        path = fixPath(path);

        _prefs->setValue("cerberusPath", path);
        _ui->cerberusPathWidget->setText(path);

    } else if(sender() == _ui->blitzmaxPathButton) {

        QString path = QFileDialog::getExistingDirectory(this, "Select BlitzMax directory", "",
                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(path.isEmpty())
            return;
        path = fixPath(path);

        _prefs->setValue("blitzmaxPath", path);
        _ui->blitzmaxPathWidget->setText(path);
    }
}

// Change the theme
void PrefsDialog::onThemeChanged(QString theme)
{
#ifdef Q_OS_MAC
    QString appPath = extractDir(extractDir(extractDir(QCoreApplication::applicationDirPath())));
#else
    QString appPath = QCoreApplication::applicationDirPath();
#endif

    if(_themeSignal) {
        _prefs->setValue("theme", theme);

        QString css = "";
        QString cssFile = "";
        cssFile = _prefs->getString("theme");
        QFile f(appPath + "/themes/" + cssFile + "/" + cssFile + ".css");
        if(f.open(QFile::ReadOnly)) {
            css = f.readAll();
            css.replace("url(:", "url(" + appPath + "/themes/" + cssFile);
            qApp->setStyleSheet(css);
        }
        f.close();
        onLoadThemeColor();
    }
}
