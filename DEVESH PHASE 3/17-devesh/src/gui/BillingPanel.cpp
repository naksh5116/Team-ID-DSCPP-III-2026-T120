// BillingPanel.cpp
#include "BillingPanel.hpp"

#include "AdminModels.hpp"
#include "BillView.hpp"
#include "InsuranceFields.hpp"
#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QVBoxLayout>

#include <functional>
#include <memory>

using Text::qs;
using Text::ss;

namespace {

QFormLayout* formIn(QWidget* w)
{
    auto* form = new QFormLayout(w);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);
    return form;
}

QDoubleSpinBox* moneyBox(QWidget* parent, const char* name)
{
    auto* s = new QDoubleSpinBox(parent);
    s->setObjectName(QLatin1String(name));
    s->setRange(0.0, 10000000.0);
    s->setDecimals(2);
    s->setPrefix(QStringLiteral("Rs "));
    return s;
}

void enterSubmits(QWidget* scope, QObject* receiver, std::function<void()> action)
{
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), scope);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(sc, &QShortcut::activated, receiver, action);
    }
}

} // namespace

BillingPanel::BillingPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    // ------------------------------------------------ left: the forms
    auto* left = new QWidget;
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(8, 8, 8, 8);
    lv->setSpacing(12);

    auto* idRow = new QWidget(left);
    auto* ih = new QHBoxLayout(idRow);
    ih->setContentsMargins(0, 0, 0, 0);
    ih->setSpacing(8);
    ih->addWidget(new QLabel(QStringLiteral("Patient ID"), idRow));
    idEdit_ = new QLineEdit(idRow);
    idEdit_->setObjectName(QStringLiteral("billingId"));
    idEdit_->setFont(Theme::monoFont());
    idEdit_->setPlaceholderText(QStringLiteral("VR-2026-000004"));
    ih->addWidget(idEdit_, 1);
    auto* open = new QPushButton(QStringLiteral("Open bill"), idRow);
    open->setObjectName(QStringLiteral("billingOpen"));
    open->setProperty("primary", true);
    connect(open, &QPushButton::clicked, this, [this] { openBill(); });
    ih->addWidget(open);
    enterSubmits(idRow, this, [this] { openBill(); });
    lv->addWidget(idRow);

    // Manual charge.
    chargeBox_ = new QGroupBox(QStringLiteral("Add a manual charge"), left);
    auto* cf = formIn(chargeBox_);
    authoriser_ = new QComboBox(chargeBox_);
    authoriser_->setObjectName(QStringLiteral("chargeAuthoriser"));
    cf->addRow(QStringLiteral("Authorised by"), authoriser_);
    type_ = new QComboBox(chargeBox_);
    type_->setObjectName(QStringLiteral("chargeType"));
    type_->addItems({QStringLiteral("Bed"), QStringLiteral("Medicine"), QStringLiteral("Test"),
                     QStringLiteral("Procedure")});
    cf->addRow(QStringLiteral("Charge type"), type_);
    description_ = new QLineEdit(chargeBox_);
    description_->setObjectName(QStringLiteral("chargeDescription"));
    cf->addRow(QStringLiteral("Description"), description_);

    fields_ = new QStackedWidget(chargeBox_);
    auto* bedPage = new QWidget;
    auto* bf = formIn(bedPage);
    bf->setContentsMargins(0, 0, 0, 0);
    bedRate_ = moneyBox(bedPage, "chargeBedRate");
    bedDays_ = new QSpinBox(bedPage);
    bedDays_->setObjectName(QStringLiteral("chargeBedDays"));
    bedDays_->setRange(1, 365);
    bf->addRow(QStringLiteral("Rate per day"), bedRate_);
    bf->addRow(QStringLiteral("Days"), bedDays_);
    auto* medPage = new QWidget;
    auto* mf = formIn(medPage);
    mf->setContentsMargins(0, 0, 0, 0);
    medUnit_ = moneyBox(medPage, "chargeMedUnit");
    medQty_ = new QSpinBox(medPage);
    medQty_->setObjectName(QStringLiteral("chargeMedQty"));
    medQty_->setRange(1, 10000);
    mf->addRow(QStringLiteral("Unit price"), medUnit_);
    mf->addRow(QStringLiteral("Quantity"), medQty_);
    auto* testPage = new QWidget;
    auto* tf = formIn(testPage);
    tf->setContentsMargins(0, 0, 0, 0);
    testPrice_ = moneyBox(testPage, "chargeTestPrice");
    testUrgent_ = new QCheckBox(QStringLiteral("Urgent (+25%)"), testPage);
    testUrgent_->setObjectName(QStringLiteral("chargeTestUrgent"));
    tf->addRow(QStringLiteral("Price"), testPrice_);
    tf->addRow(QString(), testUrgent_);
    auto* procPage = new QWidget;
    auto* pf = formIn(procPage);
    pf->setContentsMargins(0, 0, 0, 0);
    procFee_ = moneyBox(procPage, "chargeProcFee");
    procFacility_ = moneyBox(procPage, "chargeProcFacility");
    procConsumables_ = moneyBox(procPage, "chargeProcConsumables");
    pf->addRow(QStringLiteral("Professional fee"), procFee_);
    pf->addRow(QStringLiteral("Facility fee"), procFacility_);
    pf->addRow(QStringLiteral("Consumables"), procConsumables_);
    for (QWidget* page : {bedPage, medPage, testPage, procPage})
        fields_->addWidget(page);
    // Each page takes only its own height, so no gap is left under short pages.
    auto showPage = [this](int index) {
        for (int i = 0; i < fields_->count(); ++i)
            fields_->widget(i)->setSizePolicy(QSizePolicy::Preferred,
                                              i == index ? QSizePolicy::Preferred : QSizePolicy::Ignored);
        fields_->setCurrentIndex(index);
        fields_->adjustSize();
    };
    connect(type_, &QComboBox::currentIndexChanged, this, showPage);
    showPage(0);
    cf->addRow(fields_);
    auto* add = new QPushButton(QStringLiteral("Add charge"), chargeBox_);
    add->setObjectName(QStringLiteral("chargeAdd"));
    connect(add, &QPushButton::clicked, this, [this] { addCharge(); });
    cf->addRow(QString(), leftAligned(add));
    enterSubmits(chargeBox_, this, [this] { addCharge(); });
    lv->addWidget(chargeBox_);

    // Insurance.
    insuranceBox_ = new QGroupBox(QStringLiteral("Record or replace insurance"), left);
    auto* iv = new QVBoxLayout(insuranceBox_);
    iv->setSpacing(8);
    insurance_ = new InsuranceFields(false, insuranceBox_);
    insurance_->setObjectName(QStringLiteral("billingInsurance"));
    iv->addWidget(insurance_);
    auto* recordIns = new QPushButton(QStringLiteral("Record insurance"), insuranceBox_);
    recordIns->setObjectName(QStringLiteral("insuranceRecord"));
    connect(recordIns, &QPushButton::clicked, this, [this] { recordInsurance(); });
    auto* insRow = new QHBoxLayout;
    insRow->addWidget(recordIns);
    insRow->addStretch(1);
    iv->addLayout(insRow);
    enterSubmits(insuranceBox_, this, [this] { recordInsurance(); });
    lv->addWidget(insuranceBox_);

    // Discharge.
    dischargeBox_ = new QGroupBox(QStringLiteral("Discharge and settle"), left);
    auto* df = formIn(dischargeBox_);
    dischargeDate_ = new QDateEdit(dischargeBox_);
    dischargeDate_->setObjectName(QStringLiteral("dischargeDate"));
    dischargeDate_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dischargeDate_->setCalendarPopup(true);
    dischargeDate_->setDate(Text::qdate(vrclock::today()));
    df->addRow(QStringLiteral("Discharge date"), dischargeDate_);
    auto* dischargeButton = new QPushButton(QStringLiteral("Discharge and settle bill"), dischargeBox_);
    dischargeButton->setObjectName(QStringLiteral("dischargeButton"));
    connect(dischargeButton, &QPushButton::clicked, this, [this] { discharge(); });
    df->addRow(QString(), leftAligned(dischargeButton));
    df->addRow(roleLabel(QStringLiteral("Bed days after the first are charged up to the discharge date, the "
                                        "bed is released and insurance is deducted once."),
                         "hint", dischargeBox_));
    lv->addWidget(dischargeBox_);
    lv->addStretch(1);
    split->addWidget(scrollable(left, split));

    // ---------------------------------------- right: bill and all bills
    auto* right = new QSplitter(Qt::Vertical);
    right->setChildrenCollapsible(false);
    auto* billHost = new QWidget;
    auto* bh = new QVBoxLayout(billHost);
    bh->setContentsMargins(8, 8, 8, 8);
    bill_ = new BillView(hub_, billHost);
    bh->addWidget(bill_);
    right->addWidget(billHost);

    auto* listHost = new QWidget;
    auto* lh = new QVBoxLayout(listHost);
    lh->setContentsMargins(0, 0, 0, 0);
    bills_ = new BillsListModel(hub_, this);
    billsTable_ = new QTableView;
    billsTable_->setObjectName(QStringLiteral("allBills"));
    billsTable_->setModel(bills_);
    Theme::setupTable(billsTable_);
    fitColumnsOnReset(billsTable_);
    auto* section = tableSection(QStringLiteral("All bills"),
                                 QStringLiteral("Merge-sorted (stable). Double-click a bill to open it."),
                                 billsTable_, listHost);
    auto* sortRow = new QHBoxLayout;
    sortRow->setSpacing(8);
    sortRow->addWidget(new QLabel(QStringLiteral("Sort by"), section));
    auto* sort = new QComboBox(section);
    sort->setObjectName(QStringLiteral("billsSort"));
    sort->addItems({QStringLiteral("Date opened"), QStringLiteral("Amount, largest first")});
    connect(sort, &QComboBox::currentIndexChanged, this, [this](int i) { bills_->setSortByAmount(i == 1); });
    sortRow->addWidget(sort);
    sortRow->addStretch(1);
    static_cast<QVBoxLayout*>(section->layout())->insertLayout(2, sortRow);
    lh->addWidget(section);
    connect(billsTable_, &QTableView::doubleClicked, this,
            [this](const QModelIndex& i) { openBill(bills_->keyAt(i.row())); });
    right->addWidget(listHost);
    right->setStretchFactor(0, 3);
    right->setStretchFactor(1, 2);
    split->addWidget(right);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);

    connect(&hub_, &SessionHub::staffChanged, this, &BillingPanel::refreshAdmins);
    connect(&hub_, &SessionHub::reloaded, this, &BillingPanel::refreshAdmins);
    connect(&hub_, &SessionHub::billsChanged, this, &BillingPanel::updateEditable);
    connect(&hub_, &SessionHub::patientsChanged, this, &BillingPanel::updateEditable);
    refreshAdmins();
    updateEditable();
}

