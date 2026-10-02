// Loader for the 9th DIMACS Implementation Challenge shortest-path format (.gr).
//   c <comment>
//   p sp <nodes> <arcs>
//   a <from> <to> <weight>        (1-based node ids, directed arcs)
// Road networks (e.g. USA-road-d.NY.gr) list both directions explicitly, so the
// graph is loaded as directed. Download: http://www.diag.uniroma1.it/challenge9/download.shtml
#pragma once
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pf/graph.hpp"

namespace pf {

inline CSRGraph load_dimacs_gr(std::istream& in) {
    std::string line;
    std::size_t n = 0;
    std::vector<Edge> edges;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == 'c') continue;
        std::istringstream ss(line);
        char tag;
        ss >> tag;
        if (tag == 'p') {
            std::string kind;
            std::size_t m = 0;
            ss >> kind >> n >> m;
            if (!ss) throw std::runtime_error("bad DIMACS 'p' line: " + line);
            edges.reserve(m);
        } else if (tag == 'a') {
            unsigned long long u, v, w;
            ss >> u >> v >> w;
            if (!ss || u == 0 || v == 0 || (n && (u > n || v > n)))
                throw std::runtime_error("bad DIMACS 'a' line: " + line);
            edges.push_back({static_cast<NodeId>(u - 1), static_cast<NodeId>(v - 1),
                             static_cast<std::uint32_t>(w)});
        }
    }
    if (n == 0) throw std::runtime_error("DIMACS file has no 'p sp' header");
    return CSRGraph::from_edges(n, edges, /*symmetrize=*/false);
}

inline CSRGraph load_dimacs_gr_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open " + path);
    return load_dimacs_gr(f);
}

}  // namespace pf
