#include "MapView.h"
#include "MapData.h"
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QTextDocument>
#include <QTextOption>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <algorithm>

// Coordinates in locations.csv and MapData.h are in the 8000 x 2024 map space
static const double kMapW = 8000;
static const double kMapH = 2024;

static const QColor kGround("#e2e8f0");
static const QColor kText("#1a202c");
static const QColor kNodeColor("#3182ce");
static const QColor kRouteColor("#ed8936");
static const QColor kStartColor("#48bb78");
static const QColor kEndColor("#f56565");

// ---------------------------------------------------------------------------
//  The campus map, drawn from the tables in MapData.h
// ---------------------------------------------------------------------------

// Writes 'text' so that it fills the area (len x thick) around (cx, cy), turned by 'rot' degrees.
static void addMapText(QGraphicsScene* scene, const QString& text,
                       double cx, double cy, double w, double h, int rot) {
    const bool sideways = (rot == 90 || rot == 270);
    const double len = (sideways ? h : w) * 1.08;           // length along the writing direction
    const double thick = (sideways ? w : h) * 1.5;           // room above and below the letters

    auto* item = new QGraphicsTextItem;
    item->setPlainText(text);
    item->setDefaultTextColor(kText);
    QTextOption opt;
    opt.setAlignment(Qt::AlignHCenter);
    opt.setWrapMode(QTextOption::WordWrap);
    item->document()->setDefaultTextOption(opt);
    item->document()->setDocumentMargin(0);

    QFont font;
    font.setFamilies({"Segoe UI", "Arial"});
    font.setBold(true);
    const QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (int px = 200; px >= 6; px -= 2) {
        font.setPixelSize(px);
        item->setFont(font);
        item->setTextWidth(len);
        QFontMetricsF fm(font);
        double longest = 0;                                  // the longest word must fit on one line
        for (const QString& wd : words) longest = std::max(longest, fm.horizontalAdvance(wd));
        if (longest <= len && item->boundingRect().height() <= thick) break;
    }

    QRectF r = item->boundingRect();
    item->setPos(cx - r.width() / 2, cy - r.height() / 2);
    item->setTransformOriginPoint(r.center());
    item->setRotation(rot);
    item->setZValue(-7);
    scene->addItem(item);
}

static void zoneColors(ZoneKind k, QColor& fill, QColor& border) {
    switch (k) {
    case ROAD:     fill = QColor("#ffffff"); border = QColor("#cbd5e0"); break;
    case SPORT:    fill = QColor("#fed7aa"); border = QColor("#ed8936"); break;
    case BUILDING: fill = QColor("#bee3f8"); border = QColor("#4299e1"); break;
    case FOOD:     fill = QColor("#fefcbf"); border = QColor("#d69e2e"); break;
    case MOSQUE:   fill = QColor("#b2f5ea"); border = QColor("#319795"); break;
    case GARDEN:   fill = QColor("#c6f6d5"); border = QColor("#48bb78"); break;
    case WALL:     fill = QColor("#cbd5e0"); border = QColor("#718096"); break;
    case GATE:     fill = QColor("#ffffff"); border = QColor("#4a5568"); break;
    default:       fill = QColor("#e9d8fd"); border = QColor("#9f7aea"); break;   // stairs, sitting areas
    }
}

static void drawCampus(QGraphicsScene* scene) {
    QPen boundary(QColor("#4a5568"), 3);
    boundary.setCosmetic(true);
    scene->addRect(0, 0, kMapW, kMapH, boundary, QBrush(kGround))->setZValue(-20);

    for (const MapShape& s : kShapes) {
        QColor fill, border;
        zoneColors(s.kind, fill, border);
        QPen pen(border, 1.2);
        pen.setCosmetic(true);
        pen.setJoinStyle(Qt::RoundJoin);

        QPainterPath path;
        QRectF r(s.x1, s.y1, s.x2 - s.x1, s.y2 - s.y1);
        if (s.rounded) path.addRoundedRect(r, 60, 60);
        else           path.addRect(r);
        scene->addPath(path, pen, QBrush(fill))->setZValue(s.kind == ROAD ? -9 : -8);
    }

    for (const MapText& t : kTexts)
        if (*t.text) addMapText(scene, QString::fromUtf8(t.text), t.cx, t.cy, t.w, t.h, t.rot);
}

// ---------------------------------------------------------------------------

static bool isDoor(const Location& l) { return l.name.rfind("~door", 0) == 0; }

