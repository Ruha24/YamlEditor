#ifndef YAMLITEMDELEGATE_H
#define YAMLITEMDELEGATE_H

#include <QStyledItemDelegate>

class YamlItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit YamlItemDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

signals:
    void validationChanged(int invalidCount) const;
};

#endif // YAMLITEMDELEGATE_H
