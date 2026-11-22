// support.cpp - reporting and small helpers for the GUI checks.
#include "GuiCheck.hpp"

namespace {
int g_checks = 0;
int g_failures = 0;
int g_skipped = 0;
}

void report(bool ok, const char* name, const std::string& detail)
{
    ++g_checks;
    if (!ok)
        ++g_failures;
    std::printf("[%s] %s%s%s\n", ok ? "PASS" : "FAIL", name, detail.empty() ? "" : " - ", detail.c_str());
    std::fflush(stdout);
}

void skip(const char* name, const std::string& reason)
{
    ++g_skipped;
    std::printf("[SKIP] %s - %s\n", name, reason.c_str());
    std::fflush(stdout);
}

void section(const char* title)
{
    std::printf("\n-- %s --\n", title);
    std::fflush(stdout);
}

int checksRun() { return g_checks; }
int checksFailed() { return g_failures; }
int checksSkipped() { return g_skipped; }

std::string str(const QString& s)
{
    return s.toStdString();
}

bool copyDataDir(const QString& from, const QString& to)
{
    QDir().mkpath(to + QStringLiteral("/records"));
    for (const char* f : {"hospital_graph.txt", "resources.txt", "doctors.txt"}) {
        const QString name = QLatin1String(f);
        if (!QFile::copy(from + QLatin1Char('/') + name, to + QLatin1Char('/') + name))
            return false;
    }
    return true;
}

QString labelText(QWidget* w, const char* name)
{
    QLabel* l = w->findChild<QLabel*>(QLatin1String(name));
    return l ? QTextDocumentFragment::fromHtml(l->text()).toPlainText() : QString();
}

void shoot(QWidget* w, const QString& dir, const char* file)
{
    if (dir.isEmpty())
        return;
    QCoreApplication::processEvents();
    w->grab().save(dir + QLatin1Char('/') + QLatin1String(file));
}

QStringList keysOf(const SnapshotTableModel* m)
{
    QStringList out;
    for (int r = 0; r < m->rowCount(); ++r)
        out << m->keyAt(r);
    return out;
}

void fillPatient(QWidget* w, const char* prefix, const QString& name, int age, const QString& complaint)
{
    const QString p = QLatin1String(prefix);
    w->findChild<QLineEdit*>(p + QStringLiteral("Name"))->setText(name);
    w->findChild<QSpinBox*>(p + QStringLiteral("Age"))->setValue(age);
    w->findChild<QLineEdit*>(p + QStringLiteral("Complaint"))->setText(complaint);
}
