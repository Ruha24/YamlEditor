#ifndef YANDEXAPI_H
#define YANDEXAPI_H

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

#include <chrono>
#include <functional>

#include <memory>

#include "../../security/tokenstore.h"

class QNetworkReply;

class YandexApi : public QObject
{
    Q_OBJECT

public:
    explicit YandexApi(QObject *parent = nullptr);

    void UploadFile(const QString &filePath, std::function<void(bool)> callback);
    void GetFiles();

    void reloadToken();
    bool hasToken() const { return !access_token.isEmpty(); }

signals:
    void NewFile(const QString &fileName);
    void ErrorOccurred(const QString &message);

private:
    QNetworkReply *sendAuthorizedGet(const QUrl &url);

    std::unique_ptr<QNetworkAccessManager> network_manager;

    TokenStore token_store;
    QString access_token;
    QString folder_path = QDir::currentPath() + "/ymlFiles/";

    static constexpr std::chrono::milliseconds kTransferTimeout{30000};
};

#endif // YANDEXAPI_H
