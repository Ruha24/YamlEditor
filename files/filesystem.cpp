#include "filesystem.h"

FileSystem::FileSystem(const QString &path)
{
    if (!CheckFolder(path)) {
        if (!InstanceFolder(path)) {
            QMessageBox::information(nullptr, "Error", "Dir not exists");
            return;
        }
    }

    GetFilesDirectory(path);
}

bool FileSystem::CheckFolder(const QString &path)
{
    return QFile::exists(path);
}

bool FileSystem::InstanceFolder(const QString &path)
{
    QDir dir;
    return dir.mkdir(path);
}

QString FileSystem::CalculateFileCheckSum(const QString &file_path,
                                          QCryptographicHash::Algorithm algorithm)
{
    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть файл:" << file_path;
        return QString();
    }

    QCryptographicHash hash(algorithm);

    if (hash.addData(&file))
        return hash.result().toHex();

    return QString();
}

QList<QString> FileSystem::getFiles()
{
    return list_files_info;
}

void FileSystem::GetFilesDirectory(const QString &path_dir)
{
    list_files_info.clear();

    QDir dir(path_dir);
    QFileInfoList dirContent = dir.entryInfoList(QStringList() << "*.yml"
                                                               << "*.yaml",
                                                 QDir::Files);

    for (const auto &file : dirContent) {
        list_files_info.append(file.fileName());
    }
}
