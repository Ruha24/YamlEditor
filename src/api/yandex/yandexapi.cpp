#include "yandexapi.h"

#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QThreadPool>
#include <QUrlQuery>

#include "../../files/filedownloadtask.h"

YandexApi::YandexApi(QObject *parent)
    : QObject(parent)
    , network_manager(std::make_unique<QNetworkAccessManager>(this))
{
    QSettings settings("config.ini", QSettings::IniFormat);
    access_token = settings.value("yandex/token").toString();

    if (access_token.isEmpty())
        qWarning() << "YANDEX_DISK_TOKEN not found";

    connect(network_manager.get(), &QNetworkAccessManager::sslErrors, this,
            [this](QNetworkReply *reply, const QList<QSslError> &errors) {
                QStringList messages;
                for (const QSslError &error : errors)
                    messages << error.errorString();
                emit ErrorOccurred(tr("SSL error: %1").arg(messages.join(", ")));
            });

    QDir dir;
    if (!dir.exists(folder_path))
        dir.mkpath(folder_path);
}

QNetworkReply *YandexApi::sendAuthorizedGet(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QByteArray("OAuth ") + access_token.toUtf8());
    request.setTransferTimeout(kTransferTimeout);
    return network_manager->get(request);
}

void YandexApi::UploadFile(const QString &filePath, std::function<void(bool)> callback)
{
    if (access_token.isEmpty()) {
        emit ErrorOccurred(tr("No Yandex Disk token configured"));
        if (callback)
            callback(false);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit ErrorOccurred(tr("Cannot open file for reading: %1").arg(filePath));
        if (callback)
            callback(false);
        return;
    }

    const QByteArray file_data = file.readAll();
    file.close();

    const QFileInfo file_info(filePath);

    QUrl url("https://cloud-api.yandex.net/v1/disk/resources/upload");
    QUrlQuery query;
    query.addQueryItem("path", "/YamlFiles/" + file_info.fileName());
    query.addQueryItem("overwrite", "true");
    url.setQuery(query);

    QNetworkReply *reply = sendAuthorizedGet(url);
    connect(reply, &QNetworkReply::finished, this, [this, reply, file_data, callback]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit ErrorOccurred(tr("Error getting upload URL: %1").arg(reply->errorString()));
            if (callback)
                callback(false);
            return;
        }

        const QJsonDocument json_response = QJsonDocument::fromJson(reply->readAll());
        if (!json_response.isObject()) {
            emit ErrorOccurred(tr("Invalid response while requesting upload URL"));
            if (callback)
                callback(false);
            return;
        }

        const QString upload_url = json_response.object().value("href").toString();
        if (upload_url.isEmpty()) {
            emit ErrorOccurred(tr("Upload URL missing in response"));
            if (callback)
                callback(false);
            return;
        }

        QNetworkRequest upload_request((QUrl(upload_url)));
        upload_request.setTransferTimeout(kTransferTimeout);
        QNetworkReply *upload_reply = network_manager->put(upload_request, file_data);

        connect(upload_reply, &QNetworkReply::finished, this, [this, upload_reply, callback]() {
            upload_reply->deleteLater();
            const bool ok = upload_reply->error() == QNetworkReply::NoError;
            if (!ok)
                emit ErrorOccurred(tr("Error uploading file: %1").arg(upload_reply->errorString()));
            if (callback)
                callback(ok);
        });
    });
}

void YandexApi::GetFiles()
{
    if (access_token.isEmpty())
        return;

    QNetworkReply *reply =
        sendAuthorizedGet(QUrl("https://cloud-api.yandex.net/v1/disk/resources?path=/YamlFiles"));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit ErrorOccurred(tr("Error listing files: %1").arg(reply->errorString()));
            return;
        }

        const QJsonDocument json_response = QJsonDocument::fromJson(reply->readAll());
        if (!json_response.isObject()) {
            emit ErrorOccurred(tr("Invalid response while listing files"));
            return;
        }

        const QJsonArray items =
            json_response.object().value("_embedded").toObject().value("items").toArray();

        QThreadPool *thread_pool = QThreadPool::globalInstance();

        for (const QJsonValue &value : items) {
            const QJsonObject item = value.toObject();
            if (item.value("type").toString() != "file")
                continue;

            const QString file_name = item.value("name").toString();
            const QString suffix = QFileInfo(file_name).suffix();
            if (suffix != "yaml" && suffix != "yml")
                continue;

            const QString download_url = item.value("file").toString();
            if (download_url.isEmpty())
                continue;

            const QString file_path = folder_path + file_name;

            auto *task = new FileDownloadTask(download_url, file_path, access_token);
            connect(task, &FileDownloadTask::FileDownloaded, this,
                    [this](const QString &fileName) { emit NewFile(fileName); },
                    Qt::QueuedConnection);
            thread_pool->start(task);
        }
    });
}
