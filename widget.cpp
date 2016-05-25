#include "widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent) {
    resize(qApp->desktop()->size() / 1.3);
    move(qApp->desktop()->rect().center() - rect().center());
    setMinimumSize(qApp->desktop()->size() / 4);

    startTimer(16);

    setMouseTracking(true);

    newPoint = 0;
}

Widget::~Widget() {
}

void Widget::timerEvent(QTimerEvent *) {
    update();
}

void Widget::mousePressEvent(QMouseEvent *e) {
    const static int radius = 8;

    if (e->buttons() & Qt::LeftButton) {
        if (length(newPolygon.first(), e->pos()) <= radius) {
            if (newPolygon.size() > 2)
                closePolygon();
        } else {
            newPolygon.append(e->pos());

            if (newPolygon.size() == 1)
                newPoint = new QPoint(e->pos());
        }
    }
}

void Widget::mouseMoveEvent(QMouseEvent *e) {
    if (newPoint)
        *newPoint = e->pos();
}

void Widget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
    case Qt::Key_Escape:
        isFullScreen() ? showNormal() : qApp->quit();
        break;

    case Qt::Key_F11:
        isFullScreen() ? showNormal() : showFullScreen();
        break;
    }
}

void Widget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::lightGray);

    p.setRenderHint(QPainter::Antialiasing);

    foreach (QPolygon poly, polygons)
        drawPolygon(poly, &p, Qt::red);

    drawPolygon(newPolygon, &p, Qt::blue, true);
}

double Widget::length(const QPoint &a, const QPoint &b) {
    return std::sqrt(std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2));
}

void Widget::drawPolygon(const QPolygon &poly, QPainter *p, QColor color, bool connect) {
    QPainterPath linePath, circlePath;

    circlePath.setFillRule(Qt::WindingFill);

    const static int radius = 4;

    if (!poly.isEmpty()) {
        linePath.moveTo(poly.first());
        circlePath.addEllipse(poly.first(), radius, radius);
    }

    for (int i = 1; i < poly.size(); i++) {
        linePath.lineTo(poly.at(i));
        circlePath.addEllipse(poly.at(i), radius, radius);
    }

    if (!poly.isEmpty())
        linePath.lineTo(newPoint && connect ? *newPoint : poly.first());

    if (newPoint)
        circlePath.addEllipse(*newPoint, radius, radius);

    p->strokePath(linePath, QPen(color, 2));
    p->fillPath(circlePath, color.lighter());
    p->strokePath(circlePath, QPen(Qt::black, 1));
}

void Widget::closePolygon() {
    QMutableListIterator<QPolygon> i(polygons);

    while (i.hasNext()) {
        QPolygon poly = i.next();

        if (!newPolygon.intersected(poly).isEmpty()) {
            newPolygon = newPolygon.united(poly);
            i.remove();
        }
    }

    polygons.append(newPolygon);
    newPolygon.clear();

    delete newPoint;
    newPoint = 0;

    buildVisibilityGraph();
}

void Widget::buildVisibilityGraph() {
    vertices.clear();

    foreach (QPolygon polygon, polygons)
        for (int i = 0; i < polygon.size(); i++)
            if (isVertexConcave(polygon, i))
                vertices.append(polygon[i]);
}

bool Widget::isVertexConcave(const QPolygon &polygon, int vertex) {
    QPoint current = polygon[vertex];
    QPoint next = polygon[(vertex + 1) % polygon.size()];
    QPoint previous = polygon[vertex == 0 ? polygon.size() - 1 : vertex - 1];

    QPoint left = QPoint(current.x() - previous.x(), current.y() - previous.y());
    QPoint right = QPoint(next.x() - current.x(), next.y() - current.y());

    double cross = (left.x() * right.y()) - (left.y() * right.x());

    return cross >= 0;
}

bool Widget::lineSegmentsCross(const QPoint &a, const QPoint &b, const QPoint &c, const QPoint &d) {
    double denominator = ((b.x() - a.x()) * (d.y() - c.y())) - ((b.y() - a.y()) * (d.x() - c.x()));

    if (denominator == 0)
        return false;

    double numerator1 = ((a.y() - c.y()) * (d.x() - c.x())) - ((a.x() - c.x()) * (d.y() - c.y()));
    double numerator2 = ((a.y() - c.y()) * (b.x() - a.x())) - ((a.x() - c.x()) * (b.y() - a.y()));

    if (numerator1 == 0 || numerator2 == 0)
        return false;

    double r = numerator1 / denominator;
    double s = numerator2 / denominator;

    return (r > 0 && r < 1) && (s > 0 && s < 1);
}
