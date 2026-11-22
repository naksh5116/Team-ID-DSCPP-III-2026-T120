// RegistrationPanel.cpp
#include "RegistrationPanel.hpp"

#include "EsiDelegate.hpp"
#include "QueueModels.hpp"
#include "RegistrationForm.hpp"
#include "Theme.hpp"
#include "UiHelpers.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>

RegistrationPanel::RegistrationPanel(SessionHub& hub, RoleWindow& host, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setChildrenCollapsible(false);
    layout->addWidget(split);

    // Left: the two registration forms.
    auto* forms = new QTabWidget(split);
    forms->setObjectName(QStringLiteral("registrationTabs"));
    emergencyForm_ = new RegistrationForm(hub, host, RegistrationForm::Kind::Emergency);
    opdForm_ = new RegistrationForm(hub, host, RegistrationForm::Kind::Outpatient);
    forms->addTab(scrollable(emergencyForm_, forms), QStringLiteral("Emergency patient"));
    forms->addTab(scrollable(opdForm_, forms), QStringLiteral("Outpatient (OPD)"));

    // Right: the waiting lines, read from the triage unit.
    auto* lines = new QSplitter(Qt::Vertical, split);
    lines->setChildrenCollapsible(false);

    emergencyModel_ = new EmergencyQueueModel(hub, this);
    emergencyTable_ = new QTableView;
    emergencyTable_->setObjectName(QStringLiteral("emergencyQueue"));
    emergencyTable_->setModel(emergencyModel_);
    emergencyTable_->setItemDelegateForColumn(EmergencyQueueModel::Esi, new EsiDelegate(emergencyTable_));
    Theme::setupTable(emergencyTable_);
    fitColumnsOnReset(emergencyTable_);
    lines->addWidget(tableSection(
        QStringLiteral("Emergency queue, most urgent first"),
        QStringLiteral("Order from the triage heap: lower ESI level first, then earlier arrival."),
        emergencyTable_, lines));

    opdModel_ = new OpdQueueModel(hub, this);
    opdTable_ = new QTableView;
    opdTable_->setObjectName(QStringLiteral("opdQueue"));
    opdTable_->setModel(opdModel_);
    Theme::setupTable(opdTable_);
    fitColumnsOnReset(opdTable_);
    lines->addWidget(tableSection(
        QStringLiteral("OPD line, first come first served"),
        QStringLiteral("Outpatients are seen in arrival order once no emergency is waiting."),
        opdTable_, lines));
    lines->setStretchFactor(0, 3);
    lines->setStretchFactor(1, 2);

    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);

    connect(emergencyForm_, &RegistrationForm::registered, this,
            [this](const QString& id) { selectKey(emergencyTable_, id); });
    connect(opdForm_, &RegistrationForm::registered, this,
            [this](const QString& id) { selectKey(opdTable_, id); });
}
