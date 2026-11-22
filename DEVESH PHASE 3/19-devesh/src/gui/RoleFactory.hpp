// RoleFactory.hpp - creates the window for a role.
#ifndef VR_GUI_ROLE_FACTORY_HPP
#define VR_GUI_ROLE_FACTORY_HPP

#include "Roles.hpp"

class RoleWindow;
class SessionHub;

// A new, not yet shown, top-level window for `role` on the shared hub.
RoleWindow* createRoleWindow(Role role, SessionHub& hub);

#endif // VR_GUI_ROLE_FACTORY_HPP
