#ifndef FILEWATCHSERVICE_H
#define FILEWATCHSERVICE_H

#include <QObject>
#include <QHash>
#include <QByteArray>
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

    // Call right after the app itself wrote the file. Change notifications are ignored
    // while the file content still matches what was written (Windows often reports a
    // single save as several change events).
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
    QHash<QString, QByteArray> self_saved_; // abs path -> content hash after our save

    static QByteArray hashOf(const QString &path);
};

#endif // FILEWATCHSERVICE_H
