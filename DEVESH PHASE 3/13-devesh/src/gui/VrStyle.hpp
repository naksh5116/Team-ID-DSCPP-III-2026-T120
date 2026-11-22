// VrStyle.hpp - Fusion with a few primitives drawn from the design tokens.
//
// With style.qss active, Fusion's check box frame is drawn almost white on
// white. This proxy draws the small indicators itself, as vector shapes in
// logical coordinates, so they use the DESIGN.md colours and stay sharp at
// any display scale:
//   - check box: square with a `control` outline; accent fill and a white
//     tick when checked
//   - arrows that are not styled by the stylesheet (scroll bars, menus):
//     small `text` triangles
// Spin box and combo box arrows come from style.qss (images Theme draws).
#ifndef VR_GUI_VR_STYLE_HPP
#define VR_GUI_VR_STYLE_HPP

#include <QProxyStyle>

class VrStyle : public QProxyStyle {
    Q_OBJECT
public:
    VrStyle();

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
};

#endif // VR_GUI_VR_STYLE_HPP
