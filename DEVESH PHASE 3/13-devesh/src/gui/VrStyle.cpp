// VrStyle.cpp
#include "VrStyle.hpp"

#include "Theme.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStyleFactory>
#include <QStyleOption>

VrStyle::VrStyle() : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))) {}

namespace {

void drawArrow(QPainter* p, const QRectF& r, QStyle::PrimitiveElement element, const QColor& color)
{
    // A small solid triangle centred in r: about 8 x 4.5 logical pixels.
    const double w = qMin(8.0, r.width() - 2.0);
    const double h = w * 0.56;
    const QPointF c = r.center();
    QPolygonF tri;
    switch (element) {
    case QStyle::PE_IndicatorArrowUp:
    case QStyle::PE_IndicatorSpinUp:
    case QStyle::PE_IndicatorSpinPlus:
        tri << QPointF(c.x() - w / 2, c.y() + h / 2) << QPointF(c.x() + w / 2, c.y() + h / 2)
            << QPointF(c.x(), c.y() - h / 2);
        break;
    case QStyle::PE_IndicatorArrowLeft:
        tri << QPointF(c.x() + h / 2, c.y() - w / 2) << QPointF(c.x() + h / 2, c.y() + w / 2)
            << QPointF(c.x() - h / 2, c.y());
        break;
    case QStyle::PE_IndicatorArrowRight:
        tri << QPointF(c.x() - h / 2, c.y() - w / 2) << QPointF(c.x() - h / 2, c.y() + w / 2)
            << QPointF(c.x() + h / 2, c.y());
        break;
    default:   // down
        tri << QPointF(c.x() - w / 2, c.y() - h / 2) << QPointF(c.x() + w / 2, c.y() - h / 2)
            << QPointF(c.x(), c.y() + h / 2);
        break;
    }
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(color);
    p->drawPolygon(tri);
    p->restore();
}

} // namespace

void VrStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                            QPainter* painter, const QWidget* widget) const
{
    const bool enabled = option->state & State_Enabled;
    switch (element) {
    case PE_IndicatorCheckBox: {
        const int side = qMin(option->rect.width(), option->rect.height());
        const QRectF box(option->rect.x() + 0.5, option->rect.y() + (option->rect.height() - side) / 2.0 + 0.5,
                         side - 1.0, side - 1.0);
        const bool on = option->state & State_On;
        const bool focus = option->state & State_HasFocus;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QColor border = enabled ? (focus || on ? Theme::color("accent") : Theme::color("control"))
                                : Theme::color("border");
        QColor fill = !enabled ? Theme::color("panel") : (on ? Theme::color("accent") : Theme::color("surface"));
        painter->setPen(QPen(border, 1.0));
        painter->setBrush(fill);
        painter->drawRoundedRect(box, 2.0, 2.0);
        if (on || (option->state & State_NoChange)) {
            QPen tick(enabled ? Theme::color("onAccent") : Theme::color("textDisabled"), side / 7.0);
            tick.setCapStyle(Qt::RoundCap);
            tick.setJoinStyle(Qt::RoundJoin);
            painter->setPen(tick);
            painter->setBrush(Qt::NoBrush);
            if (option->state & State_NoChange) {
                painter->drawLine(QPointF(box.left() + side * 0.25, box.center().y()),
                                  QPointF(box.right() - side * 0.25, box.center().y()));
            } else {
                QPainterPath path;
                path.moveTo(box.left() + side * 0.22, box.top() + side * 0.52);
                path.lineTo(box.left() + side * 0.42, box.top() + side * 0.72);
                path.lineTo(box.left() + side * 0.78, box.top() + side * 0.30);
                painter->drawPath(path);
            }
        }
        painter->restore();
        return;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown:
    case PE_IndicatorSpinPlus:
    case PE_IndicatorSpinMinus: {
        PrimitiveElement shape = element;
        if (element == PE_IndicatorSpinPlus)
            shape = PE_IndicatorSpinUp;
        else if (element == PE_IndicatorSpinMinus)
            shape = PE_IndicatorSpinDown;
        drawArrow(painter, option->rect, shape,
                  enabled ? Theme::color("text") : Theme::color("textDisabled"));
        return;
    }
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
}

int VrStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    switch (metric) {
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
        return 14;   // logical pixels; scaled per monitor by Qt
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}
