#include "MainWindow.h"
#include "MapView.h"
#include "Algorithms.h"

#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>

// Distance of the direct edge u -> v
static double segmentDistance(const Graph &g, int u, int v)
{
    double best = -1;
    for (const auto &e : g.neighbors(u))
        if (e.to == v && (best < 0 || e.weight < best))
            best = e.weight;
    return best;
}

MainWindow::MainWindow(const Graph &graph, const QString &dataDir, QWidget *parent)
    : QMainWindow(parent), graph_(graph)
{

    setWindowTitle("FAST Campus Navigation System");
    resize(1200, 610);

    // ---------- top: map ----------
    map_ = new MapView(this);
    map_->setGraph(graph_, dataDir);

    // ---------- bottom: controls ----------
    fromBox_ = new QComboBox;
    toBox_ = new QComboBox;
    for (int i = 0; i < graph_.size(); ++i)
    {
        QString name = QString::fromStdString(graph_.location(i).name);
        fromBox_->addItem(name, i); // the id is stored with each item
        toBox_->addItem(name, i);
    }
    fromBox_->setPlaceholderText("Select starting point");
    toBox_->setPlaceholderText("Select destination");
    fromBox_->setCurrentIndex(-1);
    toBox_->setCurrentIndex(-1);

    auto *selectBox = new QGroupBox("Route");
    auto *selectLayout = new QVBoxLayout(selectBox);
    selectLayout->addWidget(new QLabel("From"));
    selectLayout->addWidget(fromBox_);
    selectLayout->addWidget(new QLabel("To"));
    selectLayout->addWidget(toBox_);

    dijkstraRadio_ = new QRadioButton("Shortest distance (Dijkstra)");
    bfsRadio_ = new QRadioButton("Fewest stops (BFS)");
    dijkstraRadio_->setChecked(true);
    findBtn_ = new QPushButton("Find Route");
    clearBtn_ = new QPushButton("Clear");

    auto *algoBox = new QGroupBox("Algorithm");
    auto *algoLayout = new QVBoxLayout(algoBox);
    algoLayout->addWidget(dijkstraRadio_);
    algoLayout->addWidget(bfsRadio_);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(findBtn_);
    buttonRow->addWidget(clearBtn_);
    algoLayout->addLayout(buttonRow);
    algoLayout->addStretch();

    stepsList_ = new QListWidget;
    auto *stepsBox = new QGroupBox("Route steps");
    auto *stepsLayout = new QVBoxLayout(stepsBox);
    stepsLayout->addWidget(stepsList_);

    summaryLabel_ = new QLabel("Choose a start and destination.");
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *summaryBox = new QGroupBox("Summary");
    auto *summaryLayout = new QVBoxLayout(summaryBox);
    summaryLayout->addWidget(summaryLabel_);

    auto *controls = new QWidget;
    controls->setFixedHeight(200);
    auto *controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(6, 0, 6, 6);
    controlsLayout->addWidget(selectBox, 2);
    controlsLayout->addWidget(algoBox, 2);
    controlsLayout->addWidget(stepsBox, 4);
    controlsLayout->addWidget(summaryBox, 2);

    // ---------- put both together ----------
    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);
    mainLayout->addWidget(map_, 1);
    mainLayout->addWidget(controls);
    setCentralWidget(central);

    connect(findBtn_, &QPushButton::clicked, this, &MainWindow::findRoute);
    connect(clearBtn_, &QPushButton::clicked, this, &MainWindow::clearAll);
}

void MainWindow::findRoute()
{
    // ----- input validation -----
    if (fromBox_->currentIndex() < 0 || toBox_->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Missing selection",
                             "Please select both a starting point and a destination.");
        return;
    }
    int src = fromBox_->currentData().toInt();
    int dst = toBox_->currentData().toInt();
    if (src == dst)
    {
        QMessageBox::information(this, "Same location",
                                 "The starting point and destination are the same. "
                                 "Please choose two different locations.");
        return;
    }

    // ----- run the chosen algorithm -----
    bool useDijkstra = dijkstraRadio_->isChecked();
    RouteResult r = useDijkstra ? dijkstra(graph_, src, dst) : bfs(graph_, src, dst);

    stepsList_->clear();
    map_->clearRoute();

    if (!r.found)
    {
        summaryLabel_->setText("No route is available between these two locations.");
        return;
    }

    // ----- show the route on the map -----
    map_->showRoute(r.path);

    // ----- fill the step list -----
    for (size_t i = 0; i < r.path.size(); ++i)
    {
        QString name = QString::fromStdString(graph_.location(r.path[i]).name);
        QString text = QString("%1. %2").arg(i + 1).arg(name);
        if (i == 0)
            text += "  (start)";
        else
            text += QString("  (+%1 m)").arg(segmentDistance(graph_, r.path[i - 1], r.path[i]));
        if (i + 1 == r.path.size())
            text += "  (destination)";
        stepsList_->addItem(text);
    }

    summaryLabel_->setText(
        QString("<b>%1</b><br>Total distance: <b>%2 m</b><br>Stops: <b>%3</b> (%4 steps)")
            .arg(useDijkstra ? "Shortest distance (Dijkstra)" : "Fewest stops (BFS)")
            .arg(r.totalDistance)
            .arg(r.path.size())
            .arg(r.hops));
}

void MainWindow::clearAll()
{
    map_->clearRoute();
    stepsList_->clear();
    fromBox_->setCurrentIndex(-1);
    toBox_->setCurrentIndex(-1);
    summaryLabel_->setText("Choose a start and destination.");
}