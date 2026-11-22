// GuiCheck.hpp - shared helpers for the automated GUI checks (not shipped).
//
// The check program (target vitalroute_gui_check) drives the real windows,
// panels and SessionHub of the GUI and prints one line per check:
//   [PASS] / [FAIL] / [SKIP]  <id>  <what is checked> - <observed detail>
// Each group runs on its own temporary copy of data/, so real records are
// never touched and groups cannot affect each other.
//
//   group      file                 what it covers
//   hub        check_hub.cpp        SessionHub: which signals each action emits
//   windows    check_windows.cpp    Launcher, shared window frame, stylesheet
//   nurse      check_nurse.cpp      registration, waiting lines, transfer desk
//   doctor     check_doctor.cpp     treatment and route, record, theatres, availability
//   patient    check_patient.cpp    read-only patient view, routeAssigned
//   admin      check_admin.cpp      billing, discharge, undo, admissions, indexes, save/reload
//   map        check_map.cpp        floor plan drawing, step-to-segment mapping, metres to pixels
//   screens    check_screens.cpp    screen placement rules, F11 wall display
//   keyboard   check_keyboard.cpp   Enter submits, Tab order, Ctrl+S, F5, F11 as key presses
//   sync       check_sync.cpp       one check per SessionHub signal, across two windows
//   (restart)  check_restart.cpp    --restart save / load: two processes, as test T8
//
// Only Qt Core, Gui and Widgets are used (no Qt Test).
#ifndef VR_GUI_CHECK_HPP
#define VR_GUI_CHECK_HPP

#include "AdminModels.hpp"
#include "AdminPanels.hpp"
#include "AdminWindow.hpp"
#include "BillView.hpp"
#include "BillingPanel.hpp"
#include "DoctorAvailabilityPanel.hpp"
#include "DoctorWindow.hpp"
#include "EsiDelegate.hpp"
#include "FloorPlanWidget.hpp"
#include "Launcher.hpp"
#include "NurseWindow.hpp"
#include "PatientLookupPanel.hpp"
#include "PatientRecordView.hpp"
#include "PatientWindow.hpp"
#include "QueueModels.hpp"
#include "RegistrationForm.hpp"
#include "RegistrationPanel.hpp"
#include "RoleWindow.hpp"
#include "RoutePanel.hpp"
#include "RouteSegments.hpp"
#include "RouteStepsView.hpp"
#include "ScreenArranger.hpp"
#include "SessionHub.hpp"
#include "StaffTheatreModels.hpp"
#include "Text.hpp"
#include "TheatrePanel.hpp"
#include "Theme.hpp"
#include "TransferDeskPanel.hpp"
#include "TreatPanel.hpp"
#include "UiHelpers.hpp"

#include "Exceptions.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMap>
#include <QMenuBar>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScreen>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextDocumentFragment>

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

// ---------------------------------------------------------- reporting --

void report(bool ok, const char* name, const std::string& detail = "");
void skip(const char* name, const std::string& reason);
void section(const char* title);
int checksRun();
int checksFailed();
int checksSkipped();

// ------------------------------------------------------------ helpers --

std::string str(const QString& s);
// Copies the three data files into `to` (with an empty records/ folder).
bool copyDataDir(const QString& from, const QString& to);
// Plain text of a child QLabel (rich text stripped).
QString labelText(QWidget* w, const char* name);
// Saves w as DIR/file when DIR is not empty.
void shoot(QWidget* w, const QString& dir, const char* file);
QStringList keysOf(const SnapshotTableModel* m);
// Fills a registration form's name, age and complaint ("er" or "opd" prefix).
void fillPatient(QWidget* w, const char* prefix, const QString& name, int age, const QString& complaint);

template <typename T>
T* child(QWidget* parent, const char* name)
{
    return parent->findChild<T*>(QLatin1String(name));
}

// ------------------------------------------------------------- groups --

void hubChecks(const QString& dataDir);
void windowChecks(const QString& dataDir, const QString& shots);
void nurseChecks(const QString& dataDir, const QString& shots);
void doctorChecks(const QString& dataDir, const QString& shots);
void patientChecks(const QString& dataDir, const QString& shots);
void adminChecks(const QString& dataDir, const QString& shots);
void mapChecks(const QString& dataDir, const QString& shots);
void screenChecks(const QString& dataDir, const QString& shots);
void keyboardChecks(const QString& dataDir, const QString& shots);
void syncChecks(const QString& dataDir, const QString& shots);

// The restart check: two runs of the program on one folder.
int restartSave(const QString& dataDir);
int restartLoad(const QString& dataDir);

#endif // VR_GUI_CHECK_HPP
