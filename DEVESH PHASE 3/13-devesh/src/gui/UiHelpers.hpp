// UiHelpers.hpp - small layout helpers shared by the role screens.
#ifndef VR_GUI_UI_HELPERS_HPP
#define VR_GUI_UI_HELPERS_HPP

#include <QString>

class QLabel;
class QLayout;
class QScrollArea;
class QTableView;
class QWidget;

// A heading, an optional hint line and a table, stacked.
QWidget* tableSection(const QString& heading, const QString& hint, QTableView* table,
                      QWidget* parent);
// Keeps the table's columns fitted to their contents after each refresh.
void fitColumnsOnReset(QTableView* table);
// Selects the row whose key (SnapshotTableModel::KeyRole) is `key`.
void selectKey(QTableView* table, const QString& key);
// Wraps a form so it scrolls instead of squeezing when the window is small.
QScrollArea* scrollable(QWidget* content, QWidget* parent);
// A layout holding `button` at its natural width, left-aligned (for the
// field column of a QFormLayout, which would otherwise stretch it).
QLayout* leftAligned(QWidget* button);
// Label styled by DESIGN.md text role: "heading" or "hint".
QLabel* roleLabel(const QString& text, const char* textRole, QWidget* parent);

// The modal confirmation used for destructive actions only (discharge,
// undo, reload). Names the action on the accept button. Returns true when
// the user accepts.
bool confirm(QWidget* parent, const QString& title, const QString& text, const QString& acceptLabel);
// For the automated GUI checks only: answer every confirm() with "accept"
// without showing a dialog. The application never calls this.
void setAutoConfirmForChecks(bool on);

#endif // VR_GUI_UI_HELPERS_HPP
