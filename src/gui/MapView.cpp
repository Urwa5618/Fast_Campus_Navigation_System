#include "MapView.h"
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <algorithm>
#include <cmath>

// Coordinates in locations.csv / paths.csv use an 8000 x 2024 map space
static const double kMapW = 8000;
static const double kMapH = 2024;

static const QColor kNodeColor("#2b6cb0");
static const QColor kRouteColor("#ed8936");
static const QColor kStartColor("#38a169");
static const QColor kEndColor("#e53e3e");

// ---------------------------------------------------------------------------
//  The campus drawn from code (layout taken from the FAST campus map)
// ---------------------------------------------------------------------------
enum ZoneKind
{
    ROAD,
    SPORT,
    BUILDING,
    FOOD,
    MOSQUE,
    GARDEN,
    OTHER
};

struct Zone
{
    double x1, y1, x2, y2;
    ZoneKind kind;
    const char *label; // "" = no label (locations get their own label)
};

static const Zone kZones[] = {
    // ---------- walking roads ----------
    {1880, 88, 4322, 212, ROAD, ""},    // top road
    {3877, 88, 4322, 1862, ROAD, ""},   // big vertical road
    {1662, 88, 1880, 212, ROAD, ""},    // link: left road to top road
    {1662, 100, 1775, 1462, ROAD, ""},  // left vertical road
    {1775, 1332, 3877, 1395, ROAD, ""}, // lower-left horizontal road
    {1330, 1425, 1720, 1462, ROAD, ""}, // small road to Badminton
    {4322, 517, 6332, 667, ROAD, ""},   // middle road (Mosque / CS)
    {4322, 1507, 6332, 1575, ROAD, ""}, // lower road (above Basketball)
    {6332, 155, 6402, 1892, ROAD, ""},  // right vertical road
    {6302, 1892, 6432, 2012, ROAD, ""}, // entrance
    {6402, 155, 7600, 370, ROAD, ""},   // top-right road (shops side)
    {6402, 1772, 7667, 1875, ROAD, ""}, // bottom-right road
    {6402, 1192, 7195, 1230, ROAD, ""}, // path to EE Building

    // ---------- sports ----------
    {168, 112, 1632, 352, SPORT, ""},    // Sports Room
    {168, 377, 1632, 1320, SPORT, ""},   // Futsal Court
    {168, 1357, 1330, 1850, SPORT, ""},  // Badminton Court
    {4530, 1587, 5325, 1850, SPORT, ""}, // Basketball / Volleyball Court

    // ---------- buildings ----------
    {1880, 230, 3852, 1042, BUILDING, ""}, // MultiPurpose Building
    {4345, 675, 6320, 1327, BUILDING, ""}, // CS Building
    {7195, 437, 7955, 1750, BUILDING, ""}, // EE Building
    {6445, 452, 6895, 790, BUILDING, "Lab 5 / Academic Block 3"},

    // ---------- food ----------
    {4362, 82, 4622, 495, FOOD, ""},  // FAST Dhaba
    {6905, 452, 7190, 792, FOOD, ""}, // Tasty Hut
    {6462, 52, 6545, 145, FOOD, ""},  // Shawarma
    {6550, 52, 6637, 145, FOOD, ""},  // Fries
    {6640, 52, 6727, 145, FOOD, ""},  // Juice Corner
    {6730, 52, 6815, 145, FOOD, ""},  // Biryani / Chai
    {6817, 52, 6902, 145, FOOD, ""},  // Print Shop

    // ---------- mosque ----------
    {5125, 78, 5919, 503, MOSQUE, ""},
    {5927, 78, 6302, 503, MOSQUE, "Wudu Area"},

    // ---------- gardens ----------
    {4632, 82, 5115, 495, GARDEN, "Garden"},
    {1707, 1415, 3850, 1850, GARDEN, "Garden"},
    {1330, 1480, 1705, 1820, GARDEN, "Garden"},
    {4532, 1340, 5220, 1495, GARDEN, "Garden"},
    {5450, 1340, 6320, 1495, GARDEN, "Garden"},
    {5522, 1590, 6320, 1850, GARDEN, "Garden"},
    {6418, 795, 7125, 1187, GARDEN, "Garden"},
    {6418, 1235, 7125, 1750, GARDEN, "Garden"},

    // ---------- stairs and sitting areas ----------
    {1880, 1067, 3485, 1310, OTHER, "Stairs"},
    {3507, 1067, 3850, 1310, OTHER, "Circle Sitting"},
    {4347, 1340, 4525, 1495, OTHER, "Sitting"},
    {5247, 1342, 5420, 1490, OTHER, "Stairs"},
    {4337, 1587, 4525, 1850, OTHER, "Grass"},
    {5337, 1587, 5510, 1850, OTHER, "Fountain"},
    {7603, 155, 7790, 440, OTHER, "Stairs"},
    {7690, 1772, 7840, 1880, OTHER, "Stairs"},
};

