#ifndef FILEDOWNLOADTASK_H
#define FILEDOWNLOADTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>

#include <chrono>

class FileDownloadTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    FileDownloadTask(const QString &url,
                     const QString &file_path,
                     const QString &accessToken,
                     QObject *parent = nullptr);

    void run() override;

signals:
    void FileDownloaded(const QString &fileName);
    void ErrorOccurred(const QString &message);

private:
    QString url;
    QString file_path;
    QString access_token;

    static constexpr std::chrono::milliseconds kTransferTimeout{30000};
};

#endif // FILEDOWNLOADTASK_H
