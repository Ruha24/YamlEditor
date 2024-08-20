#include "filesystem.h"

FileSystem::FileSystem(const QString &path)
    : path(path)
{
    if (!checkFolder()) {
        if (!instanceFolder()) {
            QMessageBox::information(nullptr, "Error", "Dir not exists");
            return;
        }
    }

    getAllFiles();
}

bool FileSystem::checkFolder()
{
    return QFile::exists(path);
}

bool FileSystem::instanceFolder()
{
    QDir dir;
    return dir.mkdir(path);
}

QString FileSystem::calculateFileCheckSum(const QString &filePath,
                                          QCryptographicHash::Algorithm algorithm)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл:" << filePath;
        return QString();
    }

    QCryptographicHash hash(algorithm);

    if (hash.addData(&file))
        return hash.result().toHex();

    return QString();
}

QList<QString> FileSystem::getFiles()
{
    return files;
}

void FileSystem::getAllFiles()
{
    files.clear();

    QDir dir(path);
    QFileInfoList dirContent = dir.entryInfoList(QStringList() << "*.yml"
                                                               << "*.yaml",
                                                 QDir::Files);

    for (const auto &file : dirContent) {
        files.append(file.fileName());
    }
}
