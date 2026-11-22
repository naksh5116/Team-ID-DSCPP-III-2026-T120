// AdminPanels.cpp
#include "AdminPanels.hpp"

#include "AdminModels.hpp"
#include "DoctorAvailabilityPanel.hpp"
#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

#include <utility>

using Text::qs;

// ---------------------------------------------------------- admissions --

AdmissionsPanel::AdmissionsPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(8);

    auto* row = new QWidget(this);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);
    const int today = vrclock::today();
    auto makeDate = [row](const char* name, int ymd) {
        auto* d = new QDateEdit(row);
        d->setObjectName(QLatin1String(name));
        d->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
        d->setCalendarPopup(true);
        d->setDate(Text::qdate(ymd));
        return d;
    };
    from_ = makeDate("admissionsFrom", vrclock::addDays(today, -30));   // console defaults
    to_ = makeDate("admissionsTo", today);
    h->addWidget(new QLabel(QStringLiteral("From"), row));
    h->addWidget(from_);
    h->addWidget(new QLabel(QStringLiteral("To"), row));
    h->addWidget(to_);
    auto* go = new QPushButton(QStringLiteral("Show admissions"), row);
    go->setObjectName(QStringLiteral("admissionsShow"));
    go->setProperty("primary", true);
    connect(go, &QPushButton::clicked, this, &AdmissionsPanel::showRange);
    h->addWidget(go);
    h->addStretch(1);
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), row);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, &AdmissionsPanel::showRange);
    }
    v->addWidget(row);

    summary_ = new QLabel(this);
    summary_->setObjectName(QStringLiteral("admissionsSummary"));
    v->addWidget(summary_);

    model_ = new AdmissionsModel(hub_, this);
    table_ = new QTableView;
    table_->setObjectName(QStringLiteral("admissionsTable"));
    table_->setModel(model_);
    Theme::setupTable(table_);
    fitColumnsOnReset(table_);
    auto* section = tableSection(QStringLiteral("Registrations in the range, in date order"),
                                 QStringLiteral("Found by an in-order walk of the admission-date AVL tree "
                                                "that skips subtrees outside the range."),
                                 table_, this);
    avl_ = new QLabel(section);
    avl_->setObjectName(QStringLiteral("admissionsAvl"));
    static_cast<QVBoxLayout*>(section->layout())->insertWidget(2, avl_);
    v->addWidget(section, 1);

    connect(model_, &QAbstractItemModel::modelReset, this, &AdmissionsPanel::refreshTexts);
    showRange();
}

void AdmissionsPanel::showRange()
{
    showAdmissions(Text::ymd(from_->date()), Text::ymd(to_->date()));
}

void AdmissionsPanel::showAdmissions(int fromYmd, int toYmd)
{
    if (fromYmd > toYmd)
        std::swap(fromYmd, toYmd);   // as the console does
    shownFrom_ = fromYmd;
    shownTo_ = toYmd;
    from_->setDate(Text::qdate(fromYmd));
    to_->setDate(Text::qdate(toYmd));
    model_->setRange(fromYmd, toYmd);
}

void AdmissionsPanel::refreshTexts()
{
    if (!hub_.isStarted() || !shownFrom_)
        return;
    // The console's option 5 heading and index line.
    summary_->setText(QStringLiteral("%1 patient(s) registered %2 to %3.")
                          .arg(model_->rowCount()).arg(Text::date(shownFrom_), Text::date(shownTo_)));
    const vr_avl& idx = hub_.session().patients().dateIndex();
    avl_->setText(QStringLiteral("AVL index: %1 dates, height %2, rotations LL %3 RR %4 LR %5 RL %6.")
                      .arg(idx.nodes).arg(vr_avl_height(&idx)).arg(idx.rot_ll).arg(idx.rot_rr)
                      .arg(idx.rot_lr).arg(idx.rot_rl));
}

QString AdmissionsPanel::summary() const
{
    return summary_->text();
}

QString AdmissionsPanel::avlText() const
{
    return avl_->text();
}

// --------------------------------------------------------------- staff --

StaffPanel::StaffPanel(SessionHub& hub, RoleWindow& host, QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Vertical, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    roster_ = new StaffRosterModel(hub, this);
    auto* table = new QTableView;
    table->setObjectName(QStringLiteral("staffRoster"));
    table->setModel(roster_);
    Theme::setupTable(table);
    fitColumnsOnReset(table);
    split->addWidget(tableSection(QStringLiteral("Staff roster"),
                                  QStringLiteral("Doctors, nurses and administrators share the Staff base "
                                                 "class; only administrators may authorise manual charges."),
                                  table, split));
    split->addWidget(new DoctorAvailabilityPanel(hub, host, split));
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
}

