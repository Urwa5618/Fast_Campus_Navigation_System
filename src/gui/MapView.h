#pragma once
#include <QGraphicsView>
#include <map>
#include <utility>
#include <vector>
#include "Graph.h"

class QGraphicsScene;
class QGraphicsLineItem;
class QGraphicsEllipseItem;

// Draws the campus graph and can highlight a route on it.
class MapView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);
    void setGraph(const Graph &g);
    void showRoute(const std::vector<int> &path); // highlight a route
    void clearRoute();                            // back to normal colours

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *scene_;
    std::vector<QGraphicsEllipseItem *> nodeItems_;                // index = location id
    std::map<std::pair<int, int>, QGraphicsLineItem *> edgeItems_; // key = (smaller id, larger id)
};