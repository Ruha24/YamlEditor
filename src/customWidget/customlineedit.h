#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>
#include <QString>

class QContextMenuEvent;

class EditWidget;

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit CustomLineEdit(QWidget *parent = nullptr, const QString &path = QString(),
                            bool isKey = false);

    QString GetCurrentPath() const;
    void SetCurrentPath(const QString &path);
    bool GetIsKey() const;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void AddKeyValue(const QString &path, const QString &newValue, bool isKey);
    void DeleteElement(const QString &path, bool isKey);

private:
    void HandleAddKeyValue(const QString &value);
    void HandleDeleteElement();

    QString current_path;
    bool is_key;
};

#endif
