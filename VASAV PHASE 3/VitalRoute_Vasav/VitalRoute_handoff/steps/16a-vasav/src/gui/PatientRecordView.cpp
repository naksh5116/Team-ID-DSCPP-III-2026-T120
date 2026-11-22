// PatientRecordView.cpp
#include "PatientRecordView.hpp"

#include "EsiDelegate.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

using Text::qs;

// ------------------------------------------------------- visit history --

VisitHistoryModel::VisitHistoryModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Date"), QStringLiteral("Doctor"), QStringLiteral("Diagnosis"),
                          QStringLiteral("Prescriptions")},
                         parent)
{
    refreshOn(&SessionHub::patientsChanged);
}

void VisitHistoryModel::setPatientId(const QString& id)
{
    id_ = id;
    refresh();
}

void VisitHistoryModel::build(QList<Row>& rows, QStringList& keys) const
{
    if (id_.isEmpty())
        return;
    const Patient* p = hub().session().patients().tryFind(Text::ss(id_));
    if (!p)
        return;
    int n = 0;
    for (const VisitRecord& v : p->visitsNewestFirst()) {   // backward walk of the list
        rows.append({text(Text::date(v.date)), text(qs(v.doctor)), text(qs(v.diagnosis)),
                     text(qs(v.prescriptions))});
        keys.append(QString::number(n++));
    }
}

// ------------------------------------------------------------- record --

PatientRecordView::PatientRecordView(SessionHub& hub, QWidget* parent) : QWidget(parent), hub_(hub)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    badge_ = new EsiBadge(0, this);
    badge_->setLevel(0);
    title_ = new QLabel(this);
    title_->setObjectName(QStringLiteral("recordTitle"));
    title_->setFont(Theme::scaledFont(1.15, true));
    title_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    titleRow->addWidget(badge_);
    titleRow->addWidget(title_, 1);
    v->addLayout(titleRow);

    fields_ = new QWidget(this);
    auto* form = new QFormLayout(fields_);
    form->setContentsMargins(0, 0, 0, 0);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(4);
    addField(form, QStringLiteral("Patient ID"), "recordId", true);
    addField(form, QStringLiteral("Type"), "recordKind");
    addField(form, QStringLiteral("Status"), "recordStatus");
    addField(form, QStringLiteral("Complaint"), "recordComplaint");
    addField(form, QStringLiteral("Registered"), "recordRegistered");
    addField(form, QStringLiteral("Bed"), "recordBed");
    addField(form, QStringLiteral("Doctor"), "recordDoctor");
    addField(form, QStringLiteral("Discharged"), "recordDischarged");
    addField(form, QStringLiteral("Bill"), "recordBill");
    addField(form, QStringLiteral("Insurance"), "recordInsurance");
    v->addWidget(fields_);

    history_ = new VisitHistoryModel(hub_, this);
    historyTable_ = new QTableView;
    historyTable_->setObjectName(QStringLiteral("visitHistory"));
    historyTable_->setModel(history_);
    Theme::setupTable(historyTable_);
    fitColumnsOnReset(historyTable_);
    v->addWidget(tableSection(QStringLiteral("Visit history, newest first"),
                              QStringLiteral("Read from the patient's doubly linked list, walked from "
                                             "the tail."),
                              historyTable_, this),
                 1);

    connect(&hub_, &SessionHub::patientsChanged, this, &PatientRecordView::refresh);
    connect(&hub_, &SessionHub::bedsChanged, this, &PatientRecordView::refresh);
    connect(&hub_, &SessionHub::billsChanged, this, &PatientRecordView::refresh);
    connect(&hub_, &SessionHub::staffChanged, this, &PatientRecordView::refresh);
    connect(&hub_, &SessionHub::reloaded, this, &PatientRecordView::refresh);
    refresh();
}

QLabel* PatientRecordView::addField(QFormLayout* form, const QString& label, const char* name, bool mono)
{
    auto* value = new QLabel(fields_);
    value->setObjectName(QLatin1String(name));
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value->setWordWrap(true);
    if (mono)
        value->setFont(Theme::monoFont());
    form->addRow(label, value);
    return value;
}

QString PatientRecordView::fieldText(const char* objectName) const
{
    if (qstrcmp(objectName, "recordTitle") == 0)
        return title_->text();
    const QLabel* l = fields_->findChild<QLabel*>(QLatin1String(objectName));
    return l ? l->text() : QString();
}

bool PatientRecordView::hasPatient() const
{
    return !id_.isEmpty() && hub_.isStarted() && hub_.session().patients().tryFind(Text::ss(id_));
}

bool PatientRecordView::setPatientId(const QString& id)
{
    id_ = id.trimmed();
    goneReported_ = false;
    history_->setPatientId(id_);
    refresh();
    return id_.isEmpty() || hasPatient();
}

void PatientRecordView::refresh()
{
    auto set = [this](const char* name, const QString& value) {
        if (QLabel* l = fields_->findChild<QLabel*>(QLatin1String(name)))
            l->setText(value);
    };
    const Patient* p = hasPatient() ? hub_.session().patients().tryFind(Text::ss(id_)) : nullptr;
    if (!p) {
        const bool gone = !id_.isEmpty();
        title_->setText(gone ? QStringLiteral("%1 is not registered").arg(id_)
                             : QStringLiteral("No patient selected"));
        badge_->setLevel(0);
        fields_->setVisible(false);
        if (gone && !goneReported_) {
            goneReported_ = true;
            Q_EMIT patientGone(id_);
        }
        return;
    }
    const HospitalSession& s = hub_.session();
    fields_->setVisible(true);
    title_->setText(QStringLiteral("%1, %2").arg(qs(p->name())).arg(p->age()));
    badge_->setLevel(p->isEmergency() ? p->severity() : 0);
    set("recordId", qs(p->id()));
    set("recordKind", p->isEmergency()
                          ? QStringLiteral("Emergency, ESI %1 %2").arg(p->severity()).arg(Theme::esiName(p->severity()))
                          : QStringLiteral("Outpatient, %1").arg(qs(p->department())));
    set("recordStatus", Text::statusText(p->status()));
    set("recordComplaint", qs(p->complaint()));
    set("recordRegistered", Text::date(p->admissionDate()));
    set("recordBed", p->bedLabel().empty() ? QStringLiteral("None") : qs(p->bedLabel()));
    const Doctor* d = s.findDoctor(p->doctorId());
    set("recordDoctor", d ? QStringLiteral("%1 (%2)").arg(qs(d->name()), qs(d->specialty()))
                          : QStringLiteral("Not assigned yet"));
    set("recordDischarged", p->dischargeDate() ? Text::date(p->dischargeDate()) : QStringLiteral("Not discharged"));
    if (const Bill* b = s.findBill(p->id())) {
        set("recordBill", QStringLiteral("%1, %2 %3").arg(qs(b->billId()), Text::money(b->projectedTotal()),
                                                         b->settled() ? QStringLiteral("(settled)")
                                                                      : QStringLiteral("due")));
        set("recordInsurance", b->insurance() ? qs(b->insurance()->summary()) : QStringLiteral("None"));
    } else {
        set("recordBill", QStringLiteral("No bill"));
        set("recordInsurance", QStringLiteral("None"));
    }
}
