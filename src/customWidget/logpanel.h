#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QDockWidget>

class QListWidget;
class QComboBox;

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

private:
    void append(Level level, const QString &message);
    void applyFilter();

    QListWidget *list_ = nullptr;
    QComboBox *filter_ = nullptr;
};

#endif // LOGPANEL_H
