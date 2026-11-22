// BillView.cpp
#include "BillView.hpp"

#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

using Text::qs;

namespace {

QString categoryText(const std::string& category)
{
    // The model's categories are upper case (BED, MEDICINE, ...).
    QString c = qs(category).toLower();
    if (!c.isEmpty())
        c[0] = c[0].toUpper();
    return c;
}

} // namespace

// ---------------------------------------------------------- bill lines --

BillLinesModel::BillLinesModel(SessionHub& hub, QObject* parent)
    : SnapshotTableModel(hub,
                         {QStringLiteral("Date"), QStringLiteral("Type"), QStringLiteral("Description"),
                          QStringLiteral("Calculation"), QStringLiteral("Amount")},
                         parent)
{
    refreshOn(&SessionHub::billsChanged);
}

void BillLinesModel::setPatientId(const QString& id)
{
    id_ = id;
    refresh();
}

void BillLinesModel::build(QList<Row>& rows, QStringList& keys) const
{
    if (id_.isEmpty())
        return;
    const Bill* b = hub().session().findBill(Text::ss(id_));
    if (!b)
        return;
    for (std::size_t i = 0; i < b->chargeCount(); ++i) {
        const Charge& c = b->charge(i);   // polymorphic: priced by calculateCost()
        rows.append({text(Text::date(c.date())), text(categoryText(c.category())),
                     text(qs(c.description())), text(qs(c.detail())),
                     amount(Text::money(c.calculateCost()))});
        keys.append(QString::number(i));
    }
}

// ---------------------------------------------------------------- view --

BillView::BillView(SessionHub& hub, QWidget* parent) : QWidget(parent), hub_(hub)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);

    header_ = new QLabel(this);
    header_->setObjectName(QStringLiteral("billHeader"));
    header_->setProperty("textRole", QStringLiteral("heading"));
    header_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(header_);

    lines_ = new BillLinesModel(hub_, this);
    table_ = new QTableView(this);
    table_->setObjectName(QStringLiteral("billLines"));
    table_->setModel(lines_);
    Theme::setupTable(table_);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(BillLinesModel::Calculation, QHeaderView::Stretch);
    connect(lines_, &QAbstractItemModel::modelReset, table_, [this] {
        for (int c : {BillLinesModel::Date, BillLinesModel::Type, BillLinesModel::Description,
                      BillLinesModel::Amount})
            table_->resizeColumnToContents(c);
    });
    v->addWidget(table_, 1);

    totals_ = new QWidget(this);
    QWidget* totals = totals_;
    auto* form = new QFormLayout(totals);
    form->setContentsMargins(0, 4, 0, 0);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(2);
    subtotal_ = new QLabel(totals);
    subtotal_->setObjectName(QStringLiteral("billSubtotal"));
    subtotal_->setFont(Theme::monoFont());
    insurance_ = new QLabel(totals);
    insurance_->setObjectName(QStringLiteral("billInsurance"));
    insurance_->setWordWrap(true);
    total_ = new QLabel(totals);
    total_->setObjectName(QStringLiteral("billTotal"));
    QFont totalFont = Theme::scaledFont(1.0, true);   // a sentence, so not monospace
    total_->setFont(totalFont);
    form->addRow(QStringLiteral("Subtotal"), subtotal_);
    form->addRow(QStringLiteral("Insurance"), insurance_);
    form->addRow(QStringLiteral("Total"), total_);
    v->addWidget(totals);

    connect(&hub_, &SessionHub::billsChanged, this, &BillView::refresh);
    connect(&hub_, &SessionHub::reloaded, this, &BillView::refresh);
    refresh();
}

void BillView::setPatientId(const QString& id)
{
    id_ = id.trimmed();
    lines_->setPatientId(id_);
    refresh();
}

void BillView::refresh()
{
    const Bill* b = (!id_.isEmpty() && hub_.isStarted()) ? hub_.session().findBill(Text::ss(id_)) : nullptr;
    if (!b) {
        totals_->setVisible(false);   // nothing to total until a bill is open
        header_->setText(QStringLiteral("No bill to show"));
        subtotal_->clear();
        insurance_->clear();
        total_->clear();
        return;
    }
    totals_->setVisible(true);
    header_->setText(QStringLiteral("Bill %1, opened %2, %3")
                         .arg(qs(b->billId()), Text::date(b->openedOn()),
                              b->settled() ? QStringLiteral("settled on %1").arg(Text::date(b->settledOn()))
                                           : QStringLiteral("open")));
    subtotal_->setText(Text::money(b->subtotal()));
    if (const Insurance* ins = b->insurance()) {
        if (b->settled())
            insurance_->setText(QStringLiteral("%1. Deducted: %2")
                                    .arg(qs(ins->summary()), Text::money(b->insuranceDeduction())));
        else
            insurance_->setText(QStringLiteral("%1. Deducted once, at discharge: about %2")
                                    .arg(qs(ins->summary()), Text::money(b->subtotal() - b->projectedTotal())));
    } else {
        insurance_->setText(QStringLiteral("None recorded"));
    }
    total_->setText(b->settled() ? QStringLiteral("Paid %1").arg(Text::money(b->total()))
                                 : QStringLiteral("Due %1 (estimate until discharge)").arg(Text::money(b->projectedTotal())));
}

QString BillView::headerText() const
{
    return header_->text();
}

QString BillView::totalText() const
{
    return total_->text();
}
