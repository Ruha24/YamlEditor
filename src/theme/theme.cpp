#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyleHints>
#include <QSvgRenderer>

namespace {

Theme::Colors darkColors()
{
    Theme::Colors c;
    c.window = QColor("#1d1f23");
    c.surface = QColor("#23262b");
    c.surface_alt = QColor("#272a30");
    c.border = QColor("#33373e");
    c.hover = QColor("#2c3036");
    c.selection = QColor("#263d63");
    c.text = QColor("#e4e6ea");
    c.text_muted = QColor("#8d949e");
    c.accent = QColor("#3b82f6");
    c.accent_hover = QColor("#5b97f8");
    c.accent_text = QColor("#ffffff");
    c.danger = QColor("#f47174");
    c.warning = QColor("#e8b44c");
    c.success = QColor("#4cc38a");
    c.yaml_string = QColor("#9fd49a");
    c.yaml_number = QColor("#f2a86f");
    c.yaml_bool = QColor("#c99cf2");
    c.yaml_null = QColor("#7d8590");
    c.yaml_meta = QColor("#6b7380");
    return c;
}

Theme::Colors lightColors()
{
    Theme::Colors c;
    c.window = QColor("#f3f4f6");
    c.surface = QColor("#ffffff");
    c.surface_alt = QColor("#f8f9fb");
    c.border = QColor("#dcdfe4");
    c.hover = QColor("#eceef2");
    c.selection = QColor("#d9e6ff");
    c.text = QColor("#1f2328");
    c.text_muted = QColor("#6a717c");
    c.accent = QColor("#2563eb");
    c.accent_hover = QColor("#1d4fd8");
    c.accent_text = QColor("#ffffff");
    c.danger = QColor("#d43a3a");
    c.warning = QColor("#b26a00");
    c.success = QColor("#1b8a4c");
    c.yaml_string = QColor("#2f7d32");
    c.yaml_number = QColor("#b4530a");
    c.yaml_bool = QColor("#8a3fbf");
    c.yaml_null = QColor("#7a828c");
    c.yaml_meta = QColor("#8c939d");
    return c;
}

QString hex(const QColor &c)
{
    return c.name(QColor::HexRgb);
}

} // namespace

Theme &Theme::instance()
{
    static Theme theme;
    return theme;
}

Theme::Theme()
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        if (mode_ == Mode::System)
            apply();
    });
}

QFont Theme::monoFont(const QFont &base)
{
    QFont font = base;
    font.setFamilies({QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"),
                      QStringLiteral("JetBrains Mono"), QStringLiteral("DejaVu Sans Mono"),
                      QStringLiteral("Menlo"), QStringLiteral("Courier New")});
    font.setStyleHint(QFont::Monospace);
    return font;
}

QString Theme::modeToString(Mode mode)
{
    switch (mode) {
    case Mode::Light:
        return QStringLiteral("light");
    case Mode::Dark:
        return QStringLiteral("dark");
    case Mode::System:
        break;
    }
    return QStringLiteral("system");
}

Theme::Mode Theme::modeFromString(const QString &text)
{
    if (text == QLatin1String("light"))
        return Mode::Light;
    if (text == QLatin1String("dark"))
        return Mode::Dark;
    return Mode::System;
}

void Theme::setMode(Mode mode)
{
    mode_ = mode;

    // Also switches the native window frame (title bar) on Windows.
    switch (mode) {
    case Mode::Light:
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
        break;
    case Mode::Dark:
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        break;
    case Mode::System:
        QGuiApplication::styleHints()->unsetColorScheme();
        break;
    }
    apply();
}

