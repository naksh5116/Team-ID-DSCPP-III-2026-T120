// Theme.hpp - the GUI's design tokens (see DESIGN.md).
//
// Every colour the GUI uses is named here. style.qss refers to the same
// names (@accent, @border, ...) and styleSheet() substitutes the values.
#ifndef VR_GUI_THEME_HPP
#define VR_GUI_THEME_HPP

#include <QColor>
#include <QFont>
#include <QPalette>
#include <QString>

class QTableView;

namespace Theme {

// Colour by token name ("accent", "border", ...). Unknown names give an
// invalid QColor.
QColor color(const char* token);

// The stylesheet from :/style.qss with every @token replaced.
QString styleSheet();

// A light palette built from the tokens, so widgets the stylesheet does not
// cover never pick up the desktop's dark mode.
QPalette palette();

// Applies VrStyle (Fusion with token-coloured indicators), the light
// palette and the stylesheet.
void apply();

// ESI levels 1-5.
QColor esiFill(int level);
QColor esiText(int level);
QString esiName(int level);          // "Resuscitation"
QString esiDescription(int level);   // what the level means at the triage desk

// A clear fixed-width font at the UI font's size (IDs, bed codes, amounts):
// Consolas / Cascadia Mono on Windows, DejaVu / Liberation Mono on Linux,
// else the system fixed font.
QFont monoFont();

// Applies the DESIGN.md table rules: compact rows, left-aligned header,
// no row numbers, horizontal rule only, whole-row selection, read-only.
void setupTable(QTableView* table);
// The UI font scaled by `factor`, optionally semibold (DESIGN.md type scale).
QFont scaledFont(double factor, bool semibold = false);

} // namespace Theme

#endif // VR_GUI_THEME_HPP
