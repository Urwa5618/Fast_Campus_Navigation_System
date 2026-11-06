#include "MainWindow.h"
#include "MapView.h"

#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

MainWindow::MainWindow(const Graph &graph, QWidget *parent)
    : QMainWindow(parent), graph_(graph)
{

    setWindowTitle("FAST Campus Navigation System");
    resize(1200, 700);

    // ---------- left: map ----------
    map_ = new MapView(this);
    map_->setGraph(graph_);

    // ---------- right: controls ----------
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

    dijkstraRadio_ = new QRadioButton("Shortest distance (Dijkstra)");
    bfsRadio_ = new QRadioButton("Fewest stops (BFS)");
    dijkstraRadio_->setChecked(true);

    auto *algoBox = new QGroupBox("Algorithm");
    auto *algoLayout = new QVBoxLayout(algoBox);
    algoLayout->addWidget(dijkstraRadio_);
    algoLayout->addWidget(bfsRadio_);

    findBtn_ = new QPushButton("Find Route");
    clearBtn_ = new QPushButton("Clear");

    stepsList_ = new QListWidget;
    summaryLabel_ = new QLabel("Choose a start and destination.");
    summaryLabel_->setWordWrap(true);

    auto *panel = new QWidget;
    panel->setFixedWidth(320);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->addWidget(new QLabel("<b>From</b>"));
    panelLayout->addWidget(fromBox_);
    panelLayout->addWidget(new QLabel("<b>To</b>"));
    panelLayout->addWidget(toBox_);
    panelLayout->addWidget(algoBox);
    panelLayout->addWidget(findBtn_);
    panelLayout->addWidget(clearBtn_);
    panelLayout->addWidget(new QLabel("<b>Route steps</b>"));
    panelLayout->addWidget(stepsList_, 1);
    panelLayout->addWidget(summaryLabel_);

    // ---------- put both together ----------
    auto *central = new QWidget;
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(map_, 1);
    mainLayout->addWidget(panel);
    setCentralWidget(central);

    // Day 4: buttons only show a message. Real routing is added on Day 5.
    connect(findBtn_, &QPushButton::clicked, this, [this]()
            { summaryLabel_->setText("Routing will be connected on Day 5."); });
    connect(clearBtn_, &QPushButton::clicked, this, [this]()
            {
        stepsList_->clear();
        fromBox_->setCurrentIndex(-1);
        toBox_->setCurrentIndex(-1);
        summaryLabel_->setText("Choose a start and destination."); });
}