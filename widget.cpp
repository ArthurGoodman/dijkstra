#include "widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent) {
    resize(qApp->desktop()->size() / 1.3);
    move(qApp->desktop()->rect().center() - rect().center());
    setMinimumSize(qApp->desktop()->size() / 4);

    startTimer(16);

    setMouseTracking(true);

    newPoint = 0;

    start = 0;
    end = new QPoint();
}

Widget::~Widget() {
    delete newPoint;

    delete start;
    delete end;
}

void Widget::timerEvent(QTimerEvent *) {
    update();
}

void Widget::mousePressEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::LeftButton) {
        delete start;
        start = 0;

        if (distance(newPolygon.first(), e->pos()) <= 2 * circleRadius) {
            if (newPolygon.size() > 2)
                closePolygon();
        } else {
            newPolygon.append(e->pos());

            if (newPolygon.size() == 1)
                newPoint = new QPoint(e->pos());

            vertices.clear();
        }
    } else if (e->buttons() & Qt::RightButton && newPoint == 0) {
        delete start;
        start = 0;

        bool outside = true;

        foreach (QPolygon polygon, polygons)
            if (polygon.containsPoint(e->pos(), Qt::WindingFill)) {
                outside = false;
                break;
            }

        if (outside) {
            start = new QPoint(e->pos());
            *end = e->pos();
        }

        compute();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *e) {
    if (newPoint != 0)
        *newPoint = e->pos();
    else if (start != 0) {
        bool outside = true;

        foreach (QPolygon polygon, polygons)
            if (polygon.containsPoint(e->pos(), Qt::WindingFill)) {
                outside = false;
                break;
            }

        if (outside) {
            *end = e->pos();
            compute();
        }
    }
}

void Widget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
    case Qt::Key_Escape:
        isFullScreen() ? showNormal() : qApp->quit();
        break;

    case Qt::Key_F11:
        isFullScreen() ? showNormal() : showFullScreen();
        break;

    case Qt::Key_Backspace:
        delete start;
        start = 0;

        if (!polygons.isEmpty())
            compute();
        else
            vertices.clear();
        break;

    case Qt::Key_R:
        delete start;
        start = 0;

        polygons.clear();
        vertices.clear();
        edges.clear();
        break;
    }
}

void Widget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::lightGray);

    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::gray, 1));

    for (int i = 0; i < vertices.size(); i++)
        for (int j = i + 1; j < vertices.size(); j++)
            if (edges[i][j])
                p.drawLine(vertices[i], vertices[j]);

    foreach (QPolygon poly, polygons)
        drawPolygon(poly, &p, Qt::red);

    drawPolygon(newPolygon, &p, Qt::blue, false);

    if (start != 0) {
        QPainterPath pp;
        pp.setFillRule(Qt::WindingFill);

        pp.moveTo(*start);

        for (int i = 0; i < path.size(); i++)
            pp.lineTo(vertices[path[i]]);

        p.strokePath(pp, QPen(Qt::blue, 2));

        pp = QPainterPath();

        pp.addEllipse(*start, circleRadius, circleRadius);
        pp.addEllipse(*end, circleRadius, circleRadius);

        p.fillPath(pp, Qt::green);
        p.strokePath(pp, QPen(Qt::black, 1));
    }
}

double Widget::distance(const QPoint &a, const QPoint &b) {
    return std::sqrt(std::pow(a.x() - b.x(), 2) + std::pow(a.y() - b.y(), 2));
}

void Widget::drawPolygon(const QPolygon &poly, QPainter *p, QColor color, bool connect) {
    QPainterPath linePath, circlePath;
    circlePath.setFillRule(Qt::WindingFill);

    if (!poly.isEmpty()) {
        linePath.moveTo(poly.first());

        if (!connect)
            circlePath.addEllipse(poly.first(), circleRadius, circleRadius);
    }

    for (int i = 1; i < poly.size(); i++) {
        linePath.lineTo(poly.at(i));

        if (!connect)
            circlePath.addEllipse(poly.at(i), circleRadius, circleRadius);
    }

    if (!poly.isEmpty())
        linePath.lineTo(newPoint && !connect ? *newPoint : poly.first());

    if (newPoint != 0)
        circlePath.addEllipse(*newPoint, circleRadius, circleRadius);

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

    if (!isPolygonClockwise(newPolygon)) {
        QPolygon reversed;

        for (int i = newPolygon.size() - 1; i >= 0; i--)
            reversed.append(newPolygon[i]);

        newPolygon = reversed;
    }

    polygons.append(newPolygon);
    newPolygon.clear();

    delete newPoint;
    newPoint = 0;

    compute();
}

void Widget::compute() {
    buildVisibilityGraph();
    dijkstra();
}

void Widget::buildVisibilityGraph() {
    vertices.clear();
    edges.clear();

    if (start != 0)
        vertices.append(*start);

    foreach (QPolygon polygon, polygons)
        for (int i = 0; i < polygon.size(); i++)
            if (isVertexConcave(polygon, i))
                vertices.append(polygon[i]);

    if (start != 0)
        vertices.append(*end);

    edges.resize(vertices.size());

    for (int i = 0; i < vertices.size(); i++)
        edges[i].resize(vertices.size());

    for (int i = 0; i < vertices.size(); i++)
        for (int j = i; j < vertices.size(); j++)
            edges[i][j] = edges[j][i] = isLineOfSight(vertices[i], vertices[j]);
}

void Widget::dijkstra() {
    path.clear();
    dist.clear();
    prev.clear();

    if (start != 0 && *start == *end)
        return;

    QSet<int> q;

    for (int i = 0; i < vertices.size(); i++) {
        dist.append(INFINITY);
        prev.append(-1);

        q.insert(i);
    }

    dist[0] = 0;

    while (!q.isEmpty()) {
        double minDist = INFINITY;
        int u = -1;

        foreach (int x, q)
            if (dist[x] < minDist) {
                minDist = dist[x];
                u = x;
            }

        if (u == vertices.size() - 1)
            break;

        q.remove(u);

        for (int v = 0; v < vertices.size(); v++) {
            if (edges[u][v]) {
                double alt = dist[u] + distance(vertices[u], vertices[v]);

                if (alt < dist[v]) {
                    dist[v] = alt;
                    prev[v] = u;
                }
            }
        }
    }

    int u = vertices.size() - 1;

    while (prev[u] != -1) {
        path.push_front(u);
        u = prev[u];
    }

    path.push_front(u);
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

bool Widget::isLineOfSight(const QPoint &a, const QPoint &b) {
    if (a == b)
        return false;

    QPolygon containingPolygon;

    foreach (QPolygon polygon, polygons) {
        if (polygon.contains(a) && polygon.contains(b))
            containingPolygon = polygon;

        for (int i = 0; i < polygon.size(); i++)
            if (lineSegmentsCross(polygon[i], polygon[(i + 1) % polygon.size()], a, b))
                return false;
    }

    if (!containingPolygon.isEmpty()) {
        int dist = qAbs(containingPolygon.indexOf(a) - containingPolygon.indexOf(b));
        return dist == 1 || dist == containingPolygon.size() - 1 || !containingPolygon.containsPoint(QPoint((a.x() + b.x()) / 2, (a.y() + b.y()) / 2), Qt::WindingFill);
    }

    return true;
}

bool Widget::isPolygonClockwise(const QPolygon &polygon) {
    int sum = 0;

    for (int i = 0; i < polygon.size(); i++) {
        QPoint a = polygon[i];
        QPoint b = polygon[(i + 1) % polygon.size()];

        sum += (b.x() - a.x()) * (b.y() + a.y());
    }

    return sum < 0;
}
