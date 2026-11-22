// AdminPanels.hpp - the Super Admin's read-mostly panels.
//
//   AdmissionsPanel   console option 5: registrations between two dates
//                     (AVL tree range walk) with the tree's figures
//   StaffPanel        staff roster plus the next-available-doctor search
//   IndexStatsPanel   live figures of the two patient indexes: the hash
//                     table (entries, buckets, load, collisions, resizes)
//                     and the AVL tree (height, rotations, balance check)
#ifndef VR_GUI_ADMIN_PANELS_HPP
#define VR_GUI_ADMIN_PANELS_HPP

#include <QWidget>

class AdmissionsModel;
class QDateEdit;
class QLabel;
class QTableView;
class RoleWindow;
class SessionHub;
class StaffRosterModel;

class AdmissionsPanel : public QWidget {
    Q_OBJECT
public:
    AdmissionsPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);

    void showAdmissions(int fromYmd, int toYmd);
    void showRange();   // the dates in the two fields
    AdmissionsModel* model() const { return model_; }
    QString summary() const;
    QString avlText() const;

private:
    void refreshTexts();

    SessionHub& hub_;
    RoleWindow& host_;
    QDateEdit* from_ = nullptr;
    QDateEdit* to_ = nullptr;
    QLabel* summary_ = nullptr;
    QLabel* avl_ = nullptr;
    AdmissionsModel* model_ = nullptr;
    QTableView* table_ = nullptr;
    int shownFrom_ = 0;
    int shownTo_ = 0;
};

class StaffPanel : public QWidget {
    Q_OBJECT
public:
    StaffPanel(SessionHub& hub, RoleWindow& host, QWidget* parent = nullptr);
    StaffRosterModel* roster() const { return roster_; }

private:
    StaffRosterModel* roster_ = nullptr;
};

class IndexStatsPanel : public QWidget {
    Q_OBJECT
public:
    explicit IndexStatsPanel(SessionHub& hub, QWidget* parent = nullptr);
    QString value(const char* objectName) const;

private:
    void refresh();
    SessionHub& hub_;
};

#endif // VR_GUI_ADMIN_PANELS_HPP
