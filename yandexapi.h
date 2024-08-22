#ifndef YANDEXAPI_H
#define YANDEXAPI_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrlQuery>
#include <QtAlgorithms>

class YandexApi
{
public:
    YandexApi();

    void downloadFile(const QString &url, const QString &filePath);

    void uploadFile(const QString &filePath, std::function<void(bool)> callback);

    void getFiles(std::function<void(bool)> callback);

    QList<QString> getListFileName() const;

private slots:
    void onUploadLinkReceived();
    void onUploadFinished();

private:
    const QString accessToken = "y0_AgAAAAAm5bEhAAvwaQAAAAEHQYHHAAAQttQpixJD1Yg8UBbxGtZm4sa_hg";
    QString currentFilePath;
    QList<QString> listFileName;

    QString folderPath = QDir::currentPath() + "/ymlFiles/";
};

#endif // YANDEXAPI_H
