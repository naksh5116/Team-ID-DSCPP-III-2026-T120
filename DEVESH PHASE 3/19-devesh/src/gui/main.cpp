// main.cpp - VitalRoute GUI entry point.
//
// One process, one SessionHub (one HospitalSession), many top-level windows.
// The Launcher opens role windows; closing a role window never quits, only
// closing the Launcher does.
#include "Launcher.hpp"
#include "SessionHub.hpp"
#include "Text.hpp"
#include "Theme.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QStringList>

#include <exception>

namespace {

// Same rule as the console (first argument, then ./data, ../data), plus the
// folders next to the executable and the source tree, so the GUI also starts
// from build-gui/ or by double-click.
QString findDataDir(const QStringList& args)
{
    if (args.size() > 1)
        return args.at(1);
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QStringLiteral("data"), QStringLiteral("../data"),
        appDir + QStringLiteral("/data"), appDir + QStringLiteral("/../data"),
        QStringLiteral(VR_SOURCE_DATA_DIR)};
    for (const QString& dir : candidates)
        if (QFileInfo::exists(dir + QStringLiteral("/hospital_graph.txt")))
            return QDir::cleanPath(dir);
    return QStringLiteral("data");
}

} // namespace

int main(int argc, char** argv)
{
    // Fractional scale factors (125 %, 150 %) are used as they are, so text
    // and lines stay sharp on every monitor. This is Qt 6's default; set
    // explicitly because the multi-monitor demo depends on it.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("VitalRoute"));
    QApplication::setQuitOnLastWindowClosed(false);
    Theme::apply();

    SessionHub hub(findDataDir(QApplication::arguments()));
    try {
        hub.start();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, QStringLiteral("VitalRoute"),
                              QStringLiteral("VitalRoute could not start: %1")
                                  .arg(Text::qs(e.what())));
        return 1;
    }

    Launcher launcher(hub);
    launcher.show();
    return app.exec();
}