void Theme::apply()
{
    if (mode_ == Mode::System)
        dark_ = QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
    else
        dark_ = mode_ == Mode::Dark;

    colors_ = dark_ ? darkColors() : lightColors();
    icon_cache_.clear();

    QPalette p;
    p.setColor(QPalette::Window, colors_.window);
    p.setColor(QPalette::WindowText, colors_.text);
    p.setColor(QPalette::Base, colors_.surface);
    p.setColor(QPalette::AlternateBase, colors_.surface_alt);
    p.setColor(QPalette::Text, colors_.text);
    p.setColor(QPalette::Button, colors_.surface);
    p.setColor(QPalette::ButtonText, colors_.text);
    p.setColor(QPalette::BrightText, colors_.accent_text);
    p.setColor(QPalette::Highlight, colors_.selection);
    p.setColor(QPalette::HighlightedText, colors_.text);
    p.setColor(QPalette::ToolTipBase, colors_.surface);
    p.setColor(QPalette::ToolTipText, colors_.text);
    p.setColor(QPalette::PlaceholderText, colors_.text_muted);
    p.setColor(QPalette::Link, colors_.accent);
    p.setColor(QPalette::Mid, colors_.border);
    p.setColor(QPalette::Dark, colors_.border);
    p.setColor(QPalette::Light, colors_.surface_alt);
    p.setColor(QPalette::Midlight, colors_.hover);
    p.setColor(QPalette::Shadow, colors_.window);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, role, colors_.text_muted);

    qApp->setPalette(p);
    qApp->setStyleSheet(styleSheet());

    emit changed();
}

QIcon Theme::icon(const QString &name) const
{
    return icon(name, colors_.text);
}

QIcon Theme::icon(const QString &name, const QColor &color) const
{
    const QString key = name + hex(color);
    if (const auto it = icon_cache_.constFind(key); it != icon_cache_.cend())
        return *it;

    QFile file(QStringLiteral(":/icons/%1.svg").arg(name));
    if (!file.open(QIODevice::ReadOnly))
        return QIcon();
    const QByteArray svg = file.readAll();

    auto render = [&svg](const QColor &c, int size, qreal dpr) {
        QByteArray tinted = svg;
        tinted.replace("currentColor", hex(c).toLatin1());
        if (c.alpha() < 255)
            tinted.replace("<svg ", QStringLiteral("<svg opacity=\"%1\" ").arg(c.alphaF()).toLatin1());
        QSvgRenderer renderer(tinted);
        QPixmap pm(QSize(size, size) * dpr);
        pm.fill(Qt::transparent);
        QPainter painter(&pm);
        renderer.render(&painter);
        painter.end();
        pm.setDevicePixelRatio(dpr);
        return pm;
    };

    QColor disabled = colors_.text_muted;
    disabled.setAlphaF(0.55);

    QIcon icon;
    for (int size : {16, 18, 20, 24, 32, 48}) {
        for (qreal dpr : {1.0, 2.0}) {
            icon.addPixmap(render(color, size, dpr), QIcon::Normal);
            icon.addPixmap(render(disabled, size, dpr), QIcon::Disabled);
        }
    }
    icon_cache_.insert(key, icon);
    return icon;
}

QString Theme::styleSheet() const
{
    const Colors &c = colors_;
    QString qss = QStringLiteral(R"(
QMainWindow, QDialog { background: @window; }
QWidget { color: @text; }
QToolTip { background: @surface; color: @text; border: 1px solid @border; padding: 4px 6px; }

QMenuBar { background: @window; padding: 2px 4px; }
QMenuBar::item { padding: 4px 10px; border-radius: 5px; background: transparent; }
QMenuBar::item:selected { background: @hover; }
QMenu { background: @surface; border: 1px solid @border; padding: 4px; }
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 5px; }
QMenu::item:selected { background: @selection; }
QMenu::item:disabled { color: @muted; }
QMenu::separator { height: 1px; background: @border; margin: 4px 6px; }
QMenu::icon { padding-left: 6px; }

QToolBar { background: @window; border: none; border-bottom: 1px solid @border; padding: 4px 8px; spacing: 2px; }
QToolBar::separator { width: 1px; background: @border; margin: 6px 6px; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 5px; }
QToolButton:hover { background: @hover; }
QToolButton:pressed, QToolButton:checked { background: @selection; }
QToolButton:disabled { background: transparent; }
QToolButton[textButton="true"] { padding: 3px 7px; color: @muted; font-weight: 600; }
QToolButton[textButton="true"]:checked { color: @accent; background: @selection; }

QDockWidget::title { background: @window; color: @muted; padding: 8px 12px; text-align: left; }
QMainWindow::separator { background: @border; width: 1px; height: 1px; }
QDockWidget::close-button, QDockWidget::float-button { border: none; background: transparent; padding: 0; }
QDockWidget::close-button:hover, QDockWidget::float-button:hover { background: @hover; border-radius: 3px; }

QListWidget::indicator, QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid @muted;
                                               border-radius: 4px; background: @surface_alt; }
