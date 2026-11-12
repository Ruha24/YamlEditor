#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QHash>
#include <QIcon>
#include <QObject>

// Application-wide look: one set of colour tokens per scheme, turned into a
// QPalette + stylesheet. Widgets never hard-code colours; they read colors()
// and re-read them when changed() is emitted.
class Theme : public QObject
{
    Q_OBJECT

public:
    enum class Mode { System, Light, Dark };

    struct Colors
    {
        QColor window;      // app background, toolbars, docks
        QColor surface;     // editors, lists, tree
        QColor surface_alt; // alternate rows, inputs
        QColor border;
        QColor hover;
        QColor selection;   // selected rows
        QColor text;
        QColor text_muted;
        QColor accent;
        QColor accent_hover;
        QColor accent_text;
        QColor danger;
        QColor warning;
        QColor success;

        // YAML value kinds in the tree
        QColor yaml_string;
        QColor yaml_number;
        QColor yaml_bool;
        QColor yaml_null;
        QColor yaml_meta;   // "{3}" / "[2]" counters, list indexes
    };

    static Theme &instance();

    void setMode(Mode mode);
    Mode mode() const { return mode_; }
    bool isDark() const { return dark_; }
    const Colors &colors() const { return colors_; }

    // Icon from :/icons/<name>.svg tinted with the current text colour.
    QIcon icon(const QString &name) const;
    QIcon icon(const QString &name, const QColor &color) const;

    // Monospace font for YAML values and the log.
    static QFont monoFont(const QFont &base);

    static QString modeToString(Mode mode);
    static Mode modeFromString(const QString &text);

signals:
    void changed();

private:
    Theme();
    void apply();
    QString styleSheet() const;

    Mode mode_ = Mode::System;
    bool dark_ = true;
    Colors colors_;
    mutable QHash<QString, QIcon> icon_cache_;
};

#endif // THEME_H
