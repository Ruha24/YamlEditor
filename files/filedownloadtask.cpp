#include "FileDownloadTask.h"
#include <QEventLoop>

FileDownloadTask::FileDownloadTask(const QString &url,
                                   const QString &filePath,
                                   const QString &accessToken,
                                   QObject *parent)
    : QObject(parent)
    , url(url)
    , filePath(filePath)
    , accessToken(accessToken)
{}

void FileDownloadTask::run()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = manager->get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error downloading file:" << reply->errorString();
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();
    } else {
        qDebug() << "Error opening file for writing:" << filePath;
        return;
    }

    QFileInfo fileInfo(filePath);

    emit fileDownloaded(fileInfo.fileName());

    reply->deleteLater();
}
