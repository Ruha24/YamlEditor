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
#include <QThreadPool>
#include <QUrlQuery>
#include <QtAlgorithms>
#include "filedownloadtask.h"

class YandexApi : public QObject
{
    Q_OBJECT

public:
    YandexApi();

    void uploadFile(const QString &filePath, std::function<void(bool)> callback);

    void getFiles();

    QList<QString> getListFileName() const;

signals:
    void newFile(const QString &fileName);

private:
    const QString accessToken = "y0_AgAAAAAm5bEhAAvwaQAAAAEHQYHHAAAQttQpixJD1Yg8UBbxGtZm4sa_hg";
    QString currentFilePath;

    QString folderPath = QDir::currentPath() + "/ymlFiles/";
};

#endif // YANDEXAPI_H
