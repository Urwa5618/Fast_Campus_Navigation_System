#include "MapView.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <algorithm>

static const QColor kNodeColor("#2b6cb0");
static const QColor kEdgeColor("#9aa5b1");
static const QColor kRouteColor("#dd6b20");
static const QColor kStartColor("#38a169");
static const QColor kEndColor("#e53e3e");

MapView::MapView(QWidget *parent) : QGraphicsView(parent)
{
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor("#f4f1ea"));
}

void MapView::setGraph(const Graph &g)
{
    scene_->clear();
    nodeItems_.clear();
    edgeItems_.clear();
    nodeItems_.resize(g.size(), nullptr);
    scene_->setSceneRect(0, 0, 900, 600);

    QFont small;
    small.setPointSize(8);
    QFont nameFont;
    nameFont.setPointSize(9);
    nameFont.setBold(true);

    // 1. Edges first (so circles are drawn on top of the lines)
    for (int u = 0; u < g.size(); ++u)
    {
        for (const auto &e : g.neighbors(u))
        {
            if (e.to < u)
                continue; // each edge only once
            const Location &a = g.location(u);
            const Location &b = g.location(e.to);

            auto *line = scene_->addLine(a.x, a.y, b.x, b.y, QPen(kEdgeColor, 2));
            line->setZValue(0);
            edgeItems_[{u, e.to}] = line; // u < e.to here

            auto *label = scene_->addSimpleText(QString::number(e.weight) + " m");
            label->setFont(small);
            label->setBrush(QColor("#5a6572"));
            label->setPos((a.x + b.x) / 2 - label->boundingRect().width() / 2,
                          (a.y + b.y) / 2 - label->boundingRect().height() / 2);
            label->setZValue(1);
        }
    }

    // 2. Locations: circle + name below it
    for (int i = 0; i < g.size(); ++i)
    {
        const Location &loc = g.location(i);

        auto *circle = scene_->addEllipse(loc.x - 10, loc.y - 10, 20, 20,
                                          QPen(Qt::white, 2), QBrush(kNodeColor));
        circle->setZValue(2);
        circle->setToolTip(QString::fromStdString(loc.name));
        nodeItems_[i] = circle;

        auto *text = scene_->addSimpleText(QString::fromStdString(loc.name));
        text->setFont(nameFont);
        text->setBrush(QColor("#1a202c"));
        text->setPos(loc.x - text->boundingRect().width() / 2, loc.y + 12);
        text->setZValue(3);
    }

    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void MapView::clearRoute()
{
    for (auto &kv : edgeItems_)
    {
        kv.second->setPen(QPen(kEdgeColor, 2));
        kv.second->setZValue(0);
    }
    for (auto *node : nodeItems_)
        if (node)
            node->setBrush(QBrush(kNodeColor));
}

void MapView::showRoute(const std::vector<int> &path)
{
    clearRoute();

    // highlight each segment of the route
    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
        int a = std::min(path[i], path[i + 1]);
        int b = std::max(path[i], path[i + 1]);
        auto it = edgeItems_.find({a, b});
        if (it != edgeItems_.end())
        {
            it->second->setPen(QPen(kRouteColor, 6, Qt::SolidLine, Qt::RoundCap));
            it->second->setZValue(0.5);
        }
    }

    // colour the locations on the route
    for (int id : path)
        if (id >= 0 && id < static_cast<int>(nodeItems_.size()))
            nodeItems_[id]->setBrush(QBrush(kRouteColor));

    // start = green, destination = red
    if (!path.empty())
    {
        nodeItems_[path.front()]->setBrush(QBrush(kStartColor));
        nodeItems_[path.back()]->setBrush(QBrush(kEndColor));
    }
}

void MapView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio); // keep the whole map visible
}