void BillingPanel::refreshAdmins()
{
    if (!hub_.isStarted())
        return;
    authoriser_->clear();
    // Only staff who may authorise manual charges (Staff::canAuthorizeCharges).
    for (const Admin* a : hub_.session().staff().admins())
        if (a->canAuthorizeCharges())
            authoriser_->addItem(QStringLiteral("%1  %2").arg(qs(a->id()), qs(a->name())), qs(a->id()));
}

void BillingPanel::updateEditable()
{
    const Bill* b = (!id_.isEmpty() && hub_.isStarted()) ? hub_.session().findBill(ss(id_)) : nullptr;
    const bool open = b && !b->settled();
    chargeBox_->setEnabled(open);
    insuranceBox_->setEnabled(open);
    dischargeBox_->setEnabled(open);
}

bool BillingPanel::openBill()
{
    return openBill(idEdit_->text());
}

bool BillingPanel::openBill(const QString& rawId)
{
    host_.showMessage(QString());
    const QString id = rawId.trimmed();
    if (id.isEmpty()) {
        host_.showError(QStringLiteral("Enter a patient ID."));
        return false;
    }
    idEdit_->setText(id);
    const bool ok = host_.attempt([&] { hub_.session().findPatient(ss(id)); });
    id_ = ok ? id : QString();
    bill_->setPatientId(id_);
    updateEditable();
    if (ok)
        if (const Bill* b = hub_.session().findBill(ss(id_)); b && b->settled())
            host_.showMessage(QStringLiteral("Bill %1 is settled and can no longer be changed.").arg(qs(b->billId())));
    return ok;
}

