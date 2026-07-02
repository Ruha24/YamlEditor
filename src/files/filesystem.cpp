#include "filesystem.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

FileSystem::FileSystem(const QString &path)
{
    if (!CheckFolder(path)) {
        if (!InstanceFolder(path)) {
            QMessageBox::information(nullptr, QObject::tr("Error"),
                                     QObject::tr("Directory does not exist"));
            return;
        }
    }
    GetFilesDirectory(path);
}

bool FileSystem::CheckFolder(const QString &path) const
{
    return QFile::exists(path);
}

bool FileSystem::InstanceFolder(const QString &path) const
{
    QDir dir;
    return dir.mkdir(path);
}

QString FileSystem::CalculateFileCheckSum(const QString &file_path,
                                          QCryptographicHash::Algorithm algorithm) const
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << file_path;
        return QString();
    }

    QCryptographicHash hash(algorithm);
    if (hash.addData(&file))
        return QString::fromLatin1(hash.result().toHex());

    return QString();
}

const QHash<QString, QString> &FileSystem::GetFiles() const
{
    return files;
}

QString FileSystem::GetFilePath(const QString &fileName) const
{
    return files.value(fileName, QString());
}

bool FileSystem::Contains(const QString &fileName) const
{
    return files.contains(fileName);
}

void FileSystem::AddFile(const QString &path)
{
    const QFileInfo file_info(path);
    const QString suffix = file_info.suffix();
    if (suffix == "yml" || suffix == "yaml")
        files.insert(file_info.fileName(), path);
}

void FileSystem::GetFilesDirectory(const QString &path_dir)
{
    files.clear();

    QDir dir(path_dir);
    const QFileInfoList dirContent = dir.entryInfoList(QStringList() << "*.yml"
                                                                     << "*.yaml",
                                                       QDir::Files);

    for (const QFileInfo &file : dirContent)
        files.insert(file.fileName(), file.filePath());
}
