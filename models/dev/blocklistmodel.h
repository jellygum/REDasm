#pragma once

#include <QAbstractListModel>
#include <rdapi/rdapi.h>
#include "../hooks/idisassemblercommand.h"

class BlockListModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        explicit BlockListModel(IDisassemblerCommand* command, const RDBlockContainer* blocks, QObject *parent = nullptr);
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        int columnCount(const QModelIndex&) const override;
        int rowCount(const QModelIndex&) const override;

    private:
        QString blockType(const RDBlock* block) const;
        QString symbolName(const RDBlock* block) const;

    private:
        const RDBlockContainer* m_blocks;
        IDisassemblerCommand* m_command;
        RDDocument* m_document;
};
