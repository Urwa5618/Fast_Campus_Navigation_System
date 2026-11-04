#include "Graph.h"
#include "DataLoader.h"
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[])
{
    string dir = (argc > 1) ? argv[1] : "data";

    Graph g;
    string error;
    if (!loadGraph(dir + "/locations.csv", dir + "/edges.csv", g, error))
    {
        cout << "LOAD FAILED: " << error << "\n";
        return 1;
    }

    // 1. Print the adjacency list
    cout << "Loaded " << g.size() << " locations\n\n";
    int edgeCount = 0;
    for (int i = 0; i < g.size(); i++)
    {
        cout << i << " " << g.location(i).name << " -> ";
        for (const auto &e : g.neighbors(i))
        {
            cout << g.location(e.to).name << " (" << e.weight << " m)  ";
            edgeCount++;
        }
        cout << "\n";
    }
    cout << "\nTotal edges: " << edgeCount / 2 << "\n";

    // 2. Connectivity check: can every location be reached from location 0?
    vector<bool> seen(g.size(), false);
    vector<int> stack = {0};
    seen[0] = true;
    int reached = 1;
    while (!stack.empty())
    {
        int u = stack.back();
        stack.pop_back();
        for (const auto &e : g.neighbors(u))
        {
            if (!seen[e.to])
            {
                seen[e.to] = true;
                reached++;
                stack.push_back(e.to);
            }
        }
    }
    cout << "Reachable from " << g.location(0).name << ": "
         << reached << " / " << g.size() << "\n";
    if (reached != g.size())
    {
        cout << "WARNING: these locations are NOT connected:\n";
        for (int i = 0; i < g.size(); i++)
            if (!seen[i])
                cout << "  - " << g.location(i).name << "\n";
    }
    else
    {
        cout << "Graph is fully connected.\n";
    }
    return 0;
}