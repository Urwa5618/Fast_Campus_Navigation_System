#include "Graph.h"

using namespace std;

int Graph::addLocation(const string &name, double x, double y)
{
    int id = static_cast<int>(locations_.size());
    locations_.push_back({id, name, x, y});
    adj_.emplace_back(); // empty neighbour list for the new node
    nameToId_[name] = id;
    return id;
}

void Graph::addEdge(int a, int b, double dist)
{
    adj_[a].push_back({b, dist}); // a -> b
    adj_[b].push_back({a, dist}); // b -> a (paths can be walked both ways)
}

int Graph::idOf(const string &name) const
{
    auto it = nameToId_.find(name);
    return it == nameToId_.end() ? -1 : it->second;
}

const vector<Edge> &Graph::neighbors(int id) const { return adj_[id]; }
const Location &Graph::location(int id) const { return locations_[id]; }
int Graph::size() const { return static_cast<int>(locations_.size()); }