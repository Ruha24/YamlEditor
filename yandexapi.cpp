#include "yandexapi.h"

YandexApi::YandexApi()
{
    QDir dir;
    if (!dir.exists(folderPath)) {
        dir.mkpath(folderPath);
    }
}

void YandexApi::uploadFile(const QString &filePath, std::function<void(bool)> callback)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening file for reading:" << filePath;
        return;
    }

    QFileInfo fileInfo(filePath);

    QByteArray fileData = file.readAll();
    file.close();

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QUrl url("https://cloud-api.yandex.net/v1/disk/resources/upload");
    QUrlQuery query;
    query.addQueryItem("path", "/YamlFiles/" + fileInfo.fileName());
    query.addQueryItem("overwrite", "true");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("OAuth %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error getting upload URL:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();
        QString uploadUrl = jsonObject["href"].toString();

        QNetworkAccessManager *uploadManager = new QNetworkAccessManager();
        QNetworkRequest uploadRequest((QUrl(uploadUrl)));
        QNetworkReply *uploadReply = uploadManager->put(uploadRequest, fileData);

        QObject::connect(uploadReply, &QNetworkReply::finished, [uploadReply, callback]() {
            if (uploadReply->error() != QNetworkReply::NoError) {
                callback(true);
                qDebug() << "Error uploading file:" << uploadReply->errorString();
            } else
                callback(true);
            uploadReply->deleteLater();
        });

        reply->deleteLater();
        manager->deleteLater();
    });
}

void YandexApi::getFiles()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("https://cloud-api.yandex.net/v1/disk/resources?path=/YamlFiles"));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << reply->errorString();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();
        QJsonArray items = jsonObject["_embedded"].toObject()["items"].toArray();

        QThreadPool *threadPool = QThreadPool::globalInstance();

        for (const QJsonValue &value : items) {
            QJsonObject item = value.toObject();

            if (item["type"].toString() == "file") {
                QString fileName = item["name"].toString();
                QString suffixFile = QFileInfo(fileName).suffix();

                if (suffixFile == "yaml" || suffixFile == "yml") {
                    QString downloadUrl = item["file"].toString();
                    QString filePath = folderPath + fileName;

                    FileDownloadTask *task = new FileDownloadTask(downloadUrl,
                                                                  filePath,
                                                                  accessToken);
                    QObject::connect(task,
                                     &FileDownloadTask::fileDownloaded,
                                     [=](const QString &fileName) { emit newFile(fileName); });
                    threadPool->start(task);
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}
