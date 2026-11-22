// SnapshotTableModel.hpp - base for every table in the GUI.
//
// A view model only DISPLAYS what the session holds. On each SessionHub
// signal it is told about, it asks the session again (heap order from the
// triage unit, FIFO order from the OPD queue, deque order from the transfer
// desk, ...) and copies the visible text into its rows. The order always
// comes from the hand-written structures in src/engine; the model never
// sorts or stores patients itself, and it never keeps pointers into the
// session between refreshes (a reload replaces every object).
#ifndef VR_GUI_SNAPSHOT_TABLE_MODEL_HPP
#define VR_GUI_SNAPSHOT_TABLE_MODEL_HPP

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QStringList>

class SessionHub;

class SnapshotTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    // Qt::UserRole data on every cell: the row's key (patient ID, code, ...).
    static constexpr int KeyRole = Qt::UserRole + 1;

    struct Cell {
        QString text;
        int esi = 0;            // 1-5: drawn as an ESI badge by EsiDelegate
        bool mono = false;      // IDs, bed codes, amounts
        bool alignRight = false;// amounts only
    };
    using Row = QList<Cell>;

    SnapshotTableModel(SessionHub& hub, const QStringList& headers, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QString keyAt(int row) const;
    int rowOfKey(const QString& key) const;   // -1 when absent
    QString textAt(int row, int column) const;

public Q_SLOTS:
    // Re-reads the session. Safe to call at any time.
    void refresh();

protected:
    // Fill `rows` (and the matching `keys`) from hub().session().
    virtual void build(QList<Row>& rows, QStringList& keys) const = 0;

    SessionHub& hub() const { return hub_; }

    // Refresh whenever the given SessionHub signal fires.
    template <typename Signal>
    void refreshOn(Signal signal)
    {
        connect(&hub_, signal, this, &SnapshotTableModel::refresh);
    }

    static Cell text(const QString& t) { return Cell{t, 0, false, false}; }
    static Cell mono(const QString& t) { return Cell{t, 0, true, false}; }
    static Cell amount(const QString& t) { return Cell{t, 0, true, true}; }
    static Cell esi(int level, const QString& name) { return Cell{name, level, false, false}; }

private:
    SessionHub& hub_;
    QStringList headers_;
    QList<Row> rows_;
    QStringList keys_;
};

#endif // VR_GUI_SNAPSHOT_TABLE_MODEL_HPP
