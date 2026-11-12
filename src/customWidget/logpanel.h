#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QDockWidget>

class QListWidget;
class QComboBox;
class QLabel;
class QPushButton;

class LogPanel : public QDockWidget
{
    Q_OBJECT

public:
    enum class Level { Info, Warning, Error };

    explicit LogPanel(QWidget *parent = nullptr);

public slots:
    void logInfo(const QString &message);
    void logWarning(const QString &message);
    void logError(const QString &message);

    void clear();

signals:
    void entryAdded(LogPanel::Level level, const QString &message);

protected:
    void changeEvent(QEvent *event) override;

private:
    void retranslate();

    void append(Level level, const QString &message);
    void applyFilter();

    QListWidget *list_ = nullptr;
    QComboBox *filter_ = nullptr;
    QLabel *filter_label_ = nullptr;
    QPushButton *clear_btn_ = nullptr;
};

#endif // LOGPANEL_H
