#pragma once
#include <vector>
#include "Graph.h"

struct RouteResult {
    bool found = false;
    std::vector<int> path;        // location ids in order: source ... destination
    double totalDistance = 0;     // metres, sum of real edge weights along the path
    int hops = 0;                 // number of edges in the path
};

// Fewest edges (ignores distances while searching)
RouteResult bfs(const Graph& g, int src, int dst);

// Minimum total distance (weights must be non-negative)
RouteResult dijkstra(const Graph& g, int src, int dst);
