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
                     const QString &filePath,
                     const QString &accessToken,
                     QObject *parent = nullptr);

    void run() override;

signals:
    void fileDownloaded(const QString &fileName);

private:
    QString url;
    QString filePath;
    QString accessToken;
};

#endif // FILEDOWNLOADTASK_H
