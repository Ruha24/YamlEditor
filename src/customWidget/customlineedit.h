#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QContextMenuEvent>
#include <QLineEdit>
#include <QString>

// Forward declaration instead of including editwidget.h.
// Breaks the circular dependency between the two headers and
// keeps EditWidget's full definition out of this header.
class EditWidget;

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit CustomLineEdit(QWidget *parent = nullptr, QString path = "", bool isKey = false);

    QString GetCurrentPath() const;

    bool GetIsKey() const;

    QString GetKey() const;
    void SetKey(const QString &newKey);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void AddKeyValue(QString path, QString newValue, bool isKey);
    void DeleteElement(QString path, bool isKey);

private:
    void HandleAddKeyValue(QString key, QString value);
    void HandleDeleteElement(QString element);

    QString current_path;
    bool is_key;
    QString key;
};

#endif // CUSTOMLINEEDIT_H