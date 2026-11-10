#include "Algorithms.h"
#include <queue>
#include <limits>
#include <algorithm>

using namespace std;

static double edgeWeight(const Graph& g, int u, int v) {
    double best = numeric_limits<double>::infinity();
    for (const auto& e : g.neighbors(u))
        if (e.to == v && e.weight < best) best = e.weight;
    return best;
}

static RouteResult buildResult(const Graph& g, const vector<int>& parent, int dst) {
    RouteResult r;
    r.found = true;
    for (int v = dst; v != -1; v = parent[v]) r.path.push_back(v);
    reverse(r.path.begin(), r.path.end());
    r.hops = static_cast<int>(r.path.size()) - 1;
    for (size_t i = 0; i + 1 < r.path.size(); i++)
        r.totalDistance += edgeWeight(g, r.path[i], r.path[i + 1]);
    return r;
}

RouteResult bfs(const Graph& g, int src, int dst) {
    RouteResult none;
    int n = g.size();
    if (src < 0 || dst < 0 || src >= n || dst >= n) return none;

    vector<bool> visited(n, false);
    vector<int> parent(n, -1);
    queue<int> q;

    visited[src] = true;
    q.push(src);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        if (u == dst) break;
        // A named place can only be the start or the end of a route, never a shortcut:
        // the walk between places goes through road corners and doors only.
        if (u != src && !isJunction(g.location(u))) continue;
        for (const auto& e : g.neighbors(u)) {
            if (!visited[e.to]) {
                visited[e.to] = true;
                parent[e.to] = u;
                q.push(e.to);
            }
        }
    }

    if (!visited[dst]) return none;
    return buildResult(g, parent, dst);
}

RouteResult dijkstra(const Graph& g, int src, int dst) {
    RouteResult none;
    int n = g.size();
    if (src < 0 || dst < 0 || src >= n || dst >= n) return none;

    const double INF = numeric_limits<double>::infinity();
    vector<double> dist(n, INF);
    vector<int> parent(n, -1);

    priority_queue<pair<double, int>,
                   vector<pair<double, int>>,
                   greater<pair<double, int>>> pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        if (u == dst) break;
        // Same rule as in BFS: places are only start/end points, not shortcuts.
        if (u != src && !isJunction(g.location(u))) continue;
        for (const auto& e : g.neighbors(u)) {
            double nd = d + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }

    if (dist[dst] == INF) return none;
    return buildResult(g, parent, dst);
}
