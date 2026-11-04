#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct Location
{
    int id;
    std::string name;
    double x, y; // pixel position on the map (used by the GUI later)
};

struct Edge
{
    int to;        // id of the connected location
    double weight; // distance in metres
};

class Graph
{
public:
    int addLocation(const std::string &name, double x, double y);
    void addEdge(int a, int b, double dist); // undirected
    int idOf(const std::string &name) const; // -1 if not found
    const std::vector<Edge> &neighbors(int id) const;
    const Location &location(int id) const;
    int size() const;

private:
    std::vector<Location> locations_;
    std::vector<std::vector<Edge>> adj_; // adjacency list
    std::unordered_map<std::string, int> nameToId_;
};