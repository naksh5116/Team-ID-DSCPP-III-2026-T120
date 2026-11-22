// NurseWindow.cpp
#include "NurseWindow.hpp"

#include "RegistrationPanel.hpp"
#include "TransferDeskPanel.hpp"

#include <QTabWidget>

NurseWindow::NurseWindow(SessionHub& hub, QWidget* parent) : RoleWindow(hub, Role::Nurse, parent)
{
    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(false);
    registration_ = new RegistrationPanel(hub, *this, tabs_);
    transfers_ = new TransferDeskPanel(hub, *this, tabs_);
    tabs_->addTab(registration_, QStringLiteral("Registration and waiting lists"));
    tabs_->addTab(transfers_, QStringLiteral("Transfer desk"));
    setWorkArea(tabs_);
}
