// RouteStepsView.hpp - the Navigator's directions as a numbered list.
//
// Shows "Central Emergency Room to ICU-1, Bed 1", one row per step of
// Route::steps, and the distance and walking time. Selecting a step emits
// stepSelected(index) - the route map (piece 9) highlights that segment.
#ifndef VR_GUI_ROUTE_STEPS_VIEW_HPP
#define VR_GUI_ROUTE_STEPS_VIEW_HPP

#include "Navigator.hpp"

#include <QWidget>

class QLabel;
class QListView;
class QStringListModel;

class RouteStepsView : public QWidget {
    Q_OBJECT
public:
    explicit RouteStepsView(QWidget* parent = nullptr);

    void setRoute(const Route& route);
    void clear(const QString& note = QString());
    // Larger type for the wall display (factor of the UI font).
    void setTextScale(double factor);

    int stepCount() const;
    QString stepText(int index) const;
    QString title() const;
    QString summary() const;
    QListView* list() const { return list_; }

Q_SIGNALS:
    void stepSelected(int index);   // -1 when the selection is cleared

private:
    QLabel* title_ = nullptr;
    QListView* list_ = nullptr;
    QStringListModel* model_ = nullptr;
    QLabel* summary_ = nullptr;
};

#endif // VR_GUI_ROUTE_STEPS_VIEW_HPP
