#include "mainwindow.h"

#include <QApplication>
#include <QSettings>

#include "theme/theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("YamlEditor");
    QApplication::setApplicationName("YamlEditor");
    QApplication::setApplicationDisplayName("YAML Editor");
    app.setStyle("Fusion");

    QFont font = QApplication::font();
    font.setPointSizeF(qMax(font.pointSizeF(), 10.0));
    QApplication::setFont(font);

    const QSettings settings;
    Theme::instance().setMode(Theme::modeFromString(settings.value("ui/theme").toString()));

    MainWindow window;
    window.show();

    return app.exec();
}
