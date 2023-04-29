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

#include "std.h"

#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>

// Returns the last part of a file path. e.g. filename
QString stripDir(const QString &path)
{
    int i = path.lastIndexOf('/');
    if(i == -1)
        return path;
    return path.mid(i + 1);
}

// Return the first parts of a file path. e,g the path without the file name.
QString extractDir(const QString &path)
{
    int i = path.lastIndexOf('/');
    if(i == -1)
        return "";
#ifdef Q_OS_WIN32
    if(i && path[i - 1] == ':')
        return "";
#endif
    return path.left(i);
}

// Returns the extension part of a file name.
QString extractExt(const QString &path)
{
    int i = path.lastIndexOf('.') + 1;
    return i && path.indexOf('/', i) == -1 ? path.mid(i) : "";
}

// Converts \ to /, removes trailing /s and prefixes drive if necessary.
QString fixPath(QString path)
{
    if(path.isEmpty())
        return path;

    if(isUrl(path))
        return path;

    path = path.replace('\\', '/');
    path = QDir::cleanPath(path);

#ifdef Q_OS_WIN32
    if(path.startsWith("//"))
        return path;
    if(path.startsWith('/'))
        path = QDir::rootPath() + path.mid(1);
    if(path.endsWith('/') && !path.endsWith(":/"))
        path = path.left(path.length() - 1);
#else
    if(path.endsWith('/') && path != "/")
        path = path.left(path.length() - 1);
#endif

    return path;
}

// Removes a directory or file.
bool removeDir(const QString &path)
{
    bool result = true;
    QDir dir(path);

    if(dir.exists(path)) {
        Q_FOREACH(QFileInfo info,
                  dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files,
                                    QDir::DirsFirst)) {
            if(info.isDir())
                result = removeDir(info.absoluteFilePath());

            else
                result = QFile::remove(info.absoluteFilePath());
            if(!result)
                return result;
        }
        result = dir.rmdir(path);
    }

    return result;
}

// Replace a tab widget witho a new one.
void replaceTabWidgetWidget(QTabWidget *tabWidget, int index, QWidget *widget)
{
    int curr = tabWidget->currentIndex();
    QIcon icon = tabWidget->tabIcon(index);
    QString text = tabWidget->tabText(index);
    tabWidget->removeTab(index);
    tabWidget->insertTab(index, widget, icon, text);
    tabWidget->setCurrentIndex(curr);
    return;
}

// Checks if a path is a URL
bool isUrl(const QString &path)
{
    return path.startsWith("file:") || path.startsWith("http:") || path.startsWith("https:");
}

// TODO: Use a preference dialog for additional file extensions.
bool isImageFile(const QString &path)
{
    static QStringList list;
    if(list.isEmpty()) {
        list << "jpg"
             << "jpeg"
             << "png"
             << "ico"
             << "bmp"
             << "gif"
             << "tif"
             << "tiff"
             << "psd"
             << "xcf"
             << "ico";
        list << "pnm"
             << "pgm"
             << "xpm"
             << "xbm"
             << "tga"
             << "icns"
             << "icon";
    }
    QString ext = extractExt(path.toLower());
    return list.contains(ext);
}

// TODO: Use a preference dialog for additional file extensions.
bool isAudioFile(const QString &path)
{
    static QStringList list;
    if(list.isEmpty()) {
        // mmpz=lmms, flp=Fruity loops/FL Studio, aup=audacity project, cpw/cbw=CakeWalk, cpr=cubase
        list << "wav"
             << "ogg"
             << "m4a"
             << "mp3"
             << "wma";
        list << "mmpz"
             << "flp"
             << "aup"
             << "cpw"
             << "cbw"
             << "cpr";
    }
    QString ext = extractExt(path.toLower());
    return list.contains(ext);
}

// TODO: Use a preference dialog for additional file extensions.
bool isDocFile(const QString &path)
{
    static QStringList list;
    if(list.isEmpty())
        list << "doc"
             << "pdf"
             << "docx"
             << "odt"
             << "rtf";
    QString ext = extractExt(path.toLower());
    return list.contains(ext);
}
