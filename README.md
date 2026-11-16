# FAST Campus Navigation System with GUI
 
**Course:** Data Structures (Section K), National University of Computer & Emerging Sciences, Karachi Campus
 
**Group members**
 
| Name | Roll No. |
|---|---|
| Urwa Rafique | 25K-3066 |
| Awais Muhammad Khan | 25K-0845 |
 
## About
 
A C++ desktop application that helps students, faculty and visitors find a walking route between places inside the FAST-NUCES Karachi campus. The user picks a starting point and a destination, chooses an algorithm, and the route is highlighted on a map of the campus together with the list of stops and the total distance.
 
The project applies graph data structures and shortest-path algorithms to a real navigation problem.
 
## Features
 
- Interactive map of the campus (buildings, sports areas, gardens, walking roads), drawn from code
- Choose a start and a destination from the list of campus places
- **Dijkstra** finds the shortest route by distance; **BFS** finds the route with the fewest road segments
- The route is highlighted on the map; the steps list shows each place with the distance travelled
- Buildings with several doors (CS, EE, Multipurpose) are entered by the door that is closest to the route
- Zoom with the mouse wheel, drag to move, double-click to fit the whole map
- Input validation: nothing selected, same start and destination, no route available
- Works offline: all data is read from two CSV files
## Data structures and algorithms
 
| Item | What is used |
|---|---|
| Campus | Weighted undirected graph stored as an **adjacency list** (`vector<vector<Edge>>`) |
| Names to ids | `unordered_map<string, int>` |
| BFS | `queue` + `visited[]` + `parent[]` |
| Dijkstra | `priority_queue` (min-heap) + `dist[]` + `parent[]` |
| Route | Rebuilt by walking `parent[]` back from the destination |
 
| | BFS | Dijkstra |
|---|---|---|
| Finds | Fewest edges (road segments) | Smallest total distance |
| Time | O(V + E) | O((V + E) log V) |
| Space | O(V) | O(V) |
 
### How the campus is modelled
 
The graph has two kinds of nodes:
 
- **Places** (18): Entrance, EE Building, CS Building, Multipurpose Building, Library, Cafeteria, FAST Dhaba, Mosque, sports areas, shops, ...
- **Road corners and building doors** (40): extra nodes that are not shown to the user. They make routes follow the walking roads and let a building have several doors.
A place can only be the **start or the end** of a route, never a shortcut in the middle. Routes pass through road corners and doors only, so no route walks through a building or a shop to get somewhere else.
 
Distances are estimated from the map (24 map pixels = 1 metre) and are approximate.
 
## Tools and technologies
 
- C++17
- Qt 6 Widgets (GUI)
- CMake + Ninja
- MinGW 64-bit
- Visual Studio Code
- Windows 64-bit
## Project structure
 
```
FastNav/
├── CMakeLists.txt
├── data/
│   ├── locations.csv      id, name, x, y  (places, road corners, doors)
│   └── edges.csv          from, to, distance_m
└── src/
    ├── core/              plain C++, no Qt
    │   ├── Graph.h / .cpp         adjacency-list graph
    │   ├── DataLoader.h / .cpp    reads the CSV files
    │   └── Algorithms.h / .cpp    bfs() and dijkstra()
    ├── gui/
    │   ├── MainWindow.h / .cpp    window, controls, route steps
    │   ├── MapView.h / .cpp       draws the map and the route
    │   └── MapData.h              the campus map (shapes and names)
    ├── main.cpp
    └── test_core.cpp      console tests for the core
```
 
The `core` folder has no Qt code, so the algorithms can be built and tested without the GUI.
 
## How to build
 
Requirements: Qt 6 (MinGW 64-bit kit), CMake, Ninja, MinGW 64-bit compiler (all of these can be installed with the Qt Online Installer).
 
```powershell
cmake -S . -B build -G Ninja
cmake --build build
```
 
## How to run
 
```powershell
.\build\FastNav.exe          # the application
.\build\test_core.exe        # console tests (21 tests)
```
 
Run both from the project folder, because the program looks for the `data` folder next to the project.
 
## Tests
 
`test_core` checks, among other things:
 
- every node can be reached from the Entrance
- Entrance to Shawarma Shop does not pass through any other shop
- CS, EE and Multipurpose buildings are entered by the right door depending on where the route starts
- no route cuts through a building to reach somewhere else
- Dijkstra's distance is never longer than BFS's distance, for every pair of places
- invalid ids and unreachable nodes are handled
## Screenshots
 
_Add screenshots of the application here (for example in a `docs` folder)._
 
## Limitations
 
- Distances are estimates taken from the map, not surveyed measurements.
- The campus map and the walking network are predefined; there is no live GPS.
