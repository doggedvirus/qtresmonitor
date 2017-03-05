#include "ratepainter.h"

RatePainter::RatePainter(QPaintDevice* parent)
    :QPainter(parent)
{
    m_color = Qt::darkGray;
    m_linewidth = 10;
    m_linestyle = Qt::SolidLine;
    m_capstyle = Qt::RoundCap;
    m_joinstyle = Qt::RoundJoin;

}

void RatePainter::drawRate(int x, int y, int width, int height, int rate)
{
    QRectF rect(x, y, width, height); //x,y,w,h
    int startAngle = (240 - 3 * rate) * 16; //值为，实际角度 * 16
    int spanAngle = 3 * 16 * rate;

    //三个参数：rect表示弧线所在的矩形，startAngle起始角度，spanAngle跨越角度
    QPen pen(m_color, m_linewidth, m_linestyle, m_capstyle, m_joinstyle);
    this->setPen(pen);
    this->drawArc(rect, startAngle, spanAngle);
}

void RatePainter::setColor(QColor color)
{
    m_color = color;
}

 int RatePainter::getPenwidth(void)
 {
     return m_linewidth;
 }
