#ifndef YANDEXAPI_H
#define YANDEXAPI_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QObject>
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

class YandexApi
{
public:
    YandexApi();

    void downloadFile(const QString &url, const QString &filePath);

    void uploadFile(const QString &filePath);

    void getFiles(std::function<void(bool)> callback);

    QList<QString> getListFileName() const;

private:
    const QString accessToken = "y0_AgAAAAAm5bEhAAvwaQAAAAEHQYHHAAAQttQpixJD1Yg8UBbxGtZm4sa_hg";

    QList<QString> listFileName;
};

#endif // YANDEXAPI_H
