#include "customtablemodel.h"

CustomTableModel::CustomTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void CustomTableModel::setData(const QList<QStringList> &data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void CustomTableModel::setColumnNames(const QStringList &columnNames)
{
    beginResetModel();
    m_columnNames = columnNames;
    endResetModel();
}

int CustomTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_data.size();
}

int CustomTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    if (m_data.isEmpty())
        return 0;
    return m_data.first().size();
}

QVariant CustomTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return m_data[index.row()][index.column()];
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter; // Align data in the center
    }

    return QVariant();
}

QVariant CustomTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal && section >= 0 && section < m_columnNames.size())
        return m_columnNames[section];
    else
        return section + 1;
}

int CustomTableModel::columnIndex(const QString &columnName) const
{
    // Iterate over header data to find the matching column name
    for (int i = 0; i < m_columnNames.size(); ++i) {
        if (m_columnNames[i] == columnName)
            return i;
    }
    qDebug() << "Column" << columnName << "not found in the model."; // Add debug statement
    return -1; // Column not found
}
