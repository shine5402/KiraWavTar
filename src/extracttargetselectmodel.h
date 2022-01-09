#ifndef EXTRACTTARGETSELECTMODEL_H
#define EXTRACTTARGETSELECTMODEL_H

#include <QAbstractTableModel>

class ExtractTargetSelectModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ExtractTargetSelectModel(QJsonArray* descArray, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void selectAll();
    void unselectAll();
    void reverseSelect();
private:
    QJsonArray* descArray;

};

#endif // EXTRACTTARGETSELECTMODEL_H
