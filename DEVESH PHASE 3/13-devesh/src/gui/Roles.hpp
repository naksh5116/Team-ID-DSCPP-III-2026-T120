// Roles.hpp - the four role windows of the prototype.
//
// A role only decides which screen opens. It is not a login: there are no
// accounts or passwords, and any role can be opened from the Launcher.
#ifndef VR_GUI_ROLES_HPP
#define VR_GUI_ROLES_HPP

#include <QString>

enum class Role { Nurse, Doctor, Patient, SuperAdmin };

inline QString roleName(Role r)
{
    switch (r) {
    case Role::Nurse: return QStringLiteral("Nurse");
    case Role::Doctor: return QStringLiteral("Doctor");
    case Role::Patient: return QStringLiteral("Patient");
    case Role::SuperAdmin: return QStringLiteral("Super Admin");
    }
    return QString();
}

// Launcher buttons, and the "Switch role" menu, in this order.
inline const Role kLauncherOrder[] = {Role::Doctor, Role::Nurse, Role::Patient, Role::SuperAdmin};

#endif // VR_GUI_ROLES_HPP