static void zoneColors(ZoneKind k, QColor &fill, QColor &border)
{
    switch (k)
    {
    case ROAD:
        fill = QColor("#ffffff");
        border = QColor("#cbd5e0");
        break;
    case SPORT:
        fill = QColor("#fed7aa");
        border = QColor("#ed8936");
        break;
    case BUILDING:
        fill = QColor("#bee3f8");
        border = QColor("#4299e1");
        break;
    case FOOD:
        fill = QColor("#fefcbf");
        border = QColor("#d69e2e");
        break;
    case MOSQUE:
        fill = QColor("#b2f5ea");
        border = QColor("#319795");
        break;
    case GARDEN:
        fill = QColor("#c6f6d5");
        border = QColor("#48bb78");
        break;
    default:
        fill = QColor("#e9d8fd");
        border = QColor("#9f7aea");
        break;
    }
}

static void drawCampus(QGraphicsScene *scene)
{
    // ground + campus boundary
    QPen boundary(QColor("#4a5568"), 3);
    boundary.setCosmetic(true);
    scene->addRect(0, 0, kMapW, kMapH, boundary, QBrush(QColor("#e2e8f0")))->setZValue(-20);

    for (const Zone &z : kZones)
    {
        QColor fill, border;
        zoneColors(z.kind, fill, border);
        QPen pen(border, 1);
        pen.setCosmetic(true);

        auto *rect = scene->addRect(z.x1, z.y1, z.x2 - z.x1, z.y2 - z.y1, pen, QBrush(fill));
        rect->setZValue(z.kind == ROAD ? -9 : -8);

        if (z.label && *z.label)
        {
            auto *text = new QGraphicsSimpleTextItem(QString::fromUtf8(z.label));
            QFont font;
            font.setBold(true);
            int px = 80;
            double maxW = (z.x2 - z.x1) * 0.9;
            do
            { // shrink until the text fits the area
                font.setPixelSize(px);
                text->setFont(font);
                px -= 4;
            } while (text->boundingRect().width() > maxW && px >= 28);
            text->setBrush(QColor("#2d3748"));
            QRectF r = text->boundingRect();
            text->setPos((z.x1 + z.x2) / 2 - r.width() / 2, (z.y1 + z.y2) / 2 - r.height() / 2);
            text->setZValue(-7);
            scene->addItem(text);
        }
    }
}

// ---------------------------------------------------------------------------

// Thin line for a normal pathway (keeps its thickness when zooming)
static QPen basePen()
{
    QPen p(QColor(43, 108, 176, 200), 2);
    p.setCosmetic(true);
    p.setJoinStyle(Qt::RoundJoin);
    p.setCapStyle(Qt::RoundCap);
    return p;
}

// Thick orange line for the chosen route
static QPen routePen()
{
    QPen p(kRouteColor, 5);
    p.setCosmetic(true);
    p.setJoinStyle(Qt::RoundJoin);
    p.setCapStyle(Qt::RoundCap);
    return p;
}

// A small text label on a box, centred on (0,0) of the returned item.
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

