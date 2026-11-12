#include "filewatchservice.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>

FileWatchService::FileWatchService(QObject *parent)
    : QObject(parent)
    , watcher_(new QFileSystemWatcher(this))
{
    connect(watcher_, &QFileSystemWatcher::fileChanged,
            this, &FileWatchService::handleFileChanged);
    connect(watcher_, &QFileSystemWatcher::directoryChanged,
            this, &FileWatchService::handleDirectoryChanged);
}

void FileWatchService::watchFolder(const QString &path)
{
    if (!path.isEmpty())
        watcher_->addPath(path);
}

void FileWatchService::watchFile(const QString &path)
{
    if (path.isEmpty())
        return;
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (!watcher_->files().contains(abs) && QFileInfo::exists(abs))
        watcher_->addPath(abs);
}

void FileWatchService::unwatchFile(const QString &path)
{
    if (path.isEmpty())
        return;
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (watcher_->files().contains(abs))
        watcher_->removePath(abs);
    self_saved_.remove(abs);
}

QByteArray FileWatchService::hashOf(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return hash.result();
}

void FileWatchService::markSelfSaved(const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    self_saved_.insert(abs, hashOf(abs));
    watchFile(abs);
}

void FileWatchService::handleFileChanged(const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    const QString name = QFileInfo(abs).fileName();

    if (!QFileInfo::exists(abs)) {
        self_saved_.remove(abs);
        emit fileRemoved(name, abs);
        return;
    }

    // Editors (and QSaveFile) replace the file, which drops it from the watcher.
    watchFile(abs);

    const auto own = self_saved_.constFind(abs);
    if (own != self_saved_.cend() && *own == hashOf(abs))
        return;
    self_saved_.remove(abs);

    emit fileModified(name, abs);
}

void FileWatchService::handleDirectoryChanged(const QString &path)
{
    emit folderChanged(path);
}
