#ifndef KEYFILTERPANEL_H
#define KEYFILTERPANEL_H

#include <QSet>
#include <QStringList>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

// Right side panel: one checkbox per leaf key of the document. Unchecked keys are
// hidden in the tree, which makes it easy to focus on e.g. only "price" fields.
class KeyFilterPanel : public QWidget
{
    Q_OBJECT

public:
    explicit KeyFilterPanel(QWidget *parent = nullptr);

    void setKeys(const QStringList &keys, const QSet<QString> &hidden);
    QSet<QString> hiddenKeys() const;

signals:
    void hiddenKeysChanged(const QSet<QString> &hidden);

protected:
    void changeEvent(QEvent *event) override;

private:
    void setAllVisibleChecked(bool checked);
    void applySearch();
    void updateSummary();
    void retranslate();

    QLineEdit *search_;
    QListWidget *list_;
    QLabel *summary_;
    QLabel *empty_;
    QPushButton *all_btn_;
    QPushButton *none_btn_;
    bool updating_ = false;
};

#endif // KEYFILTERPANEL_H