// paths.csv line:  from,to,x:y;x:y;...   (the corners a walker turns at between two locations)
static std::map<std::pair<int, int>, std::vector<QPointF>> loadBends(const QString &file)
{
    std::map<std::pair<int, int>, std::vector<QPointF>> bends;
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return bends; // no file = straight lines
    QTextStream in(&f);
    in.readLine(); // header
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList parts = line.split(',');
        if (parts.size() < 2)
            continue;
        int a = parts[0].trimmed().toInt();
        int b = parts[1].trimmed().toInt();
        std::vector<QPointF> pts;
        if (parts.size() >= 3 && !parts[2].trimmed().isEmpty())
        {
            const QStringList corners = parts[2].trimmed().split(';');
            for (const QString &c : corners)
            {
                QStringList xy = c.split(':');
                if (xy.size() == 2)
                    pts.push_back(QPointF(xy[0].toDouble(), xy[1].toDouble()));
            }
        }
        if (a > b)
        { // store as (smaller, larger)
            std::swap(a, b);
            std::reverse(pts.begin(), pts.end());
        }
        bends[{a, b}] = pts;
    }
    return bends;
}

MapView::MapView(QWidget *parent) : QGraphicsView(parent)
{
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor("#edf2f7"));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MapView::setGraph(const Graph &g, const QString &dataDir)
{
    scene_->clear();
    nodeItems_.clear();
    edgeItems_.clear();
    edgeTags_.clear();
    nodeItems_.resize(g.size(), nullptr);
    scene_->setSceneRect(0, 0, kMapW, kMapH);

    drawCampus(scene_);

    auto bends = loadBends(dataDir + "/paths.csv");

    QFont edgeFont;
    edgeFont.setPointSize(8);
    edgeFont.setBold(true);
    QFont nameFont;
    nameFont.setPointSize(8);
    nameFont.setBold(true);

    // ----- pathways: follow the walking roads through the corner points -----
    for (int u = 0; u < g.size(); ++u)
    {
        for (const auto &e : g.neighbors(u))
        {
            if (e.to < u)
                continue; // each edge only once
            const Location &a = g.location(u);
            const Location &b = g.location(e.to);

            QPainterPath path(QPointF(a.x, a.y));
            auto it = bends.find({u, e.to});
            if (it != bends.end())
                for (const QPointF &p : it->second)
                    path.lineTo(p);
            path.lineTo(b.x, b.y);

            auto *item = scene_->addPath(path, basePen());
            item->setZValue(0);
            edgeItems_[{u, e.to}] = item;

            // distance label: hidden until this edge is part of a route
            auto *tag = makeTag(QString::number(e.weight) + " m", edgeFont,
                                QColor("#1a202c"), QColor("#fbd38d"), nullptr);
            scene_->addItem(tag);
            tag->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            tag->setPos(path.pointAtPercent(0.5));
            tag->setZValue(1);
            tag->setVisible(false);
            edgeTags_[{u, e.to}] = tag;
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

        // Nodes close together in a row (e.g. the shops) get labels at different heights
        int level = 0;
        for (int j = 0; j < g.size(); ++j)
        {
            if (j == i)
                continue;
            const Location &o = g.location(j);
            if (std::abs(o.y - loc.y) < 150 && std::abs(o.x - loc.x) < 900 &&
                (o.x < loc.x || (o.x == loc.x && j < i)))
                ++level;
        }
        level %= 5;

        auto *tag = makeTag(name, nameFont, QColor("#1a202c"), QColor(255, 255, 255, 230), node);
        tag->setPos(0, 18 + level * 18);
    }

    fitAll();
}

void MapView::clearRoute()
{
    for (auto &kv : edgeItems_)
    {
        kv.second->setPen(basePen());
        kv.second->setZValue(0);
    }
    for (auto &kv : edgeTags_)
        kv.second->setVisible(false);
    for (auto *node : nodeItems_)
        if (node)
            node->setBrush(QBrush(kNodeColor));
}

void MapView::showRoute(const std::vector<int> &path)
{
    clearRoute();

    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
        int a = std::min(path[i], path[i + 1]);
        int b = std::max(path[i], path[i + 1]);
        auto it = edgeItems_.find({a, b});
        if (it != edgeItems_.end())
        {
            it->second->setPen(routePen());
            it->second->setZValue(0.5);
        }
        auto tg = edgeTags_.find({a, b});
        if (tg != edgeTags_.end())
            tg->second->setVisible(true);
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