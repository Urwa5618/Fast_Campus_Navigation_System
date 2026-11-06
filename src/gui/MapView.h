#pragma once
#include <QGraphicsView>
#include "Graph.h"

class QGraphicsScene;

// Draws the campus graph: circles for locations, lines for pathways.
class MapView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);
    void setGraph(const Graph &g);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *scene_;
};