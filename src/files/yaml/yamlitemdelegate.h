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
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

protected:
    // Colours values by YAML type, uses a monospace font for values and dims
    // structural hints ("{3}", "[0]").
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

signals:
    // Emitted when an edit is thrown away because the text is not acceptable.
    void editRejected(const QString &reason) const;
};

#endif // YAMLITEMDELEGATE_H
