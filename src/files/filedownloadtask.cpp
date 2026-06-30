#include "FileDownloadTask.h"
#include <QEventLoop>

FileDownloadTask::FileDownloadTask(const QString &url,
                                   const QString &file_path,
                                   const QString &accessToken,
                                   QObject *parent)
    : QObject(parent)
    , url(url)
    , file_path(file_path)
    , access_token(accessToken)
{}

void FileDownloadTask::run()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(access_token).toUtf8());

    QNetworkReply *reply = manager->get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error downloading file:" << reply->errorString();
        return;
    }

    QFile file(file_path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reply->readAll());
        file.close();
    } else {
        qDebug() << "Error opening file for writing:" << file_path;
        return;
    }

    QFileInfo file_info(file_path);

    emit FileDownloaded(file_info.fileName());

    reply->deleteLater();
}
