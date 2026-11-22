// PatientLookupPanel.cpp
#include "PatientLookupPanel.hpp"

#include "PatientRecordView.hpp"
#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QVBoxLayout>

#include <functional>

using Text::qs;
using Text::ss;

namespace {

void enterSubmits(QWidget* scope, QObject* receiver, std::function<void()> action)
{
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), scope);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(sc, &QShortcut::activated, receiver, action);
    }
}

} // namespace

PatientLookupPanel::PatientLookupPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    // ---- left: search + record
    auto* left = new QWidget;
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(8, 8, 8, 8);
    lv->setSpacing(8);

    auto* searchRow = new QWidget(left);
    auto* sh = new QHBoxLayout(searchRow);
    sh->setContentsMargins(0, 0, 0, 0);
    sh->setSpacing(8);
    sh->addWidget(new QLabel(QStringLiteral("Patient ID"), searchRow));
    id_ = new QLineEdit(searchRow);
    id_->setObjectName(QStringLiteral("lookupId"));
    id_->setFont(Theme::monoFont());
    id_->setPlaceholderText(QStringLiteral("VR-2026-000001"));
    sh->addWidget(id_, 1);
    auto* go = new QPushButton(QStringLiteral("Look up"), searchRow);
    go->setObjectName(QStringLiteral("lookupButton"));
    go->setProperty("primary", true);
    connect(go, &QPushButton::clicked, this, [this] { lookUp(); });
    sh->addWidget(go);
    enterSubmits(searchRow, this, [this] { lookUp(); });
    lv->addWidget(searchRow);

    indexStats_ = roleLabel(QString(), "hint", left);
    indexStats_->setObjectName(QStringLiteral("hashStats"));
    lv->addWidget(indexStats_);

    record_ = new PatientRecordView(hub_, left);
    lv->addWidget(record_, 1);
    split->addWidget(left);

    // ---- right: add a visit note
    auto* right = new QWidget;
    auto* rv = new QVBoxLayout(right);
    rv->setContentsMargins(8, 8, 8, 8);
    rv->setSpacing(12);
    noteBox_ = new QGroupBox(QStringLiteral("Add a visit note"), right);
    auto* form = new QFormLayout(noteBox_);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);
    noteDate_ = new QDateEdit(noteBox_);
    noteDate_->setObjectName(QStringLiteral("noteDate"));
    noteDate_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    noteDate_->setCalendarPopup(true);
    noteDate_->setDate(Text::qdate(vrclock::today()));
    noteDoctor_ = new QComboBox(noteBox_);
    noteDoctor_->setObjectName(QStringLiteral("noteDoctor"));
    noteDoctor_->setEditable(true);   // the console accepts any name
    noteDiagnosis_ = new QLineEdit(noteBox_);
    noteDiagnosis_->setObjectName(QStringLiteral("noteDiagnosis"));
    notePrescriptions_ = new QLineEdit(noteBox_);
    notePrescriptions_->setObjectName(QStringLiteral("notePrescriptions"));
    notePrescriptions_->setPlaceholderText(QStringLiteral("None"));
    form->addRow(QStringLiteral("Visit date"), noteDate_);
    form->addRow(QStringLiteral("Doctor"), noteDoctor_);
    form->addRow(QStringLiteral("Diagnosis"), noteDiagnosis_);
    form->addRow(QStringLiteral("Prescriptions"), notePrescriptions_);
    auto* add = new QPushButton(QStringLiteral("Add visit note"), noteBox_);
    add->setObjectName(QStringLiteral("noteAdd"));
    connect(add, &QPushButton::clicked, this, [this] { addVisitNote(); });
    form->addRow(QString(), leftAligned(add));
    enterSubmits(noteBox_, this, [this] { addVisitNote(); });
    rv->addWidget(noteBox_);
    rv->addWidget(roleLabel(QStringLiteral(
        "Notes go into the visit history in date order: at the head or tail in one step, "
        "otherwise walked into place."), "hint", right));
    rv->addStretch(1);
    split->addWidget(right);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);

    noteBox_->setEnabled(false);
    connect(record_, &PatientRecordView::patientGone, this, [this](const QString& id) {
        noteBox_->setEnabled(false);
        host_.showMessage(QStringLiteral("%1 is no longer registered.").arg(id));
    });
    connect(&hub_, &SessionHub::staffChanged, this, &PatientLookupPanel::refreshDoctors);
    connect(&hub_, &SessionHub::reloaded, this, &PatientLookupPanel::refreshDoctors);
    connect(&hub_, &SessionHub::patientsChanged, this, &PatientLookupPanel::refreshIndexStats);
    connect(&hub_, &SessionHub::reloaded, this, &PatientLookupPanel::refreshIndexStats);
    refreshDoctors();
    refreshIndexStats();
}

void PatientLookupPanel::refreshDoctors()
{
    if (!hub_.isStarted())
        return;
    const QString current = noteDoctor_->currentText();
    noteDoctor_->clear();
    for (const Doctor* d : hub_.session().staff().doctors())
        noteDoctor_->addItem(qs(d->name()));
    if (!current.isEmpty())
        noteDoctor_->setCurrentText(current);
}

void PatientLookupPanel::refreshIndexStats()
{
    if (!hub_.isStarted())
        return;
    const vr_hash_stats st = hub_.session().patients().hashStats();
    indexStats_->setText(QStringLiteral("Patient ID index (hash table): %1 entries in %2 buckets, load %3, "
                                        "%4 collisions, %5 resizes.")
                             .arg(st.entries).arg(st.buckets).arg(st.load_factor, 0, 'f', 2)
                             .arg(st.collisions).arg(st.resizes));
}

bool PatientLookupPanel::lookUp()
{
    return lookUp(id_->text());
}

bool PatientLookupPanel::lookUp(const QString& rawId)
{
    host_.showMessage(QString());
    const QString id = rawId.trimmed();
    if (id.isEmpty()) {
        host_.showError(QStringLiteral("Enter a patient ID, for example VR-2026-000001."));
        id_->setFocus();
        return false;
    }
    id_->setText(id);
    const bool ok = host_.attempt([&] { hub_.session().findPatient(ss(id)); });   // hash lookup
    if (!ok) {
        record_->setPatientId(QString());
        noteBox_->setEnabled(false);
        return false;
    }
    record_->setPatientId(id);
    noteBox_->setEnabled(true);
    return true;
}

bool PatientLookupPanel::addVisitNote()
{
    host_.showMessage(QString());
    if (!record_->hasPatient()) {
        host_.showError(QStringLiteral("Look up a patient first."));
        return false;
    }
    if (noteDoctor_->currentText().trimmed().isEmpty()) {
        host_.showError(QStringLiteral("Enter the doctor's name."));
        noteDoctor_->setFocus();
        return false;
    }
    if (noteDiagnosis_->text().trimmed().isEmpty()) {
        host_.showError(QStringLiteral("Enter the diagnosis."));
        noteDiagnosis_->setFocus();
        return false;
    }
    const QString id = record_->patientId();
    const bool ok = host_.attempt([&] {
        hub_.addVisitNote(ss(id), Text::ymd(noteDate_->date()), ss(noteDoctor_->currentText()),
                          ss(noteDiagnosis_->text()), ss(notePrescriptions_->text()));
    });
    if (!ok)
        return false;
    host_.showMessage(QStringLiteral("Visit added in date order to %1.").arg(id));
    noteDiagnosis_->clear();
    notePrescriptions_->clear();
    noteDate_->setDate(Text::qdate(vrclock::today()));
    return true;
}
