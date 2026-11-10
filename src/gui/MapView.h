#pragma once
#include <QGraphicsView>
#include <QColor>
#include <map>
#include <utility>
#include <vector>
#include "Graph.h"

class QGraphicsScene;
class QGraphicsLineItem;
class QGraphicsEllipseItem;
class QWheelEvent;
class QMouseEvent;

// Draws the FAST campus map (buildings, roads, gardens... written in MapData.h) and the walking
// network on top of it. Mouse wheel = zoom, drag = move, double-click = fit the whole map.
class MapView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapView(QWidget* parent = nullptr);
    void setGraph(const Graph& g, const QString& dataDir);
    void showRoute(const std::vector<int>& path);
    void clearRoute();
    void fitAll();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QGraphicsScene* scene_;
    std::vector<QGraphicsEllipseItem*> nodeItems_;                    // index = node id
    std::vector<QColor> nodeColors_;                                  // normal colour of each node
    std::map<std::pair<int, int>, QGraphicsLineItem*> edgeItems_;     // key = (smaller id, larger id)
    double fitScale_ = 1.0;
    bool zoomed_ = false;
};
