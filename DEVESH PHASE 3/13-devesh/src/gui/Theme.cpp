// Theme.cpp - token values. Change a colour here and nowhere else.
#include "Theme.hpp"

#include "VrStyle.hpp"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHeaderView>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QStringList>
#include <QTableView>

#include <cstring>

namespace {

struct Token {
    const char* name;
    const char* value;
};

const Token kTokens[] = {
    {"surface", "#FFFFFF"},      {"panel", "#F4F5F7"},       {"border", "#D9DCE1"},
    {"control", "#8C939D"},      {"text", "#1B1F24"},        {"textMuted", "#5B636E"},
    {"textDisabled", "#8A9099"}, {"hover", "#ECEEF1"},       {"pressed", "#E2E5E9"},
    {"accent", "#143769"},       {"accentHover", "#1D4A87"}, {"accentPressed", "#0E2850"},
    {"onAccent", "#FFFFFF"},     {"corridor", "#9AA1AB"},
};

const char* const kEsiFill[] = {"#C62828", "#E65100", "#F9A825", "#2E7D32", "#1565C0"};

bool validLevel(int level) { return level >= 1 && level <= 5; }

} // namespace

namespace Theme {

QColor color(const char* token)
{
    for (const Token& t : kTokens)
        if (std::strcmp(t.name, token) == 0)
            return QColor(QLatin1String(t.value));
    return QColor();
}

namespace {

// Stylesheets can only draw spin box and combo box arrows from image files.
// These are drawn here from the tokens (4x size, so they stay sharp at any
// display scale) into a folder that lives as long as the application.
QString arrowImage(const QString& name, bool up, const QColor& fill)
{
    static QTemporaryDir dir;
    if (!dir.isValid())
        return QString();
    const QString path = dir.path() + QLatin1Char('/') + name + QStringLiteral(".png");
    if (!QFile::exists(path)) {
        QImage img(32, 20, QImage::Format_ARGB32_Premultiplied);   // 8 x 5 logical, 4x
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        QPolygonF tri;
        if (up)
            tri << QPointF(0, 20) << QPointF(32, 20) << QPointF(16, 0);
        else
            tri << QPointF(0, 0) << QPointF(32, 0) << QPointF(16, 20);
        p.drawPolygon(tri);
        p.end();
        img.save(path, "PNG");
    }
    return path;
}

QString imageToken(const QString& name)
{
    if (name == QStringLiteral("imgArrowUp"))
        return arrowImage(name, true, Theme::color("text"));
    if (name == QStringLiteral("imgArrowDown"))
        return arrowImage(name, false, Theme::color("text"));
    if (name == QStringLiteral("imgArrowUpDisabled"))
        return arrowImage(name, true, Theme::color("textDisabled"));
    if (name == QStringLiteral("imgArrowDownDisabled"))
        return arrowImage(name, false, Theme::color("textDisabled"));
    return QString();
}

} // namespace

QString styleSheet()
{
    QFile file(QStringLiteral(":/style.qss"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QString qss = QString::fromUtf8(file.readAll());

    // Replace @name with its colour (or, for @img... names, the path of the
    // image drawn from the colours). The match takes the whole identifier.
    static const QRegularExpression tokenPattern(QStringLiteral("@([A-Za-z]+)"));
    QString out;
    qsizetype last = 0;
    auto it = tokenPattern.globalMatch(qss);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += QStringView(qss).mid(last, m.capturedStart() - last);
        const QString name = m.captured(1);
        const QString image = imageToken(name);
        const QColor c = color(name.toLatin1().constData());
        if (!image.isEmpty())
            out += image;
        else
            out += c.isValid() ? c.name() : m.captured(0);   // unknown tokens stay visible
        last = m.capturedEnd();
    }
    out += QStringView(qss).mid(last);
    return out;
}

QPalette palette()
{
    QPalette p;
    const QColor surface = color("surface"), panel = color("panel"), text = color("text");
    p.setColor(QPalette::Window, surface);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, surface);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, surface);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::ToolTipBase, surface);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, color("textMuted"));
    p.setColor(QPalette::Highlight, color("accent"));
    p.setColor(QPalette::HighlightedText, color("onAccent"));
    p.setColor(QPalette::Mid, color("border"));
    p.setColor(QPalette::Dark, color("control"));
    p.setColor(QPalette::Light, surface);
    p.setColor(QPalette::Midlight, panel);
    for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, role, color("textDisabled"));
    p.setColor(QPalette::Disabled, QPalette::Base, panel);
    p.setColor(QPalette::Disabled, QPalette::Button, panel);
    // Keep the selection accent when a table loses focus.
    p.setColor(QPalette::Inactive, QPalette::Highlight, color("accent"));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, color("onAccent"));
    return p;
}

void apply()
{
    QApplication::setStyle(new VrStyle);   // Fusion plus token-coloured indicators
    QApplication::setPalette(palette());
    qApp->setStyleSheet(styleSheet());
}

QColor esiFill(int level)
{
    return validLevel(level) ? QColor(QLatin1String(kEsiFill[level - 1])) : color("panel");
}

QColor esiText(int level)
{
    return level == 3 ? color("text") : (validLevel(level) ? color("onAccent") : color("text"));
}

QString esiName(int level)
{
    switch (level) {
    case 1: return QStringLiteral("Resuscitation");
    case 2: return QStringLiteral("Emergent");
    case 3: return QStringLiteral("Urgent");
    case 4: return QStringLiteral("Less urgent");
    case 5: return QStringLiteral("Non-urgent");
    default: return QString();
    }
}

QString esiDescription(int level)
{
    // Emergency Severity Index decision points, in short.
    switch (level) {
    case 1: return QStringLiteral("Needs an immediate life-saving intervention");
    case 2: return QStringLiteral("High-risk, confused or lethargic, or in severe pain or distress");
    case 3: return QStringLiteral("Stable; expected to need two or more resources");
    case 4: return QStringLiteral("Stable; expected to need one resource");
    case 5: return QStringLiteral("Stable; expected to need no resources");
    default: return QString();
    }
}

QFont monoFont()
{
    static const QString family = [] {
        const QStringList installed = QFontDatabase::families();
        for (const char* name : {"Consolas", "Cascadia Mono", "DejaVu Sans Mono", "Liberation Mono"})
            if (installed.contains(QLatin1String(name)))
                return QString::fromLatin1(name);
        return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    }();
    QFont f(family);
    f.setStyleHint(QFont::Monospace);
    const QFont ui = QApplication::font();
    if (ui.pointSizeF() > 0)
        f.setPointSizeF(ui.pointSizeF());
    else if (ui.pixelSize() > 0)
        f.setPixelSize(ui.pixelSize());
    return f;
}

void setupTable(QTableView* table)
{
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->setAlternatingRowColors(false);
    table->verticalHeader()->hide();
    // Compact rows: one text line plus 2 px padding above and below.
    const int row = QFontMetrics(table->font()).height() + 6;
    table->verticalHeader()->setMinimumSectionSize(row);
    table->verticalHeader()->setDefaultSectionSize(row);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setStretchLastSection(true);
}

QFont scaledFont(double factor, bool semibold)
{
    QFont f = QApplication::font();
    if (f.pointSizeF() > 0)
        f.setPointSizeF(f.pointSizeF() * factor);
    else if (f.pixelSize() > 0)
        f.setPixelSize(qRound(f.pixelSize() * factor));
    if (semibold)
        f.setWeight(QFont::DemiBold);
    return f;
}

} // namespace Theme
