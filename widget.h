#pragma once

#include <QtWidgets>

class Widget : public QWidget {
    Q_OBJECT

    const static int circleRadius = 4;

    QList<QPolygon> polygons;
    QPolygon newPolygon;
    QPoint *newPoint;

    QVector<QPoint> vertices;
    QVector<QVector<bool>> edges;

    QPoint *start, *end;

public:
    Widget(QWidget *parent = 0);
    ~Widget();

    void timerEvent(QTimerEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    double length(const QPoint &a, const QPoint &b);
    void drawPolygon(const QPolygon &poly, QPainter *p, QColor color, bool connect = false);

    void closePolygon();
    void buildVisibilityGraph();

    bool isVertexConcave(const QPolygon &polygon, int index);
    bool lineSegmentsCross(const QPoint &a, const QPoint &b, const QPoint &c, const QPoint &d);
    bool isLineOfSight(const QPoint &a, const QPoint &b);
};
