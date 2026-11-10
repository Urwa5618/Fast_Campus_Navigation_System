#include "DataLoader.h"
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static vector<string> splitCsv(const string& line) {
    vector<string> parts;
    stringstream ss(line);
    string item;
    while (getline(ss, item, ',')) parts.push_back(trim(item));
    return parts;
}

bool loadGraph(const string& locationsFile,
               const string& edgesFile,
               Graph& g,
               string& error) {
    string line;

    ifstream locIn(locationsFile);
    if (!locIn) {
        error = "Cannot open file: " + locationsFile;
        return false;
    }
    getline(locIn, line);
    int lineNo = 1;
    while (getline(locIn, line)) {
        lineNo++;
        if (trim(line).empty()) continue;
        vector<string> p = splitCsv(line);
        if (p.size() != 4) {
            error = locationsFile + " line " + to_string(lineNo) + ": expected 4 fields";
            return false;
        }
        try {
            int id = stoi(p[0]);
            double x = stod(p[2]);
            double y = stod(p[3]);
            if (id != g.size()) {
                error = locationsFile + " line " + to_string(lineNo) +
                        ": ids must be 0,1,2,... in order";
                return false;
            }
            g.addLocation(p[1], x, y);
        } catch (...) {
            error = locationsFile + " line " + to_string(lineNo) + ": invalid number";
            return false;
        }
    }

    ifstream edgeIn(edgesFile);
    if (!edgeIn) {
        error = "Cannot open file: " + edgesFile;
        return false;
    }
    getline(edgeIn, line);
    lineNo = 1;
    while (getline(edgeIn, line)) {
        lineNo++;
        if (trim(line).empty()) continue;
        vector<string> p = splitCsv(line);
        if (p.size() != 3) {
            error = edgesFile + " line " + to_string(lineNo) + ": expected 3 fields";
            return false;
        }
        try {
            int a = stoi(p[0]);
            int b = stoi(p[1]);
            double d = stod(p[2]);
            if (a < 0 || b < 0 || a >= g.size() || b >= g.size()) {
                error = edgesFile + " line " + to_string(lineNo) + ": unknown location id";
                return false;
            }
            if (d <= 0) {
                error = edgesFile + " line " + to_string(lineNo) + ": distance must be > 0";
                return false;
            }
            g.addEdge(a, b, d);
        } catch (...) {
            error = edgesFile + " line " + to_string(lineNo) + ": invalid number";
            return false;
        }
    }
    return true;
}
