#ifndef CUSTOMTABLEMODEL_H
#define CUSTOMTABLEMODEL_H

#include <QAbstractTableModel>

class CustomTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CustomTableModel(QObject *parent = nullptr);
    void setData(const QList<QStringList> &data);
    void setColumnNames(const QStringList &columnNames); // New method to set column names dynamically
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnIndex(const QString &columnName) const;

private:
    QList<QStringList> m_data;
    QStringList m_columnNames; // New member variable to store column names
};

#endif // CUSTOMTABLEMODEL_H
