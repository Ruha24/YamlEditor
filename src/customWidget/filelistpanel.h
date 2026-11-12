#ifndef FILELISTPANEL_H
#define FILELISTPANEL_H

#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;

// Left side panel: every known YAML file. Open files are bold, files with unsaved
// changes get a dot, the active file is selected.
class FileListPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileListPanel(QWidget *parent = nullptr);

    void addFile(const QString &name, const QString &path);
    void removeFile(const QString &name);
    bool contains(const QString &name) const;

    void setOpen(const QString &name, bool open);
    void setDirty(const QString &name, bool dirty);
    void setCurrent(const QString &name);

signals:
    void fileActivated(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

private:
    QListWidgetItem *find(const QString &name) const;
    void refresh(QListWidgetItem *item);
    void updateEmptyState();
    void retranslate();

    QListWidget *list_;
    QLabel *empty_;
};

#endif // FILELISTPANEL_H
