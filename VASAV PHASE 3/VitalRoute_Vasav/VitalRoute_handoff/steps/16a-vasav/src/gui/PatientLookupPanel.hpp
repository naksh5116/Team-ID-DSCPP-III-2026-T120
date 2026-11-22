// PatientLookupPanel.hpp - console option 4: look up a patient by ID (hash
// table), show the record and visit history, add a visit note.
// Used by the Doctor and Super Admin windows.
#ifndef VR_GUI_PATIENT_LOOKUP_PANEL_HPP
#define VR_GUI_PATIENT_LOOKUP_PANEL_HPP

#include <QWidget>

class PatientRecordView;
class QComboBox;
class QDateEdit;
class QGroupBox;
class QLabel;
class QLineEdit;
class RoleWindow;
class SessionHub;

class PatientLookupPanel : public QWidget {
    Q_OBJECT
public:
    PatientLookupPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    bool lookUp(const QString& id);
    bool lookUp();               // the ID typed in the field
    bool addVisitNote();
    PatientRecordView* record() const { return record_; }

private:
    void refreshDoctors();
    void refreshIndexStats();

    SessionHub& hub_;
    RoleWindow& host_;
    QLineEdit* id_ = nullptr;
    QLabel* indexStats_ = nullptr;
    PatientRecordView* record_ = nullptr;
    QGroupBox* noteBox_ = nullptr;
    QDateEdit* noteDate_ = nullptr;
    QComboBox* noteDoctor_ = nullptr;
    QLineEdit* noteDiagnosis_ = nullptr;
    QLineEdit* notePrescriptions_ = nullptr;
};

#endif // VR_GUI_PATIENT_LOOKUP_PANEL_HPP
