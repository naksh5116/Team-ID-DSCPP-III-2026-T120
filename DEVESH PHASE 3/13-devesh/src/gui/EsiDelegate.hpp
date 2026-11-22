// EsiDelegate.hpp - draws the ESI badge in a table cell.
//
// The model puts the ESI level (1-5) in EsiDelegate::LevelRole and the level
// name ("Resuscitation") in Qt::DisplayRole. The delegate paints a small
// square badge with the number, then the name in normal text - so the level
// is never carried by colour alone (DESIGN.md).
#ifndef VR_GUI_ESI_DELEGATE_HPP
#define VR_GUI_ESI_DELEGATE_HPP

#include <QStyledItemDelegate>
#include <QWidget>

class EsiDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    static constexpr int LevelRole = Qt::UserRole + 40;

    explicit EsiDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // Paints one badge for `level` in `rect` (also used outside tables).
    static void paintBadge(QPainter* painter, const QRect& rect, int level, const QFont& font);
    // Side of the square badge for a given font: one text line high.
    static int badgeSide(const QFont& font);
};

// The same badge as a small stand-alone widget (forms, record headers).
class EsiBadge : public QWidget {
    Q_OBJECT
public:
    explicit EsiBadge(int level = 0, QWidget* parent = nullptr);
    void setLevel(int level);
    int level() const { return level_; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int level_;
};

#endif // VR_GUI_ESI_DELEGATE_HPP
