// SnapshotTableModel.cpp - rows copied from the session on each refresh.
#include "SnapshotTableModel.hpp"

#include "EsiDelegate.hpp"
#include "SessionHub.hpp"
#include "Theme.hpp"

SnapshotTableModel::SnapshotTableModel(SessionHub& hub, const QStringList& headers,
                                       QObject* parent)
    : QAbstractTableModel(parent), hub_(hub), headers_(headers)
{
    // Every model follows a full reload or an explicit refresh.
    connect(&hub_, &SessionHub::reloaded, this, &SnapshotTableModel::refresh);
}

int SnapshotTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

int SnapshotTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(headers_.size());
}

QVariant SnapshotTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rows_.size())
        return QVariant();
    const Row& row = rows_.at(index.row());
    if (index.column() >= row.size())
        return QVariant();
    const Cell& c = row.at(index.column());
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return c.text;
    case Qt::FontRole:
        if (c.mono)
            return Theme::monoFont();
        return QVariant();
    case Qt::TextAlignmentRole:
        return static_cast<int>(Qt::AlignVCenter | (c.alignRight ? Qt::AlignRight : Qt::AlignLeft));
    case EsiDelegate::LevelRole:
        return c.esi > 0 ? QVariant(c.esi) : QVariant();
    case KeyRole:
        return keys_.value(index.row());
    default:
        return QVariant();
    }
}

QVariant SnapshotTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return headers_.value(section);
    return QAbstractTableModel::headerData(section, orientation, role);
}

QString SnapshotTableModel::keyAt(int row) const
{
    return keys_.value(row);
}

int SnapshotTableModel::rowOfKey(const QString& key) const
{
    return static_cast<int>(keys_.indexOf(key));
}

QString SnapshotTableModel::textAt(int row, int column) const
{
    if (row < 0 || row >= rows_.size() || column < 0 || column >= rows_.at(row).size())
        return QString();
    return rows_.at(row).at(column).text;
}

void SnapshotTableModel::refresh()
{
    QList<Row> rows;
    QStringList keys;
    if (hub_.isStarted())
        build(rows, keys);
    beginResetModel();
    rows_ = std::move(rows);
    keys_ = std::move(keys);
    endResetModel();
}
