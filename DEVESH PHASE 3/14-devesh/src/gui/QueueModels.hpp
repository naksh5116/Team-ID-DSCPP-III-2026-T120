// QueueModels.hpp - the waiting lines and the transfer desk as tables.
//
//   EmergencyQueueModel  TriageUnit's max-heap, most urgent first
//                        (TriageUnit::emergencyOrder, Patient::operator<)
//   OpdQueueModel        TriageUnit's FIFO queue, first come first served
//                        (TriageUnit::opdLine)
//   TransferModel        TransferDesk's deque, front to back
//                        (TransferDesk::pending)
#ifndef VR_GUI_QUEUE_MODELS_HPP
#define VR_GUI_QUEUE_MODELS_HPP

#include "SnapshotTableModel.hpp"

class EmergencyQueueModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit EmergencyQueueModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Position, Id, Name, Age, Esi, Complaint, Registered };

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

class OpdQueueModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit OpdQueueModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Position, Id, Name, Age, Department, Reason };

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

class TransferModel : public SnapshotTableModel {
    Q_OBJECT
public:
    explicit TransferModel(SessionHub& hub, QObject* parent = nullptr);
    enum Column { Position, Code, Priority, Type, PatientId, From, To };

protected:
    void build(QList<Row>& rows, QStringList& keys) const override;
};

#endif // VR_GUI_QUEUE_MODELS_HPP
