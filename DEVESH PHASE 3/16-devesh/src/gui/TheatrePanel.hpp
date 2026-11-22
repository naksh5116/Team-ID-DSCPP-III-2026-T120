// TheatrePanel.hpp - console option 7: operation theatre slots.
//
// Each theatre keeps the numbers of its free slots in a circular queue:
// booking takes the next free slot in cycle order, releasing puts the slot
// back so it can be booked again at once. Used by Doctor and Super Admin.
#ifndef VR_GUI_THEATRE_PANEL_HPP
#define VR_GUI_THEATRE_PANEL_HPP

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QTableView;
class RoleWindow;
class SessionHub;
class TheatreModel;

class TheatrePanel : public QWidget {
    Q_OBJECT
public:
    TheatrePanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    bool book();
    bool releaseSelected();
    QTableView* table() const { return table_; }
    TheatreModel* model() const { return model_; }
    QString ringText() const;

private:
    void refreshTheatres();

    SessionHub& hub_;
    RoleWindow& host_;
    QComboBox* theatre_ = nullptr;
    QLineEdit* patient_ = nullptr;
    QLineEdit* procedure_ = nullptr;
    QDoubleSpinBox* fee_ = nullptr;
    QLabel* rings_ = nullptr;
    TheatreModel* model_ = nullptr;
    QTableView* table_ = nullptr;
};

#endif // VR_GUI_THEATRE_PANEL_HPP
