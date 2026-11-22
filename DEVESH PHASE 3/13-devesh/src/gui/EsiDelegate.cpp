// EsiDelegate.cpp - square ESI badge plus the level name.
#include "EsiDelegate.hpp"

#include "Theme.hpp"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

namespace {
constexpr int kPad = 6;   // DESIGN.md cell padding
}

int EsiDelegate::badgeSide(const QFont& font)
{
    return QFontMetrics(font).height();
}

void EsiDelegate::paintBadge(QPainter* painter, const QRect& rect, int level, const QFont& font)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Theme::esiFill(level));
    painter->drawRoundedRect(QRectF(rect), 2.0, 2.0);   // radius 2 (DESIGN.md)
    QFont bold = font;
    bold.setWeight(QFont::Bold);
    painter->setFont(bold);
    painter->setPen(Theme::esiText(level));
    painter->drawText(rect, Qt::AlignCenter, QString::number(level));
    painter->restore();
}

void EsiDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const
{
    const int level = index.data(LevelRole).toInt();
    if (level < 1 || level > 5) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QString name = opt.text;
    opt.text.clear();
    const QWidget* widget = opt.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);   // background, selection

    const int side = badgeSide(opt.font);
    const QRect badge(opt.rect.left() + kPad, opt.rect.center().y() - side / 2 + 1, side, side);
    paintBadge(painter, badge, level, opt.font);

    const QRect textRect = opt.rect.adjusted(kPad + side + kPad, 0, -kPad, 0);
    painter->save();
    painter->setFont(opt.font);
    const bool selected = opt.state & QStyle::State_Selected;
    painter->setPen(selected ? Theme::color("onAccent") : Theme::color("text"));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(opt.font).elidedText(name, Qt::ElideRight, textRect.width()));
    painter->restore();
}

EsiBadge::EsiBadge(int level, QWidget* parent) : QWidget(parent), level_(level)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void EsiBadge::setLevel(int level)
{
    level_ = level;
    setVisible(level_ >= 1 && level_ <= 5);
    update();
}

QSize EsiBadge::sizeHint() const
{
    const int side = EsiDelegate::badgeSide(font());
    return QSize(side, side);
}

void EsiBadge::paintEvent(QPaintEvent*)
{
    if (level_ < 1 || level_ > 5)
        return;
    QPainter painter(this);
    const int side = qMin(width(), height());
    EsiDelegate::paintBadge(&painter, QRect(0, (height() - side) / 2, side, side), level_, font());
}

QSize EsiDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    const int side = badgeSide(option.font);
    s.setWidth(s.width() + side + kPad);
    s.setHeight(qMax(s.height(), side + 4));
    return s;
}
