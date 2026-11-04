#pragma once
#include <string>
#include "Graph.h"

// Reads locations and edges from CSV files into the graph.
// Returns true on success. On failure returns false and fills 'error'.
bool loadGraph(const std::string &locationsFile, const std::string &edgesFile, Graph &g, std::string &error);