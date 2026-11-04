#include "Graph.h"
#include "DataLoader.h"
#include "Algorithms.h"
#include <iostream>
#include <cmath>

using namespace std;

static int passed = 0, failed = 0;

static void check(const string &name, bool ok)
{
    cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (ok)
        passed++;
    else
        failed++;
}

static bool same(double a, double b) { return fabs(a - b) < 1e-9; }

static string pathToString(const Graph &g, const RouteResult &r)
{
    if (!r.found)
        return "(no route)";
    string s;
    for (size_t i = 0; i < r.path.size(); i++)
    {
        if (i)
            s += " -> ";
        s += g.location(r.path[i]).name;
    }
    return s;
}

static void show(const Graph &g, const string &label, const RouteResult &r)
{
    cout << "  " << label << ": " << pathToString(g, r)
         << "  [" << r.totalDistance << " m, " << r.hops << " hops]\n";
}

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
    cout << "Loaded " << g.size() << " locations\n\n";

    int entrance = g.idOf("Entrance");
    int ee = g.idOf("EE Building");
    int library = g.idOf("Library");
    int dhaba = g.idOf("FAST Dhaba");
    int shawarma = g.idOf("Shawarma Shop");
    int mosque = g.idOf("Mosque");
    int multi = g.idOf("MultiPurpose Building");

    RouteResult d, b;

    // Test 1: same source and destination
    d = dijkstra(g, entrance, entrance);
    b = bfs(g, entrance, entrance);
    check("1. Entrance -> Entrance (distance 0, 0 hops)",
          d.found && b.found && same(d.totalDistance, 0) && d.hops == 0 && b.hops == 0);

    // Test 2: directly connected
    d = dijkstra(g, entrance, ee);
    b = bfs(g, entrance, ee);
    check("2. Entrance -> EE Building (90 m)",
          same(d.totalDistance, 90) && same(b.totalDistance, 90) && d.hops == 1);

    // Test 3: Library via MultiPurpose
    d = dijkstra(g, entrance, library);
    check("3. Entrance -> Library = 183 m (via MultiPurpose)",
          d.found && same(d.totalDistance, 183) && d.hops == 2);

    // Test 4: Dhaba via Juice Corner
    d = dijkstra(g, entrance, dhaba);
    b = bfs(g, entrance, dhaba);
    check("4. Entrance -> FAST Dhaba = 210 m, 2 hops",
          same(d.totalDistance, 210) && b.hops == 2);

    // Test 5: long route into the food area
    d = dijkstra(g, entrance, shawarma);
    check("5. Entrance -> Shawarma Shop = 150 m, 5 hops",
          same(d.totalDistance, 150) && d.hops == 5 && d.path.size() == 6);

    // Test 6: BFS and Dijkstra DIFFER (great for the demo)
    cout << "\nDemo pair: Mosque -> MultiPurpose Building\n";
    d = dijkstra(g, mosque, multi);
    b = bfs(g, mosque, multi);
    show(g, "Dijkstra", d);
    show(g, "BFS     ", b);
    check("6. Mosque -> MultiPurpose: Dijkstra 138 m, BFS 265 m, both 2 hops",
          same(d.totalDistance, 138) && same(b.totalDistance, 265) &&
              d.hops == 2 && b.hops == 2);
    cout << "\n";

    // Test 7: symmetry (A->B same distance as B->A)
    d = dijkstra(g, library, entrance);
    RouteResult d2 = dijkstra(g, entrance, library);
    check("7. Library -> Entrance equals Entrance -> Library",
          same(d.totalDistance, d2.totalDistance));

    // Test 8: invalid ids
    check("8a. Invalid source id gives no route", !dijkstra(g, -1, 3).found && !bfs(g, -1, 3).found);
    check("8b. Invalid destination id gives no route", !dijkstra(g, 0, 999).found && !bfs(g, 0, 999).found);

    // Test 9: unreachable node (small graph with an isolated node)
    Graph tiny;
    int a = tiny.addLocation("A", 0, 0);
    int c = tiny.addLocation("B", 0, 0);
    int iso = tiny.addLocation("Isolated", 0, 0);
    tiny.addEdge(a, c, 10);
    check("9. Unreachable node gives found = false",
          !dijkstra(tiny, a, iso).found && !bfs(tiny, a, iso).found);

    // Test 10: every pair, Dijkstra never longer than BFS, BFS never more hops
    int bad = 0, pairs = 0, different = 0;
    for (int i = 0; i < g.size(); i++)
    {
        for (int j = 0; j < g.size(); j++)
        {
            RouteResult dd = dijkstra(g, i, j);
            RouteResult bb = bfs(g, i, j);
            pairs++;
            if (!dd.found || !bb.found)
            {
                bad++;
                continue;
            }
            if (dd.totalDistance > bb.totalDistance + 1e-9)
                bad++;
            if (bb.hops > dd.hops)
                bad++;
            if (dd.path != bb.path)
                different++;
        }
    }
    cout << "  " << pairs << " pairs checked, BFS and Dijkstra routes differ in "
         << different << " of them\n";
    check("10. All pairs: Dijkstra distance <= BFS distance, BFS hops <= Dijkstra hops",
          bad == 0);

    cout << "\n"
         << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}