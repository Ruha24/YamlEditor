#ifndef FILEDOWNLOADTASK_H
#define FILEDOWNLOADTASK_H

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QRunnable>

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

private:
    QString url;
    QString file_path;
    QString access_token;
};

#endif // FILEDOWNLOADTASK_H
