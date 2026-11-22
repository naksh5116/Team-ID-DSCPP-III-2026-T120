// RoleFactory.cpp - one window class per role.
#include "RoleFactory.hpp"

#include "AdminWindow.hpp"
#include "DoctorWindow.hpp"
#include "NurseWindow.hpp"
#include "PatientWindow.hpp"

RoleWindow* createRoleWindow(Role role, SessionHub& hub)
{
    switch (role) {
    case Role::Nurse:
        return new NurseWindow(hub);
    case Role::Doctor:
        return new DoctorWindow(hub);
    case Role::Patient:
        return new PatientWindow(hub);
    case Role::SuperAdmin:
        return new AdminWindow(hub);
    }
    return new NurseWindow(hub);   // unreachable: every Role is handled above
}