QListWidget::indicator:checked, QCheckBox::indicator:checked { background: @accent; border-color: @accent;
                                                               image: url(:/icons/check-white.svg); }

QTabBar { background: @window; }
QTabBar::tab { background: transparent; color: @muted; border: none; border-right: 1px solid @border;
               border-bottom: 2px solid transparent; padding: 7px 6px 7px 14px; min-width: 60px; }
QTabBar::tab:selected { background: @surface; color: @text; border-bottom: 2px solid @accent; }
QTabBar::tab:hover:!selected { background: @hover; color: @text; }
QTabBar QToolButton { padding: 1px; border-radius: 4px; }

QTreeView, QListView, QListWidget { background: @surface; alternate-background-color: @surface_alt;
                                    border: none; outline: 0; selection-background-color: @selection;
                                    selection-color: @text; }
QTreeView::item, QListWidget::item { min-height: 26px; padding: 0 4px; border: none; }
QTreeView::item:hover, QListWidget::item:hover { background: @hover; }
QTreeView::item:selected, QListWidget::item:selected { background: @selection; }
QHeaderView { background: @surface; }
QHeaderView::section { background: @surface; color: @muted; border: none; border-bottom: 1px solid @border;
                       border-right: 1px solid @border; padding: 6px 10px; font-weight: 600; }

QLineEdit, QComboBox { background: @surface_alt; border: 1px solid @border; border-radius: 6px;
                       padding: 5px 8px; selection-background-color: @accent; selection-color: @accent_text; }
QLineEdit:focus, QComboBox:focus { border-color: @accent; }
QLineEdit[invalid="true"] { border-color: @danger; }
QTreeView QLineEdit { border-radius: 3px; padding: 1px 4px; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView { background: @surface; border: 1px solid @border; selection-background-color: @selection; }

QPushButton { background: @surface; border: 1px solid @border; border-radius: 6px; padding: 6px 14px; }
QPushButton:hover { background: @hover; }
QPushButton:pressed { background: @selection; }
QPushButton[primary="true"] { background: @accent; border-color: @accent; color: @accent_text; font-weight: 600; }
QPushButton[primary="true"]:hover { background: @accent_hover; border-color: @accent_hover; }
QPushButton[flat="true"] { background: transparent; border: none; color: @accent; padding: 2px 4px; }
QPushButton[flat="true"]:hover { text-decoration: underline; }

QLabel[role="muted"] { color: @muted; }
QLabel[role="title"] { font-size: 16pt; font-weight: 600; }
QLabel[role="heading"] { font-size: 11pt; font-weight: 600; }
QLabel[role="warning"] { color: @warning; }
QLabel[role="success"] { color: @success; }

QStatusBar { background: @window; border-top: 1px solid @border; color: @muted; }
QStatusBar::item { border: none; }
QStatusBar QLabel { color: @muted; padding: 0 6px; }
QStatusBar QToolButton { padding: 2px 6px; color: @muted; }

#findBar { background: @window; border-bottom: 1px solid @border; }
#emptyState { background: @surface; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @border; border-radius: 4px; min-height: 28px; margin: 2px; }
QScrollBar::handle:vertical:hover { background: @muted; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: @border; border-radius: 4px; min-width: 28px; margin: 2px; }
QScrollBar::handle:horizontal:hover { background: @muted; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }
)");

    // Longest names first so "@surface_alt" is not clobbered by "@surface".
    const QList<QPair<QString, QColor>> tokens = {
        {"@surface_alt", c.surface_alt}, {"@accent_hover", c.accent_hover},
        {"@accent_text", c.accent_text}, {"@selection", c.selection},
        {"@surface", c.surface}, {"@window", c.window}, {"@border", c.border},
        {"@hover", c.hover}, {"@muted", c.text_muted}, {"@accent", c.accent},
        {"@danger", c.danger}, {"@warning", c.warning}, {"@success", c.success},
        {"@text", c.text},
    };
    for (const auto &[name, color] : tokens)
        qss.replace(name, hex(color));
    return qss;
}