// Roads are solid lines; the short links from a place or a door to the road are dashed.
static QPen basePen(bool access) {
    QPen p(access ? QColor(113, 128, 150, 220) : QColor(43, 108, 176, 235), access ? 1.5 : 2.5);
    p.setCosmetic(true);
    p.setStyle(access ? Qt::DashLine : Qt::SolidLine);
    p.setCapStyle(Qt::RoundCap);
    p.setJoinStyle(Qt::RoundJoin);
    return p;
}

// Thick orange line for the chosen route
static QPen routePen() {
    QPen p(kRouteColor, 5);
    p.setCosmetic(true);
    p.setCapStyle(Qt::RoundCap);
    p.setJoinStyle(Qt::RoundJoin);
    return p;
}

MapView::MapView(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setBackgroundBrush(QColor("#edf2f7"));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MapView::setGraph(const Graph& g, const QString& dataDir) {
    Q_UNUSED(dataDir);
    scene_->clear();
    nodeItems_.clear();
    nodeColors_.clear();
    edgeItems_.clear();
    nodeItems_.resize(g.size(), nullptr);
    nodeColors_.resize(g.size());
    scene_->setSceneRect(0, 0, kMapW, kMapH);

    drawCampus(scene_);

    // ----- pathways: straight pieces along the walking roads -----
    for (int u = 0; u < g.size(); ++u) {
        for (const auto& e : g.neighbors(u)) {
            if (e.to < u) continue;                       // each edge only once
            const Location& a = g.location(u);
            const Location& b = g.location(e.to);
            bool access = !isJunction(a) || !isJunction(b) || isDoor(a) || isDoor(b);

            auto* line = scene_->addLine(a.x, a.y, b.x, b.y, basePen(access));
            line->setZValue(0);
            line->setData(0, access);                     // remembered for clearRoute()
            edgeItems_[{u, e.to}] = line;
        }
    }

    // ----- nodes: circles for places (name only as a tooltip), tiny dots for road corners / doors -----
    for (int i = 0; i < g.size(); ++i) {
        const Location& loc = g.location(i);

        if (isJunction(loc)) {
            QColor c("#718096");
            auto* dot = scene_->addEllipse(-3, -3, 6, 6, QPen(Qt::white, 1), QBrush(c));
            dot->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            dot->setPos(loc.x, loc.y);
            dot->setZValue(1.5);
            nodeItems_[i] = dot;
            nodeColors_[i] = c;
            continue;
        }

        QString name = QString::fromStdString(loc.name);
        QString tip = name;
        if (name == "Library")               tip += " (basement)";
        if (name == "Multipurpose Building") tip += " (ground floor)";
        if (name == "Cafeteria")             tip += " (1st floor)";

        auto* node = scene_->addEllipse(-7, -7, 14, 14, QPen(Qt::white, 2), QBrush(kNodeColor));
        node->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        node->setPos(loc.x, loc.y);
        node->setZValue(2);
        node->setToolTip(tip);
        nodeItems_[i] = node;
        nodeColors_[i] = kNodeColor;
    }

    fitAll();
}

void MapView::clearRoute() {
    for (auto& kv : edgeItems_) {
        kv.second->setPen(basePen(kv.second->data(0).toBool()));
        kv.second->setZValue(0);
    }
    for (size_t i = 0; i < nodeItems_.size(); ++i)
        if (nodeItems_[i]) nodeItems_[i]->setBrush(QBrush(nodeColors_[i]));
}

void MapView::showRoute(const std::vector<int>& path) {
    clearRoute();

    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int a = std::min(path[i], path[i + 1]);
        int b = std::max(path[i], path[i + 1]);
        auto it = edgeItems_.find({a, b});
        if (it != edgeItems_.end()) {
            it->second->setPen(routePen());
            it->second->setZValue(0.5);
        }
    }

    for (int id : path)
        if (id >= 0 && id < static_cast<int>(nodeItems_.size()))
            nodeItems_[id]->setBrush(QBrush(kRouteColor));

    if (!path.empty()) {
        nodeItems_[path.front()]->setBrush(QBrush(kStartColor));   // start = green
        nodeItems_[path.back()]->setBrush(QBrush(kEndColor));      // destination = red
    }
}

void MapView::fitAll() {
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    fitScale_ = transform().m11();
    zoomed_ = false;
}

void MapView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (!zoomed_) fitAll();            // keep the whole map visible unless the user zoomed in
}

void MapView::wheelEvent(QWheelEvent* event) {
    double factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
    double target = transform().m11() * factor;
    if (target <= fitScale_ * 1.01) {  // zoomed all the way out
        fitAll();
        return;
    }
    if (target > 3.0) return;          // maximum zoom
    scale(factor, factor);
    zoomed_ = true;
}

void MapView::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    fitAll();
}
