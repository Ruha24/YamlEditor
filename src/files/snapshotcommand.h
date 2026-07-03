#ifndef SNAPSHOTCOMMAND_H
#define SNAPSHOTCOMMAND_H

#include <QUndoCommand>

#include <functional>

#include "yaml/yamlnode.h"

class YamlSnapshotCommand : public QUndoCommand
{
public:
    using ApplyFn = std::function<void(const YamlNode &)>;

    YamlSnapshotCommand(const QString &text,
                        YamlNode before,
                        YamlNode after,
                        ApplyFn apply,
                        QUndoCommand *parent = nullptr)
        : QUndoCommand(text, parent)
        , before_(std::move(before))
        , after_(std::move(after))
        , apply_(std::move(apply))
    {}

    void undo() override { apply_(before_); }
    void redo() override { apply_(after_); }

private:
    YamlNode before_;
    YamlNode after_;
    ApplyFn apply_;
};

#endif // SNAPSHOTCOMMAND_H
