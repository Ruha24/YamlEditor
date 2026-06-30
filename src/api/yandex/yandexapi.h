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
#include <QSettings>

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
    QString access_token;
    QString current_file_path;

    QString folder_path = QDir::currentPath() + "/ymlFiles/";
};

#endif // YANDEXAPI_H