// ------------------------------------------------------------- indexes --

namespace {

QLabel* statField(QFormLayout* form, QWidget* parent, const QString& label, const char* name)
{
    auto* l = new QLabel(parent);
    l->setObjectName(QLatin1String(name));
    l->setFont(Theme::monoFont());
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(label, l);
    return l;
}

} // namespace

IndexStatsPanel::IndexStatsPanel(SessionHub& hub, QWidget* parent) : QWidget(parent), hub_(hub)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(12);

    auto* row = new QHBoxLayout;
    row->setSpacing(12);

    auto* hashBox = new QGroupBox(QStringLiteral("Patient ID index (hash table, separate chaining)"), this);
    auto* hf = new QFormLayout(hashBox);
    hf->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hf->setHorizontalSpacing(16);
    hf->setVerticalSpacing(6);
    statField(hf, hashBox, QStringLiteral("Entries"), "hashEntries");
    statField(hf, hashBox, QStringLiteral("Buckets"), "hashBuckets");
    statField(hf, hashBox, QStringLiteral("Buckets in use"), "hashUsed");
    statField(hf, hashBox, QStringLiteral("Load factor"), "hashLoad");
    statField(hf, hashBox, QStringLiteral("Longest chain"), "hashLongest");
    statField(hf, hashBox, QStringLiteral("Collisions"), "hashCollisions");
    statField(hf, hashBox, QStringLiteral("Resizes"), "hashResizes");
    hf->addRow(roleLabel(QStringLiteral("FNV-1a hash. The bucket array doubles before the load factor would "
                                        "pass its limit. A collision is an insert into a bucket that already "
                                        "holds an entry."), "hint", hashBox));
    row->addWidget(hashBox, 1);

    auto* avlBox = new QGroupBox(QStringLiteral("Admission-date index (AVL tree)"), this);
    auto* af = new QFormLayout(avlBox);
    af->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    af->setHorizontalSpacing(16);
    af->setVerticalSpacing(6);
    statField(af, avlBox, QStringLiteral("Distinct dates (nodes)"), "avlNodes");
    statField(af, avlBox, QStringLiteral("Patients stored"), "avlValues");
    statField(af, avlBox, QStringLiteral("Height"), "avlHeight");
    statField(af, avlBox, QStringLiteral("Rotations LL / RR"), "avlSingle");
    statField(af, avlBox, QStringLiteral("Rotations LR / RL"), "avlDouble");
    statField(af, avlBox, QStringLiteral("Balanced (checked now)"), "avlValid");
    af->addRow(roleLabel(QStringLiteral("Several patients can share a date, so each node holds a list. "
                                        "Balance is restored after every insert and removal."), "hint", avlBox));
    row->addWidget(avlBox, 1);
    v->addLayout(row);
    v->addWidget(roleLabel(QStringLiteral("Figures update after every registration, undo and reload."), "hint", this));
    v->addStretch(1);

    connect(&hub_, &SessionHub::patientsChanged, this, &IndexStatsPanel::refresh);
    connect(&hub_, &SessionHub::reloaded, this, &IndexStatsPanel::refresh);
    refresh();
}

QString IndexStatsPanel::value(const char* objectName) const
{
    const QLabel* l = findChild<QLabel*>(QLatin1String(objectName));
    return l ? l->text() : QString();
}

void IndexStatsPanel::refresh()
{
    if (!hub_.isStarted())
        return;
    auto set = [this](const char* name, const QString& text) {
        if (QLabel* l = findChild<QLabel*>(QLatin1String(name)))
            l->setText(text);
    };
    const PatientRegistry& reg = hub_.session().patients();
    const vr_hash_stats st = reg.hashStats();
    set("hashEntries", QString::number(st.entries));
    set("hashBuckets", QString::number(st.buckets));
    set("hashUsed", QString::number(st.used_buckets));
    set("hashLoad", QString::number(st.load_factor, 'f', 2));
    set("hashLongest", QString::number(st.longest_chain));
    set("hashCollisions", QString::number(st.collisions));
    set("hashResizes", QString::number(st.resizes));
    const vr_avl& idx = reg.dateIndex();
    set("avlNodes", QString::number(idx.nodes));
    set("avlValues", QString::number(idx.values));
    set("avlHeight", QString::number(vr_avl_height(&idx)));
    set("avlSingle", QStringLiteral("%1 / %2").arg(idx.rot_ll).arg(idx.rot_rr));
    set("avlDouble", QStringLiteral("%1 / %2").arg(idx.rot_lr).arg(idx.rot_rl));
    set("avlValid", vr_avl_validate(&idx) ? QStringLiteral("Yes") : QStringLiteral("No"));
}
