#include "filewatchservice.h"

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

void FileWatchService::markSelfSaved(const QString &path)
{
    self_saved_.insert(QFileInfo(path).absoluteFilePath());
}

void FileWatchService::handleFileChanged(const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    const QString name = QFileInfo(abs).fileName();

    if (self_saved_.remove(abs)) {
        watchFile(abs);
        return;
    }

    if (!QFileInfo::exists(abs)) {
        emit fileRemoved(name, abs);
        return;
    }

    watchFile(abs);
    emit fileModified(name, abs);
}

void FileWatchService::handleDirectoryChanged(const QString &path)
{
    emit folderChanged(path);
}
