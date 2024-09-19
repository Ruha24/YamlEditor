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

#include "../../files/filedownloadtask.h"

class YandexApi : public QObject
{
    Q_OBJECT

public:
    YandexApi();

    void UploadFile(const QString &filePath, std::function<void(bool)> callback);

    void GetFiles();

    QList<QString> GetListFileName() const;

signals:
    void NewFile(const QString &fileName);

private:
    const QString access_token = "y0_AgAAAAAm5bEhAAvwaQAAAAEHQYHHAAAQttQpixJD1Yg8UBbxGtZm4sa_hg";
    QString current_file_path;

    QString folder_path = QDir::currentPath() + "/ymlFiles/";
};

#endif // YANDEXAPI_H
