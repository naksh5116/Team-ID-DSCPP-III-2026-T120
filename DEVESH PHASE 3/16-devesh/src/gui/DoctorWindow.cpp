// DoctorWindow.cpp
#include "DoctorWindow.hpp"

#include "DoctorAvailabilityPanel.hpp"
#include "PatientLookupPanel.hpp"
#include "TheatrePanel.hpp"
#include "TreatPanel.hpp"

#include <QTabWidget>

DoctorWindow::DoctorWindow(SessionHub& hub, QWidget* parent) : RoleWindow(hub, Role::Doctor, parent)
{
    tabs_ = new QTabWidget(this);
    treat_ = new TreatPanel(hub, *this, tabs_);
    lookup_ = new PatientLookupPanel(hub, *this, tabs_);
    theatres_ = new TheatrePanel(hub, *this, tabs_);
    availability_ = new DoctorAvailabilityPanel(hub, *this, tabs_);
    tabs_->addTab(treat_, QStringLiteral("Treatment"));
    tabs_->addTab(lookup_, QStringLiteral("Patient record"));
    tabs_->addTab(theatres_, QStringLiteral("Operation theatres"));
    tabs_->addTab(availability_, QStringLiteral("Doctors on duty"));
    setWorkArea(tabs_);
}
