#ifndef RATEPAINTER_H
#define RATEPAINTER_H
#include <QPainter>

class RatePainter : public QPainter
{
public:
    RatePainter(QPaintDevice* parent);
    void drawRate(int x, int y, int width, int height, int rate);
    void setColor(QColor color);
    int getPenwidth(void);

private:
    QColor m_color;
    qreal m_linewidth;
    Qt::PenStyle m_linestyle;
    Qt::PenCapStyle m_capstyle;
    Qt::PenJoinStyle m_joinstyle;

};

#endif // RATEPAINTER_H