bool BillingPanel::addCharge()
{
    host_.showMessage(QString());
    if (id_.isEmpty()) {
        host_.showError(QStringLiteral("Open a bill first."));
        return false;
    }
    const QString desc = description_->text().trimmed();
    if (desc.isEmpty()) {
        host_.showError(QStringLiteral("Enter a description for the charge."));
        description_->setFocus();
        return false;
    }
    if (authoriser_->currentIndex() < 0) {
        host_.showError(QStringLiteral("No administrator on the roster can authorise the charge."));
        return false;
    }
    const int today = vrclock::today();
    std::unique_ptr<Charge> charge;
    switch (type_->currentIndex()) {
    case 0: charge = std::make_unique<BedCharge>(ss(desc), today, bedRate_->value(), bedDays_->value()); break;
    case 1: charge = std::make_unique<MedicineCharge>(ss(desc), today, medUnit_->value(), medQty_->value()); break;
    case 2: charge = std::make_unique<TestCharge>(ss(desc), today, testPrice_->value(), testUrgent_->isChecked()); break;
    default:
        charge = std::make_unique<ProcedureCharge>(ss(desc), today, procFee_->value(), procFacility_->value(),
                                                   procConsumables_->value());
        break;
    }
    const bool ok = host_.attempt([&] {
        hub_.addManualCharge(ss(id_), *charge, ss(authoriser_->currentData().toString()));
    });
    if (!ok)
        return false;
    // The console's sentence after a manual charge.
    host_.showMessage(QStringLiteral("Added %1. New subtotal %2.")
                          .arg(Text::money(charge->calculateCost()),
                               Text::money(hub_.session().findBill(ss(id_))->subtotal())));
    description_->clear();
    return true;
}

