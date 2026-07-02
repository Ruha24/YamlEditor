#include "filedownloadtask.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <memory>
#include <QTimer>

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
    QNetworkAccessManager manager;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", QByteArray("OAuth ") + access_token.toUtf8());
    request.setTransferTimeout(kTransferTimeout);

    std::unique_ptr<QNetworkReply> reply(manager.get(request));

    QEventLoop loop;
    QObject::connect(reply.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit ErrorOccurred(tr("Error downloading file: %1").arg(reply->errorString()));
        return;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit ErrorOccurred(tr("Cannot open file for writing: %1").arg(file_path));
        return;
    }

    if (file.write(reply->readAll()) < 0) {
        emit ErrorOccurred(tr("Failed writing file: %1").arg(file_path));
        return;
    }
    file.close();

    emit FileDownloaded(QFileInfo(file_path).fileName());
}
