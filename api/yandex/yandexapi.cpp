#include "yandexapi.h"

YandexApi::YandexApi()
{
    QDir dir;
    if (!dir.exists(folder_path)) {
        dir.mkpath(folder_path);
    }
}

void YandexApi::UploadFile(const QString &filePath, std::function<void(bool)> callback)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening file for reading:" << filePath;
        return;
    }

    QFileInfo file_info(filePath);

    QByteArray file_data = file.readAll();
    file.close();

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QUrl url("https://cloud-api.yandex.net/v1/disk/resources/upload");
    QUrlQuery query;
    query.addQueryItem("path", "/YamlFiles/" + file_info.fileName());
    query.addQueryItem("overwrite", "true");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("OAuth %1").arg(access_token).toUtf8());

    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error getting upload URL:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument json_response = QJsonDocument::fromJson(response);
        QJsonObject json_object = json_response.object();
        QString upload_url = json_object["href"].toString();

        QNetworkAccessManager *upload_manager = new QNetworkAccessManager();
        QNetworkRequest upload_request((QUrl(upload_url)));
        QNetworkReply *upload_reply = upload_manager->put(upload_request, file_data);

        QObject::connect(upload_reply, &QNetworkReply::finished, [upload_reply, callback]() {
            if (upload_reply->error() != QNetworkReply::NoError) {
                callback(true);
                qDebug() << "Error uploading file:" << upload_reply->errorString();
            } else
                callback(true);
            upload_reply->deleteLater();
        });

        reply->deleteLater();
        manager->deleteLater();
    });
}

void YandexApi::GetFiles()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("https://cloud-api.yandex.net/v1/disk/resources?path=/YamlFiles"));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(access_token).toUtf8());

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << reply->errorString();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument json_response = QJsonDocument::fromJson(response);
        QJsonObject json_object = json_response.object();
        QJsonArray items = json_object["_embedded"].toObject()["items"].toArray();

        QThreadPool *thread_pool = QThreadPool::globalInstance();

        for (const QJsonValue &value : items) {
            QJsonObject item = value.toObject();

            if (item["type"].toString() == "file") {
                QString file_name = item["name"].toString();
                QString suffix_file = QFileInfo(file_name).suffix();

                if (suffix_file == "yaml" || suffix_file == "yml") {
                    QString download_url = item["file"].toString();
                    QString file_path = folder_path + file_name;

                    FileDownloadTask *task = new FileDownloadTask(download_url,
                                                                  file_path,
                                                                  access_token);
                    QObject::connect(task,
                                     &::FileDownloadTask::FileDownloaded,
                                     [=](const QString &fileName) { emit NewFile(fileName); });
                    thread_pool->start(task);
                }
            }
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}
