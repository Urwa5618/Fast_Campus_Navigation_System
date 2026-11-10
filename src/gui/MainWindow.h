#pragma once
#include <QMainWindow>
#include "Graph.h"

class MapView;
class QComboBox;
class QRadioButton;
class QPushButton;
class QListWidget;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(const Graph& graph, const QString& dataDir, QWidget* parent = nullptr);

private slots:
    void findRoute();
    void clearAll();

private:
    Graph graph_;
    MapView* map_;
    QComboBox* fromBox_;
    QComboBox* toBox_;
    QRadioButton* dijkstraRadio_;
    QRadioButton* bfsRadio_;
    QPushButton* findBtn_;
    QPushButton* clearBtn_;
    QListWidget* stepsList_;
    QLabel* summaryLabel_;
};
