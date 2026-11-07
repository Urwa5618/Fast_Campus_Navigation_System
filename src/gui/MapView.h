#pragma once
#include <QGraphicsView>
#include <map>
#include <utility>
#include <vector>
#include "Graph.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsItem;
class QWheelEvent;
class QMouseEvent;

// Shows the campus map image with the graph drawn on top of it.
// Mouse wheel = zoom, drag = move, double-click = fit whole map.
class MapView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);
    // dataDir must contain campus_map.png and (optionally) paths.csv
    void setGraph(const Graph &g, const QString &dataDir);
    void showRoute(const std::vector<int> &path);
    void clearRoute();
    void fitAll();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QGraphicsScene *scene_;
    std::vector<QGraphicsEllipseItem *> nodeItems_;                // index = location id
    std::map<std::pair<int, int>, QGraphicsPathItem *> edgeItems_; // key = (smaller id, larger id)
    std::map<std::pair<int, int>, QGraphicsItem *> edgeTags_;      // distance labels
    double fitScale_ = 1.0;
    bool zoomed_ = false;
};