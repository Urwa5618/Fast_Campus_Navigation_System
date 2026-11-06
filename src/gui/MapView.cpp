#include "MapView.h"
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>

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

            auto *line = scene_->addLine(a.x, a.y, b.x, b.y,
                                         QPen(QColor("#9aa5b1"), 2));
            line->setZValue(0);

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
                                          QPen(Qt::white, 2),
                                          QBrush(QColor("#2b6cb0")));
        circle->setZValue(2);
        circle->setToolTip(QString::fromStdString(loc.name));

        auto *text = scene_->addSimpleText(QString::fromStdString(loc.name));
        text->setFont(nameFont);
        text->setBrush(QColor("#1a202c"));
        text->setPos(loc.x - text->boundingRect().width() / 2, loc.y + 12);
        text->setZValue(3);
    }

    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void MapView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio); // keep the whole map visible
}