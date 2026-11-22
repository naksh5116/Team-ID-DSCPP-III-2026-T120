// RouteStepsView.cpp
#include "RouteStepsView.hpp"

#include "Text.hpp"
#include "Theme.hpp"

#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QStringListModel>
#include <QVBoxLayout>

using Text::qs;

RouteStepsView::RouteStepsView(QWidget* parent) : QWidget(parent)
{
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(4);

    title_ = new QLabel(this);
    title_->setObjectName(QStringLiteral("routeTitle"));
    title_->setProperty("textRole", QStringLiteral("heading"));
    title_->setWordWrap(true);
    v->addWidget(title_);

    model_ = new QStringListModel(this);
    list_ = new QListView(this);
    list_->setObjectName(QStringLiteral("routeSteps"));
    list_->setModel(model_);
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setWordWrap(true);
    list_->setUniformItemSizes(false);
    v->addWidget(list_, 1);

    summary_ = new QLabel(this);
    summary_->setObjectName(QStringLiteral("routeSummary"));
    summary_->setWordWrap(true);
    v->addWidget(summary_);

    connect(list_->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current) { Q_EMIT stepSelected(current.isValid() ? current.row() : -1); });

    clear();
}

void RouteStepsView::setRoute(const Route& route)
{
    QStringList rows;
    for (std::size_t i = 0; i < route.steps.size(); ++i)
        rows << QStringLiteral("%1. %2").arg(i + 1).arg(qs(route.steps[i]));
    model_->setStringList(rows);
    title_->setText(QStringLiteral("Route: %1 to %2").arg(qs(route.origin), qs(route.destination)));
    summary_->setText(Text::routeSummary(route.distance, route.seconds));
    list_->setEnabled(true);
}

void RouteStepsView::clear(const QString& note)
{
    model_->setStringList({});
    title_->setText(QStringLiteral("Route"));
    summary_->setText(note.isEmpty() ? QStringLiteral("No route to show.") : note);
    list_->setEnabled(false);
}

void RouteStepsView::setTextScale(double factor)
{
    list_->setFont(Theme::scaledFont(factor));
    title_->setFont(Theme::scaledFont(factor, true));
    summary_->setFont(Theme::scaledFont(factor * 1.2, true));
}

int RouteStepsView::stepCount() const
{
    return model_->rowCount();
}

QString RouteStepsView::stepText(int index) const
{
    return model_->stringList().value(index);
}

QString RouteStepsView::title() const
{
    return title_->text();
}

QString RouteStepsView::summary() const
{
    return summary_->text();
}
