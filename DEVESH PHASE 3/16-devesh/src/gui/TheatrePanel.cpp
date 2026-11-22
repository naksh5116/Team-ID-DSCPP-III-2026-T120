// TheatrePanel.cpp
#include "TheatrePanel.hpp"

#include "RoleWindow.hpp"
#include "SessionHub.hpp"
#include "StaffTheatreModels.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QComboBox>
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
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

using Text::qs;
using Text::ss;

TheatrePanel::TheatrePanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent), hub_(hub), host_(host)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    // ---- booking form
    auto* left = new QWidget;
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(8, 8, 8, 8);
    lv->setSpacing(12);
    auto* box = new QGroupBox(QStringLiteral("Book the next free slot"), left);
    auto* form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);
    theatre_ = new QComboBox(box);
    theatre_->setObjectName(QStringLiteral("theatreName"));
    patient_ = new QLineEdit(box);
    patient_->setObjectName(QStringLiteral("theatrePatient"));
    patient_->setFont(Theme::monoFont());
    patient_->setPlaceholderText(QStringLiteral("VR-2026-000004"));
    procedure_ = new QLineEdit(box);
    procedure_->setObjectName(QStringLiteral("theatreProcedure"));
    fee_ = new QDoubleSpinBox(box);
    fee_->setObjectName(QStringLiteral("theatreFee"));
    fee_->setRange(0.0, 10000000.0);
    fee_->setPrefix(QStringLiteral("Rs "));
    form->addRow(QStringLiteral("Theatre"), theatre_);
    form->addRow(QStringLiteral("Patient ID"), patient_);
    form->addRow(QStringLiteral("Procedure"), procedure_);
    form->addRow(QStringLiteral("Surgeon's fee"), fee_);
    lv->addWidget(box);
    auto* bookButton = new QPushButton(QStringLiteral("Book next free slot"), left);
    bookButton->setObjectName(QStringLiteral("theatreBook"));
    bookButton->setProperty("primary", true);
    connect(bookButton, &QPushButton::clicked, this, [this] { book(); });
    auto* bookRow = new QHBoxLayout;
    bookRow->addWidget(bookButton);
    bookRow->addStretch(1);
    lv->addLayout(bookRow);
    lv->addWidget(roleLabel(QStringLiteral(
        "Booking charges the surgery to the patient's bill: surgeon's fee, Rs 8000.00 theatre fee "
        "and Rs 3500.00 consumables."), "hint", left));
    lv->addStretch(1);
    for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto* sc = new QShortcut(QKeySequence(key), left);
        sc->setContext(Qt::WidgetWithChildrenShortcut);
        connect(sc, &QShortcut::activated, this, [this] { book(); });
    }
    split->addWidget(scrollable(left, split));

    // ---- schedule
    auto* right = new QWidget;
    auto* rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);
    model_ = new TheatreModel(hub_, this);
    table_ = new QTableView;
    table_->setObjectName(QStringLiteral("theatreTable"));
    table_->setModel(model_);
    Theme::setupTable(table_);
    fitColumnsOnReset(table_);
    auto* section = tableSection(QStringLiteral("Today's theatre schedule"),
                                 QStringLiteral("Free slots wait in a circular queue per theatre. A released "
                                                "slot joins the back of the ring and can be booked again."),
                                 table_, right);
    rings_ = new QLabel(section);
    rings_->setObjectName(QStringLiteral("theatreRings"));
    rings_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    static_cast<QVBoxLayout*>(section->layout())->insertWidget(2, rings_);
    rv->addWidget(section, 1);
    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(8, 0, 8, 8);
    auto* release = new QPushButton(QStringLiteral("Release selected slot"), right);
    release->setObjectName(QStringLiteral("theatreRelease"));
    connect(release, &QPushButton::clicked, this, [this] { releaseSelected(); });
    actions->addWidget(release);
    actions->addStretch(1);
    rv->addLayout(actions);
    split->addWidget(right);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);

    connect(model_, &QAbstractItemModel::modelReset, this, &TheatrePanel::refreshTheatres);
    refreshTheatres();
}

void TheatrePanel::refreshTheatres()
{
    if (!hub_.isStarted())
        return;
    const QList<TheatreDay> days = readTheatreDays(hub_.session().theatres());
    const QString current = theatre_->currentText();
    theatre_->clear();
    QStringList lines;
    for (const TheatreDay& d : days) {
        theatre_->addItem(d.name);
        QStringList order;
        for (int n : d.ringOrder)
            order << QString::number(n);
        lines << QStringLiteral("%1: %2 of %3 slots free. Next free in cycle order: %4")
                     .arg(d.name).arg(d.freeCount).arg(d.slots.size())
                     .arg(order.isEmpty() ? QStringLiteral("none (fully booked)") : order.join(QLatin1Char(' ')));
    }
    rings_->setText(lines.join(QLatin1Char('\n')));
    const int keep = theatre_->findText(current);
    if (keep >= 0)
        theatre_->setCurrentIndex(keep);
}

QString TheatrePanel::ringText() const
{
    return rings_->text();
}

bool TheatrePanel::book()
{
    host_.showMessage(QString());
    const QString theatre = theatre_->currentText();
    if (theatre.isEmpty()) {
        host_.showError(QStringLiteral("No operation theatres are configured."));
        return false;
    }
    if (patient_->text().trimmed().isEmpty()) {
        host_.showError(QStringLiteral("Enter the patient ID."));
        patient_->setFocus();
        return false;
    }
    if (procedure_->text().trimmed().isEmpty()) {
        host_.showError(QStringLiteral("Enter the procedure."));
        procedure_->setFocus();
        return false;
    }
    QString key;
    const bool ok = host_.attempt([&] {
        const HospitalSession& s = hub_.session();
        const Patient& p = s.findPatient(ss(patient_->text()));
        const OTScheduler::Booking b = hub_.bookTheatre(ss(theatre), p.id(), ss(procedure_->text()), fee_->value());
        key = theatre + QLatin1Char('|') + QString::number(b.slot);
        // The console's option 7 sentence.
        host_.showMessage(QStringLiteral("Booked %1 slot %2, %3 for %4.")
                              .arg(theatre).arg(b.slot + 1)
                              .arg(qs(s.theatres().slotTime(ss(theatre), b.slot)), qs(p.name())));
    });
    if (!ok)
        return false;
    patient_->clear();
    procedure_->clear();
    fee_->setValue(0.0);
    selectKey(table_, key);
    return true;
}

bool TheatrePanel::releaseSelected()
{
    host_.showMessage(QString());
    const QModelIndex current = table_->currentIndex();
    if (!current.isValid()) {
        host_.showError(QStringLiteral("Select a booked slot in the schedule first."));
        return false;
    }
    const QString key = model_->keyAt(current.row());
    const QString theatre = TheatreModel::theatreOf(key);
    const int slot = TheatreModel::slotOf(key);
    return host_.attempt([&] {
        hub_.releaseTheatreSlot(ss(theatre), slot);
        host_.showMessage(QStringLiteral("Released %1 slot %2; it is back in the free-slot ring.")
                              .arg(theatre).arg(slot + 1));
    });
}
