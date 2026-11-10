#include "Graph.h"
#include "DataLoader.h"
#include "Algorithms.h"
#include <iostream>
#include <cmath>
#include <vector>

using namespace std;

static int passed = 0, failed = 0;

static void check(const string& name, bool ok) {
    cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (ok) passed++; else failed++;
}

static bool same(double a, double b) { return fabs(a - b) < 1e-9; }

// Only the named places (not road corners / doors) are printed
static string placesOnly(const Graph& g, const RouteResult& r) {
    if (!r.found) return "(no route)";
    string s;
    for (int id : r.path) {
        if (isJunction(g.location(id))) continue;
        if (!s.empty()) s += " -> ";
        s += g.location(id).name;
    }
    return s;
}

// Does the route pass through the node with this exact name?
static bool passes(const Graph& g, const RouteResult& r, const string& name) {
    for (int id : r.path)
        if (g.location(id).name == name) return true;
    return false;
}

int main(int argc, char* argv[]) {
    string dir = (argc > 1) ? argv[1] : "data";

    Graph g;
    string error;
    if (!loadGraph(dir + "/locations.csv", dir + "/edges.csv", g, error)) {
        cout << "LOAD FAILED: " << error << "\n";
        return 1;
    }

    int places = 0;
    for (int i = 0; i < g.size(); i++) if (!isJunction(g.location(i))) places++;
    cout << "Loaded " << g.size() << " nodes (" << places << " places, "
         << g.size() - places << " road corners / doors)\n\n";

    int entrance = g.idOf("Entrance");
    int ee       = g.idOf("EE Building");
    int cs       = g.idOf("CS Building");
    int shawarma = g.idOf("Shawarma Shop");
    int mosque   = g.idOf("Mosque");
    int multi    = g.idOf("Multipurpose Building");
    int library  = g.idOf("Library");
    int futsal   = g.idOf("Futsal Court");
    int basket   = g.idOf("Basketball / Volleyball Court");
    int dhaba    = g.idOf("FAST Dhaba");
    int juice    = g.idOf("Juice Corner");

    RouteResult d, b;

    // 1. Graph is connected
    {
        bool ok = true;
        for (int i = 0; i < g.size(); i++)
            if (!dijkstra(g, entrance, i).found) ok = false;
        check("1. Every node can be reached from the Entrance", ok);
    }

    // 2. same source and destination
    d = dijkstra(g, entrance, entrance);
    check("2. Entrance -> Entrance (0 m, 0 hops)", d.found && same(d.totalDistance, 0) && d.hops == 0);

    // 3. Shawarma Shop: goes straight along the road, NOT through the other shops
    d = dijkstra(g, entrance, shawarma);
    cout << "  Entrance -> Shawarma: " << placesOnly(g, d) << "  [" << d.totalDistance << " m]\n";
    check("3. Entrance -> Shawarma Shop does not pass through any other shop",
          d.found && placesOnly(g, d) == "Entrance -> Shawarma Shop");

    // 4. CS Building is reached through different doors depending on where you come from
    d = dijkstra(g, mosque, cs);
    check("4a. Mosque -> CS Building enters by the back door", passes(g, d, "~door-cs-back"));
    d = dijkstra(g, basket, cs);
    check("4b. Basketball Court -> CS Building enters by the bottom door (stairs)",
          passes(g, d, "~door-cs-bottom"));
    d = dijkstra(g, dhaba, cs);
    check("4c. FAST Dhaba -> CS Building enters by the left door", passes(g, d, "~door-cs-left"));
    d = dijkstra(g, entrance, cs);
    check("4d. Entrance -> CS Building enters by the right door", passes(g, d, "~door-cs-right"));

    // 5. Multipurpose Building: front, back and west doors
    d = dijkstra(g, entrance, library);
    check("5a. Entrance -> Library enters through the MultiPurpose east door (D)",
          passes(g, d, "~door-mp-east"));
    d = dijkstra(g, mosque, multi);
    check("5b. Mosque -> Multipurpose Building enters by the east door (D)",
          passes(g, d, "~door-mp-east"));
    d = dijkstra(g, futsal, multi);
    check("5c. Futsal Court -> Multipurpose Building enters by the west door",
          passes(g, d, "~door-mp-west"));

    // 5d. Library (basement), MultiPurpose (ground floor) and Cafeteria (1st floor) share one entrance,
    //     and the distances between them are the ones that were measured
    {
        int cafeteria = g.idOf("Cafeteria");
        RouteResult a1 = dijkstra(g, multi, library), a2 = dijkstra(g, library, cafeteria), a3 = dijkstra(g, cafeteria, dhaba);
        check("5d. MultiPurpose -> Library = 18 m, Library -> Cafeteria = 25 m",
              same(a1.totalDistance, 18) && same(a2.totalDistance, 25));
        check("5e. Cafeteria -> FAST Dhaba is about 50 m (measured 50 m)",
              a3.found && a3.totalDistance >= 45 && a3.totalDistance <= 60 && passes(g, a3, "~door-mp-east"));
    }

    // 6. EE Building: front, top and bottom doors
    d = dijkstra(g, juice, ee);
    check("6a. Juice Corner -> EE Building enters by the top door (stairs)",
          passes(g, d, "~door-ee-top"));
    d = dijkstra(g, entrance, ee);
    check("6b. Entrance -> EE Building uses one of its doors",
          passes(g, d, "~door-ee-front") || passes(g, d, "~door-ee-bottom"));

    // 7. Routes only pass through road corners and doors, never through another place
    {
        int bad = 0;
        for (int i = 0; i < g.size(); i++) {
            for (int j = 0; j < g.size(); j++) {
                RouteResult r = dijkstra(g, i, j);
                RouteResult q = bfs(g, i, j);
                for (size_t k = 1; k + 1 < r.path.size(); k++)
                    if (!isJunction(g.location(r.path[k]))) bad++;
                for (size_t k = 1; k + 1 < q.path.size(); k++)
                    if (!isJunction(g.location(q.path[k]))) bad++;
            }
        }
        check("7. No route cuts through a building or shop on its way to somewhere else", bad == 0);
    }

    // 7b. Basketball Court -> Mosque must go round the CS Building by road
    d = dijkstra(g, basket, mosque);
    check("7b. Basketball Court -> Mosque does not use the CS Building as a shortcut",
          !passes(g, d, "CS Building"));

    // 8. Symmetry
    d = dijkstra(g, library, entrance);
    RouteResult d2 = dijkstra(g, entrance, library);
    check("8. Library -> Entrance equals Entrance -> Library", same(d.totalDistance, d2.totalDistance));

    // 9. Invalid ids
    check("9a. Invalid source id gives no route", !dijkstra(g, -1, 3).found && !bfs(g, -1, 3).found);
    check("9b. Invalid destination id gives no route", !dijkstra(g, 0, 999).found && !bfs(g, 0, 999).found);

    // 10. Unreachable node
    Graph tiny;
    int a = tiny.addLocation("A", 0, 0);
    int c = tiny.addLocation("B", 0, 0);
    int iso = tiny.addLocation("Isolated", 0, 0);
    tiny.addEdge(a, c, 10);
    check("10. Unreachable node gives found = false",
          !dijkstra(tiny, a, iso).found && !bfs(tiny, a, iso).found);

    // 11. BFS vs Dijkstra on all pairs of places
    int bad = 0, pairs = 0, different = 0;
    for (int i = 0; i < g.size(); i++) {
        if (isJunction(g.location(i))) continue;
        for (int j = 0; j < g.size(); j++) {
            if (isJunction(g.location(j))) continue;
            RouteResult dd = dijkstra(g, i, j);
            RouteResult bb = bfs(g, i, j);
            pairs++;
            if (!dd.found || !bb.found) { bad++; continue; }
            if (dd.totalDistance > bb.totalDistance + 1e-9) bad++;
            if (bb.hops > dd.hops) bad++;
            if (dd.path != bb.path) different++;
        }
    }
    cout << "  " << pairs << " pairs of places checked, BFS and Dijkstra routes differ in "
         << different << " of them\n";
    check("11. All pairs: Dijkstra distance <= BFS distance, BFS hops <= Dijkstra hops", bad == 0);

    // Show the pair where the two algorithms disagree the most (good for the demo)
    {
        int bi = -1, bj = -1;
        double bestDiff = 0;
        for (int i = 0; i < g.size(); i++) {
            if (isJunction(g.location(i))) continue;
            for (int j = 0; j < g.size(); j++) {
                if (isJunction(g.location(j))) continue;
                RouteResult dd = dijkstra(g, i, j), bb = bfs(g, i, j);
                if (dd.found && bb.found && bb.totalDistance - dd.totalDistance > bestDiff) {
                    bestDiff = bb.totalDistance - dd.totalDistance;
                    bi = i; bj = j;
                }
            }
        }
        if (bi >= 0) {
            RouteResult dd = dijkstra(g, bi, bj), bb = bfs(g, bi, bj);
            cout << "\nDemo pair: " << g.location(bi).name << " -> " << g.location(bj).name << "\n";
            cout << "  Dijkstra: " << dd.totalDistance << " m, " << dd.hops << " segments\n";
            cout << "  BFS     : " << bb.totalDistance << " m, " << bb.hops << " segments\n";
        }
    }

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
