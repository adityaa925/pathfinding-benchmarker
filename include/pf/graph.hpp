// Two graph representations behind one tiny concept:
//     std::size_t num_nodes() const;
//     template <class F> void for_each_neighbor(NodeId u, F&& f) const;   // f(NodeId v, double w)
// GridGraph is implicit (no edge storage); CSRGraph is a compressed adjacency list.
#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "pf/common.hpp"

namespace pf {

// ------------------------------------------------------------------ grid
enum class Connectivity { Four, Eight };

// Cells are NodeId = y * width + x. Orthogonal moves cost 1, diagonal moves
// cost sqrt(2). Diagonals never cut corners (both orthogonal neighbours must be free).
class GridGraph {
public:
    GridGraph(int width, int height, Connectivity conn, std::vector<std::uint8_t> blocked)
        : w_(width), h_(height), conn_(conn), blocked_(std::move(blocked)) {}

    int width() const { return w_; }
    int height() const { return h_; }
    Connectivity connectivity() const { return conn_; }
    std::size_t num_nodes() const { return static_cast<std::size_t>(w_) * h_; }
    bool is_blocked(NodeId u) const { return blocked_[u] != 0; }
    NodeId id(int x, int y) const { return static_cast<NodeId>(y) * w_ + x; }
    int x_of(NodeId u) const { return static_cast<int>(u % static_cast<NodeId>(w_)); }
    int y_of(NodeId u) const { return static_cast<int>(u / static_cast<NodeId>(w_)); }

    std::size_t free_cells() const {
        std::size_t c = 0;
        for (auto b : blocked_) c += (b == 0);
        return c;
    }

    template <class F>
    void for_each_neighbor(NodeId u, F&& f) const {
        const int x = x_of(u), y = y_of(u);
        auto open = [&](int nx, int ny) {
            return nx >= 0 && nx < w_ && ny >= 0 && ny < h_ && !blocked_[id(nx, ny)];
        };
        static constexpr int kOrth[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (const auto& d : kOrth)
            if (open(x + d[0], y + d[1])) f(id(x + d[0], y + d[1]), 1.0);
        if (conn_ == Connectivity::Eight) {
            static constexpr int kDiag[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
            for (const auto& d : kDiag)
                if (open(x + d[0], y + d[1]) && open(x + d[0], y) && open(x, y + d[1]))
                    f(id(x + d[0], y + d[1]), kSqrt2);
        }
    }

private:
    int w_, h_;
    Connectivity conn_;
    std::vector<std::uint8_t> blocked_;
};

// ------------------------------------------------------------------ CSR
struct Edge {
    NodeId from;
    NodeId to;
    std::uint32_t weight;
};

class CSRGraph {
public:
    CSRGraph() = default;

    // Builds the CSR arrays with a counting sort. If `symmetrize`, every edge is
    // inserted in both directions (undirected graph).
    static CSRGraph from_edges(std::size_t n, const std::vector<Edge>& edges, bool symmetrize) {
        CSRGraph g;
        g.off_.assign(n + 1, 0);
        for (const Edge& e : edges) {
            ++g.off_[e.from + 1];
            if (symmetrize) ++g.off_[e.to + 1];
        }
        for (std::size_t i = 0; i < n; ++i) g.off_[i + 1] += g.off_[i];
        g.to_.resize(g.off_[n]);
        g.w_.resize(g.off_[n]);
        std::vector<std::uint32_t> cursor(g.off_.begin(), g.off_.end() - 1);
        for (const Edge& e : edges) {
            g.to_[cursor[e.from]] = e.to;
            g.w_[cursor[e.from]++] = e.weight;
            if (symmetrize) {
                g.to_[cursor[e.to]] = e.from;
                g.w_[cursor[e.to]++] = e.weight;
            }
        }
        return g;
    }

    std::size_t num_nodes() const { return off_.empty() ? 0 : off_.size() - 1; }
    std::size_t num_edges() const { return to_.size(); }  // directed arcs stored

    template <class F>
    void for_each_neighbor(NodeId u, F&& f) const {
        for (std::uint32_t e = off_[u]; e < off_[u + 1]; ++e) f(to_[e], static_cast<double>(w_[e]));
    }

private:
    std::vector<std::uint32_t> off_;
    std::vector<NodeId> to_;
    std::vector<std::uint32_t> w_;
};

}  // namespace pf
