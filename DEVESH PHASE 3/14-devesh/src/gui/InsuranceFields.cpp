// InsuranceFields.cpp
#include "InsuranceFields.hpp"

#include "Text.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

InsuranceFields::InsuranceFields(bool optional, QWidget* parent) : QWidget(parent)
{
    auto* form = new QFormLayout(this);
    form->setContentsMargins(0, 0, 0, 0);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);

    if (optional) {
        has_ = new QCheckBox(QStringLiteral("Patient has insurance"), this);
        has_->setObjectName(QStringLiteral("insuranceCheck"));
        form->addRow(QString(), has_);
        connect(has_, &QCheckBox::toggled, this, &InsuranceFields::updateEnabled);
    }

    provider_ = new QLineEdit(this);
    provider_->setObjectName(QStringLiteral("insuranceProvider"));
    form->addRow(QStringLiteral("Insurer"), provider_);

    policy_ = new QLineEdit(this);
    policy_->setObjectName(QStringLiteral("insurancePolicy"));
    form->addRow(QStringLiteral("Policy number"), policy_);

    kind_ = new QComboBox(this);
    kind_->setObjectName(QStringLiteral("insuranceKind"));
    kind_->addItems({QStringLiteral("Flat amount"), QStringLiteral("Percentage of the bill")});
    form->addRow(QStringLiteral("Cover type"), kind_);
    connect(kind_, &QComboBox::currentIndexChanged, this, &InsuranceFields::updateEnabled);

    amount_ = new QDoubleSpinBox(this);
    amount_->setObjectName(QStringLiteral("insuranceAmount"));
    amount_->setRange(0.0, 10000000.0);
    amount_->setDecimals(2);
    amount_->setPrefix(QStringLiteral("Rs "));
    amountLabel_ = new QLabel(QStringLiteral("Amount covered"), this);
    form->addRow(amountLabel_, amount_);

    percent_ = new QDoubleSpinBox(this);
    percent_->setObjectName(QStringLiteral("insurancePercent"));
    percent_->setRange(0.0, 100.0);
    percent_->setDecimals(1);
    percent_->setSuffix(QStringLiteral(" %"));
    percentLabel_ = new QLabel(QStringLiteral("Percentage covered"), this);
    form->addRow(percentLabel_, percent_);

    cap_ = new QDoubleSpinBox(this);
    cap_->setObjectName(QStringLiteral("insuranceCap"));
    cap_->setRange(0.0, 10000000.0);
    cap_->setDecimals(2);
    cap_->setPrefix(QStringLiteral("Rs "));
    cap_->setSpecialValueText(QStringLiteral("No limit"));
    capLabel_ = new QLabel(QStringLiteral("Maximum payout"), this);
    form->addRow(capLabel_, cap_);

    updateEnabled();
}

bool InsuranceFields::wanted() const
{
    return !has_ || has_->isChecked();
}

void InsuranceFields::updateEnabled()
{
    const bool on = wanted();
    const bool flat = kind_->currentIndex() == 0;
    for (QWidget* w : {static_cast<QWidget*>(provider_), static_cast<QWidget*>(policy_),
                       static_cast<QWidget*>(kind_)})
        w->setEnabled(on);
    amount_->setVisible(flat);
    amountLabel_->setVisible(flat);
    percent_->setVisible(!flat);
    percentLabel_->setVisible(!flat);
    cap_->setVisible(!flat);
    capLabel_->setVisible(!flat);
    amount_->setEnabled(on);
    percent_->setEnabled(on);
    cap_->setEnabled(on);
}

std::optional<Insurance> InsuranceFields::value(QString* error) const
{
    if (!wanted())
        return std::nullopt;
    const QString provider = provider_->text().trimmed();
    if (provider.isEmpty()) {
        if (error)
            *error = QStringLiteral("Enter the insurer, or untick \"Patient has insurance\".");
        return std::nullopt;
    }
    const bool flat = kind_->currentIndex() == 0;
    return Insurance(Text::ss(provider), Text::ss(policy_->text()),
                     flat ? Insurance::Kind::Flat : Insurance::Kind::Percentage,
                     flat ? amount_->value() : percent_->value(), flat ? 0.0 : cap_->value());
}

void InsuranceFields::clear()
{
    if (has_)
        has_->setChecked(false);
    provider_->clear();
    policy_->clear();
    kind_->setCurrentIndex(0);
    amount_->setValue(0.0);
    percent_->setValue(0.0);
    cap_->setValue(0.0);
    updateEnabled();
}
