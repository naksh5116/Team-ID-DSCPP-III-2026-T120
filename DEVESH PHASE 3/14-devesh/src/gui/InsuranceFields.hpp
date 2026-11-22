// InsuranceFields.hpp - the insurance part of a form (same questions as the
// console: insurer, policy number, flat amount or percentage with a cap).
#ifndef VR_GUI_INSURANCE_FIELDS_HPP
#define VR_GUI_INSURANCE_FIELDS_HPP

#include "Insurance.hpp"

#include <QWidget>

#include <optional>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;

class InsuranceFields : public QWidget {
    Q_OBJECT
public:
    // `optional`: shows a "Has insurance" check box that enables the fields.
    explicit InsuranceFields(bool optional, QWidget* parent = nullptr);

    // The cover entered, nullopt when the box is unticked. Sets `error`
    // (and returns nullopt) when the entry is incomplete.
    std::optional<Insurance> value(QString* error) const;
    bool wanted() const;
    void clear();

private:
    void updateEnabled();

    QCheckBox* has_ = nullptr;
    QLineEdit* provider_ = nullptr;
    QLineEdit* policy_ = nullptr;
    QComboBox* kind_ = nullptr;
    QDoubleSpinBox* amount_ = nullptr;
    QDoubleSpinBox* percent_ = nullptr;
    QDoubleSpinBox* cap_ = nullptr;
    QLabel* amountLabel_ = nullptr;
    QLabel* percentLabel_ = nullptr;
    QLabel* capLabel_ = nullptr;
};

#endif // VR_GUI_INSURANCE_FIELDS_HPP
