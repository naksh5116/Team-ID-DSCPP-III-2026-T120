// UiHelpers.cpp
#include "UiHelpers.hpp"

#include "SnapshotTableModel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTableView>
#include <QVBoxLayout>

QLayout* leftAligned(QWidget* button)
{
    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(button);
    row->addStretch(1);
    return row;
}

QLabel* roleLabel(const QString& text, const char* textRole, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setProperty("textRole", QLatin1String(textRole));
    if (qstrcmp(textRole, "hint") == 0)
        l->setWordWrap(true);
    return l;
}

QWidget* tableSection(const QString& heading, const QString& hint, QTableView* table,
                      QWidget* parent)
{
    auto* w = new QWidget(parent);
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(4);
    v->addWidget(roleLabel(heading, "heading", w));
    if (!hint.isEmpty())
        v->addWidget(roleLabel(hint, "hint", w));
    v->addSpacing(4);
    table->setParent(w);
    v->addWidget(table, 1);
    return w;
}

void fitColumnsOnReset(QTableView* table)
{
    QObject::connect(table->model(), &QAbstractItemModel::modelReset, table,
                     [table] { table->resizeColumnsToContents(); });
    table->resizeColumnsToContents();
}

void selectKey(QTableView* table, const QString& key)
{
    const auto* model = qobject_cast<const SnapshotTableModel*>(table->model());
    if (!model)
        return;
    const int row = model->rowOfKey(key);
    if (row >= 0) {
        table->selectRow(row);
        table->scrollTo(model->index(row, 0));
    }
}

namespace {
bool g_autoConfirm = false;
}

void setAutoConfirmForChecks(bool on)
{
    g_autoConfirm = on;
}

bool confirm(QWidget* parent, const QString& title, const QString& text, const QString& acceptLabel)
{
    if (g_autoConfirm)
        return true;
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(text);
    box.setIcon(QMessageBox::NoIcon);
    QPushButton* accept = box.addButton(acceptLabel, QMessageBox::AcceptRole);
    QPushButton* cancel = box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(cancel);   // a destructive action is never the default
    box.exec();
    return box.clickedButton() == accept;
}

QScrollArea* scrollable(QWidget* content, QWidget* parent)
{
    auto* area = new QScrollArea(parent);
    area->setWidget(content);
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    return area;
}
