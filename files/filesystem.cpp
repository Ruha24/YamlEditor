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

QMap<QString, QString> FileSystem::GetFiles()
{
    return files;
}

QString FileSystem::GetFilePath(QString fileName)
{
    return files.value(fileName, "");
}

void FileSystem::AddFile(QString path)
{
    QFileInfo file_info(path);
    QString file_name = file_info.fileName();
    if (file_info.suffix() == "yml" || file_info.suffix() == "yaml")
        files[file_name] = path;
}

void FileSystem::GetFilesDirectory(const QString &path_dir)
{
    files.clear();

    QDir dir(path_dir);
    QFileInfoList dirContent = dir.entryInfoList(QStringList() << "*.yml"
                                                               << "*.yaml",
                                                 QDir::Files);

    for (const auto &file : dirContent) {
        files.insert(file.fileName(), file.filePath());
    }
}
