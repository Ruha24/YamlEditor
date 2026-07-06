#ifndef FILEWATCHSERVICE_H
#define FILEWATCHSERVICE_H

#include <QObject>
#include <QSet>
#include <QString>

class QFileSystemWatcher;

class FileWatchService : public QObject
{
    Q_OBJECT

public:
    explicit FileWatchService(QObject *parent = nullptr);

    void watchFolder(const QString &path);
    void watchFile(const QString &path);
    void unwatchFile(const QString &path);

    void markSelfSaved(const QString &path);

signals:
    void fileModified(const QString &fileName, const QString &absPath);
    void fileRemoved(const QString &fileName, const QString &absPath);
    void folderChanged(const QString &path);

private slots:
    void handleFileChanged(const QString &path);
    void handleDirectoryChanged(const QString &path);

private:
    QFileSystemWatcher *watcher_;
    QSet<QString> self_saved_;
};

#endif // FILEWATCHSERVICE_H
