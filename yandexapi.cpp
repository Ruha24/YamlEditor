#include "yandexapi.h"

YandexApi::YandexApi()
{
    QString folderPath = "ymlFiles";
    QDir dir;
    if (!dir.exists(folderPath)) {
        dir.mkpath(folderPath);
    }
}

void YandexApi::downloadFile(const QString &url, const QString &filePath)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(accessToken).toUtf8());

    QFileInfo fileInfo(filePath);

    listFileName.append(fileInfo.fileName());

    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
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
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

void YandexApi::uploadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening file for reading:" << filePath;
        return;
    }

    QFileInfo fileInfo(file.fileName());
    QString filename(fileInfo.fileName());

    QByteArray fileData = file.readAll();
    file.close();

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QUrl url("https://cloud-api.yandex.net/v1/disk/resources/upload");
    QUrlQuery query;
    query.addQueryItem("path", "/YamlFiles/" + filename);
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

        QObject::connect(uploadReply, &QNetworkReply::finished, [uploadReply]() {
            if (uploadReply->error() != QNetworkReply::NoError) {
                qDebug() << "Error uploading file:" << uploadReply->errorString();
            }
            uploadReply->deleteLater();
        });

        reply->deleteLater();
        manager->deleteLater();
    });
}

void YandexApi::getFiles(std::function<void(bool)> callback)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("https://cloud-api.yandex.net/v1/disk/resources?path=/YamlFiles"));
    request.setRawHeader("Authorization", QString("OAuth %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << reply->errorString();
            qDebug() << "Response code:"
                     << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qDebug() << "Response body:" << reply->readAll();
            return;
        }

        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();
        QJsonArray items = jsonObject["_embedded"].toObject()["items"].toArray();

        for (const QJsonValue &value : items) {
            QJsonObject item = value.toObject();

            if (item["type"].toString() == "file") {
                QString fileName = item["name"].toString();
                QString downloadUrl = item["file"].toString();
                QString filePath = QString("ymlFiles/%1").arg(fileName);

                QString suffixFile = QFileInfo(fileName).suffix();

                qDebug() << suffixFile;

                if (suffixFile == ".yaml" || suffixFile == ".yml")
                    downloadFile(downloadUrl, filePath);
            }
        }

        callback(true);

        reply->deleteLater();
        manager->deleteLater();
    });
}

QList<QString> YandexApi::getListFileName() const
{
    return listFileName;
}
