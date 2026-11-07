#include "MapView.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <algorithm>
#include <cmath>

// Coordinates in locations.csv are pixels of the 8000 x 2024 map image
static const double kMapW = 8000;
static const double kMapH = 2024;

static const QColor kNodeColor("#3182ce");
static const QColor kEdgeColor("#63b3ed");
static const QColor kRouteColor("#f6ad55");
static const QColor kStartColor("#48bb78");
static const QColor kEndColor("#f56565");

// A small text label on a dark box, centred on (0,0) of the returned item.
static QGraphicsRectItem *makeTag(const QString &text, const QFont &font,
                                  const QColor &fg, const QColor &bg,
                                  QGraphicsItem *parent)
{
    auto *box = new QGraphicsRectItem(parent);
    auto *label = new QGraphicsSimpleTextItem(text, box);
    label->setFont(font);
    label->setBrush(fg);
    QRectF r = label->boundingRect();
    box->setRect(-r.width() / 2 - 3, -r.height() / 2 - 1, r.width() + 6, r.height() + 2);
    label->setPos(-r.width() / 2, -r.height() / 2);
    box->setBrush(bg);
    box->setPen(Qt::NoPen);
    return box;
}

MapView::MapView(QWidget *parent) : QGraphicsView(parent)
{
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setBackgroundBrush(QColor("#111111"));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MapView::setGraph(const Graph &g, const QString &imagePath)
{
    scene_->clear();
    nodeItems_.clear();
    edgeItems_.clear();
    nodeItems_.resize(g.size(), nullptr);
    scene_->setSceneRect(0, 0, kMapW, kMapH);

    // ----- background map image -----
    QPixmap pix(imagePath);
    if (!pix.isNull())
    {
        auto *bg = scene_->addPixmap(pix);
        bg->setTransformationMode(Qt::SmoothTransformation);
        bg->setScale(kMapW / pix.width());
        bg->setZValue(-10);
    }

    QFont edgeFont;
    edgeFont.setPointSize(7);
    QFont nameFont;
    nameFont.setPointSize(8);
    nameFont.setBold(true);

    // ----- edges (lines stay the same thickness when zooming) -----
    QPen edgePen(kEdgeColor, 2);
    edgePen.setCosmetic(true);

    for (int u = 0; u < g.size(); ++u)
    {
        for (const auto &e : g.neighbors(u))
        {
            if (e.to < u)
                continue; // each edge only once
            const Location &a = g.location(u);
            const Location &b = g.location(e.to);

            auto *line = scene_->addLine(a.x, a.y, b.x, b.y, edgePen);
            line->setZValue(0);
            edgeItems_[{u, e.to}] = line; // u < e.to here

            auto *tag = makeTag(QString::number(e.weight) + " m", edgeFont,
                                QColor("#e2e8f0"), QColor(20, 20, 20, 200), nullptr);
            scene_->addItem(tag);
            tag->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            tag->setPos((a.x + b.x) / 2, (a.y + b.y) / 2);
            tag->setZValue(1);
        }
    }

    // ----- locations: circle + name label (both keep their size when zooming) -----
    for (int i = 0; i < g.size(); ++i)
    {
        const Location &loc = g.location(i);
        QString name = QString::fromStdString(loc.name);

        auto *node = scene_->addEllipse(-7, -7, 14, 14, QPen(Qt::white, 2), QBrush(kNodeColor));
        node->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        node->setPos(loc.x, loc.y);
        node->setZValue(2);
        node->setToolTip(name);
        nodeItems_[i] = node;

        // Nodes that sit close together in a row (the shops) get labels at different heights
        int level = 0;
        for (int j = 0; j < g.size(); ++j)
        {
            if (j == i)
                continue;
            const Location &o = g.location(j);
            if (std::abs(o.y - loc.y) < 150 && std::abs(o.x - loc.x) < 250 && o.x < loc.x)
                ++level;
        }
        level %= 4;

        auto *tag = makeTag(name, nameFont, Qt::white, QColor(0, 0, 0, 175), node);
        tag->setPos(0, 18 + level * 20);
    }

    fitAll();
}

void MapView::clearRoute()
{
    QPen edgePen(kEdgeColor, 2);
    edgePen.setCosmetic(true);
    for (auto &kv : edgeItems_)
    {
        kv.second->setPen(edgePen);
        kv.second->setZValue(0);
    }
    for (auto *node : nodeItems_)
        if (node)
            node->setBrush(QBrush(kNodeColor));
}

void MapView::showRoute(const std::vector<int> &path)
{
    clearRoute();

    QPen routePen(kRouteColor, 5, Qt::SolidLine, Qt::RoundCap);
    routePen.setCosmetic(true);

    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
        int a = std::min(path[i], path[i + 1]);
        int b = std::max(path[i], path[i + 1]);
        auto it = edgeItems_.find({a, b});
        if (it != edgeItems_.end())
        {
            it->second->setPen(routePen);
            it->second->setZValue(0.5);
        }
    }

    for (int id : path)
        if (id >= 0 && id < static_cast<int>(nodeItems_.size()))
            nodeItems_[id]->setBrush(QBrush(kRouteColor));

    if (!path.empty())
    {
        nodeItems_[path.front()]->setBrush(QBrush(kStartColor)); // start = green
        nodeItems_[path.back()]->setBrush(QBrush(kEndColor));    // destination = red
    }
}

void MapView::fitAll()
{
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    fitScale_ = transform().m11();
    zoomed_ = false;
}

void MapView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (!zoomed_)
        fitAll(); // keep the whole map visible unless the user zoomed in
}

void MapView::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
    double target = transform().m11() * factor;
    if (target <= fitScale_ * 1.01)
    { // zoomed all the way out
        fitAll();
        return;
    }
    if (target > 2.0)
        return; // maximum zoom
    scale(factor, factor);
    zoomed_ = true;
}

void MapView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    fitAll();
}