bool BillingPanel::recordInsurance()
{
    host_.showMessage(QString());
    if (id_.isEmpty()) {
        host_.showError(QStringLiteral("Open a bill first."));
        return false;
    }
    QString error;
    const std::optional<Insurance> cover = insurance_->value(&error);
    if (!cover) {
        host_.showError(error.isEmpty() ? QStringLiteral("Enter the insurer.") : error);
        return false;
    }
    const bool ok = host_.attempt([&] { hub_.attachInsurance(ss(id_), *cover); });
    if (ok) {
        host_.showMessage(QStringLiteral("Insurance recorded; it is deducted once, when the bill is settled."));
        insurance_->clear();
    }
    return ok;
}

bool BillingPanel::discharge()
{
    using Block = HospitalSession::DischargeBlock;
    host_.showMessage(QString());
    if (id_.isEmpty()) {
        host_.showError(QStringLiteral("Open a bill first."));
        return false;
    }
    const HospitalSession& s = hub_.session();
    const Patient* p = s.patients().tryFind(ss(id_));
    if (!p) {
        host_.showError(QStringLiteral("Not found: no patient with ID %1.").arg(id_));
        return false;
    }
    // The console's refusals.
    const Block block = s.dischargeBlocker(p->id());
    if (block == Block::StillWaiting) {
        host_.showMessage(QStringLiteral("%1 is still waiting to be seen.").arg(id_));
        return false;
    }
    if (block == Block::AlreadyDischarged) {
        host_.showMessage(QStringLiteral("%1 was already discharged.").arg(id_));
        return false;
    }

    int date = Text::ymd(dischargeDate_->date());
    const bool clamped = date < p->admissionDate();
    if (clamped)
        date = p->admissionDate();
    const Bill* bill = s.findBill(p->id());
    const QString bed = qs(p->bedLabel());
    const QString billId = bill ? qs(bill->billId()) : QString();
    QString text = QStringLiteral("Discharge %1 %2 on %3 and settle bill %4?\n\n")
                       .arg(qs(p->id()), qs(p->name()), Text::date(date), billId);
    if (!bed.isEmpty())
        text += QStringLiteral("%1 is released; bed days after the first are charged up to the discharge date.\n").arg(bed);
    text += bill && bill->insurance()
                ? QStringLiteral("Insurance deducted once: %1.\n").arg(qs(bill->insurance()->summary()))
                : QStringLiteral("No insurance is recorded; the full amount is due.\n");
    text += QStringLiteral("\nThis cannot be undone.");
    if (!confirm(this, QStringLiteral("Discharge and settle"), text, QStringLiteral("Discharge")))
        return false;

    const bool ok = host_.attempt([&] { hub_.discharge(p->id(), date, std::nullopt); });
    if (!ok)
        return false;
    const Bill* settled = hub_.session().findBill(ss(id_));
    QStringList parts;
    if (clamped)
        parts << QStringLiteral("Discharge cannot be before admission; using the admission date.");
    if (!bed.isEmpty())
        parts << QStringLiteral("%1 is free again.").arg(bed);
    parts << QStringLiteral("Bill %1 settled: paid %2.").arg(billId, Text::money(settled ? settled->total() : 0.0));
    host_.showMessage(parts.join(QLatin1Char(' ')));
    updateEditable();
    return true;
}
