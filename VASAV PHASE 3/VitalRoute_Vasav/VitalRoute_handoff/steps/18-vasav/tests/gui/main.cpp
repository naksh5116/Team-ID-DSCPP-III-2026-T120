// main.cpp - runs the GUI check groups (target vitalroute_gui_check).
//
//   vitalroute_gui_check [--shots DIR] [--only GROUP[,GROUP...]] [path/to/data]
//
//   --shots DIR   also save screenshots of the windows into DIR
//   --only ...    run only these groups (names: see GuiCheck.hpp)
//   --restart save DIR, then --restart load DIR
//                 save in the GUI, then reload in a new process (as T8)
//
// Run with QT_QPA_PLATFORM=offscreen to keep windows off the screen (ctest
// does). On Windows the offscreen platform draws text badly, so screenshot
// runs use the normal platform: windows then appear briefly.
#include "GuiCheck.hpp"

#include <functional>

namespace {

struct Group {
    const char* name;
    std::function<void(const QString& data, const QString& shots)> run;
};

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    Theme::apply();

    QStringList args = QApplication::arguments();
    args.removeFirst();
    auto takeOption = [&args](const QString& name) {
        const int i = static_cast<int>(args.indexOf(name));
        if (i < 0 || i + 1 >= args.size())
            return QString();
        const QString value = args.at(i + 1);
        args.remove(i, 2);
        return value;
    };
    // --restart save|load DIR: the two halves of the restart check, in two
    // separate processes on one data folder (see check_restart.cpp).
    const int r = static_cast<int>(args.indexOf(QStringLiteral("--restart")));
    if (r >= 0 && r + 2 < args.size()) {
        const QString mode = args.at(r + 1);
        const QString dir = args.at(r + 2);
        const int rc = mode == QStringLiteral("save") ? restartSave(dir) : restartLoad(dir);
        std::printf("\n%s (%d of %d checks passed)\n", rc == 0 ? "ALL GUI CHECKS PASSED" : "SOME GUI CHECKS FAILED",
                    checksRun() - checksFailed(), checksRun());
        return rc;
    }

    const QString shots = takeOption(QStringLiteral("--shots"));
    const QStringList only = takeOption(QStringLiteral("--only")).split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (!shots.isEmpty())
        QDir().mkpath(shots);
    const QString source = args.isEmpty() ? QStringLiteral(VR_SOURCE_DATA_DIR) : args.first();

    const Group groups[] = {
        {"hub", [](const QString& d, const QString&) { hubChecks(d); }},
        {"windows", windowChecks},
        {"nurse", nurseChecks},
        {"doctor", doctorChecks},
        {"patient", patientChecks},
        {"admin", adminChecks},
        {"map", mapChecks},
        {"screens", screenChecks},
        {"keyboard", keyboardChecks},
        {"sync", syncChecks},
    };

    // A misspelt group must not look like a clean run of nothing.
    for (const QString& name : only) {
        bool known = false;
        for (const Group& g : groups)
            known = known || name == QLatin1String(g.name);
        if (!known) {
            std::printf("[FAIL] unknown group '%s' (hub, windows, nurse, doctor, patient, admin, map, "
                        "screens, keyboard)\n", qPrintable(name));
            return 1;
        }
    }

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        std::printf("[FAIL] could not create a temporary folder\n");
        return 1;
    }
    for (const Group& g : groups) {
        if (!only.isEmpty() && !only.contains(QLatin1String(g.name)))
            continue;
        // Every group starts from the sample data, in its own folder.
        const QString data = tmp.path() + QLatin1Char('/') + QLatin1String(g.name) + QStringLiteral("/data");
        if (!copyDataDir(source, data)) {
            report(false, g.name, "could not copy " + str(source) + " to a temporary folder");
            continue;
        }
        try {
            g.run(data, shots);
        } catch (const std::exception& e) {
            report(false, g.name, std::string("unexpected exception: ") + e.what());
        }
    }

    const int failed = checksFailed();
    std::printf("\n%s (%d of %d checks passed%s)\n",
                failed == 0 ? "ALL GUI CHECKS PASSED" : "SOME GUI CHECKS FAILED",
                checksRun() - failed, checksRun(),
                checksSkipped() ? qPrintable(QStringLiteral(", %1 skipped").arg(checksSkipped())) : "");
    return failed == 0 ? 0 : 1;
}
