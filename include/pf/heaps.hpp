// Open-list implementations compared in the Dijkstra / A* experiments.
//
// Common interface (so the search code is written once):
//     explicit Heap(std::size_t num_nodes);
//     void        push_or_update(const Item&);  // insert, or decrease-key if supported
//     Item        pop();                        // removes and returns the best item
//     bool        empty() const;
//     std::size_t size() const;
//     std::size_t peak_size() const;            // max simultaneous entries
//     std::size_t peak_bytes() const;           // bytes owned by the heap at its peak
//     static const char* name();
//
// Lazy-deletion heaps (StdPriorityQueue, DAryHeap) just push duplicates and the
// search skips stale entries on pop. IndexedBinaryHeap keeps one entry per node
// and performs a real decrease-key.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

#include "pf/common.hpp"
#include "pf/dynamic_array.hpp"

namespace pf {

// Swap this alias to use your own DS-library dynamic array.
template <class T>
using DefaultContainer = DynamicArray<T>;

// ---------------------------------------------------------------- std::priority_queue
class StdPriorityQueue {
    struct Cmp {
        bool operator()(const Item& a, const Item& b) const noexcept { return better(b, a); }
    };
    struct Queue : std::priority_queue<Item, std::vector<Item>, Cmp> {
        std::size_t cap() const { return this->c.capacity(); }  // `c` is protected
    };

public:
    explicit StdPriorityQueue(std::size_t = 0) {}
    static const char* name() { return "std_pq"; }
    void push_or_update(const Item& it) {
        q_.push(it);
        peak_ = std::max(peak_, q_.size());
    }
    Item pop() {
        Item t = q_.top();
        q_.pop();
        return t;
    }
    bool empty() const { return q_.empty(); }
    std::size_t size() const { return q_.size(); }
    std::size_t peak_size() const { return peak_; }
    std::size_t peak_bytes() const { return q_.cap() * sizeof(Item); }

private:
    Queue q_;
    std::size_t peak_ = 0;
};

// ---------------------------------------------------------------- D-ary heap, lazy deletion
// D = 2 -> classic binary heap, D = 4 -> 4-ary heap (shallower tree, better cache use).
template <unsigned D, template <class> class Container = DefaultContainer>
class DAryHeap {
    static_assert(D >= 2, "arity must be >= 2");

public:
    explicit DAryHeap(std::size_t = 0) {}
    static const char* name() { return D == 2 ? "binary" : (D == 4 ? "quaternary" : "dary"); }

    void push_or_update(const Item& it) {
        a_.push_back(it);  // grow by one, then sift the new item up from the hole
        sift_up(a_.size() - 1, it);
        peak_ = std::max(peak_, a_.size());
    }
    Item pop() {
        Item top = a_[0];
        Item last = a_.back();
        a_.pop_back();
        if (!a_.empty()) sift_down(0, last);
        return top;
    }
    bool empty() const { return a_.empty(); }
    std::size_t size() const { return a_.size(); }
    std::size_t peak_size() const { return peak_; }
    std::size_t peak_bytes() const { return a_.capacity() * sizeof(Item); }  // capacity never shrinks

private:
    void sift_up(std::size_t i, const Item& it) {
        while (i > 0) {
            std::size_t p = (i - 1) / D;
            if (!better(it, a_[p])) break;
            a_[i] = a_[p];
            i = p;
        }
        a_[i] = it;
    }
    void sift_down(std::size_t i, const Item& it) {
        const std::size_t n = a_.size();
        for (;;) {
            std::size_t c = i * D + 1;
            if (c >= n) break;
            std::size_t best = c;
            const std::size_t end = std::min(c + D, n);
            for (std::size_t k = c + 1; k < end; ++k)
                if (better(a_[k], a_[best])) best = k;
            if (!better(a_[best], it)) break;
            a_[i] = a_[best];
            i = best;
        }
        a_[i] = it;
    }

    Container<Item> a_;
    std::size_t peak_ = 0;
};

using BinaryHeap = DAryHeap<2>;
using QuaternaryHeap = DAryHeap<4>;

// ---------------------------------------------------------------- indexed binary heap, decrease-key
class IndexedBinaryHeap {
    static constexpr std::uint32_t kNone = 0xFFFFFFFFu;

public:
    explicit IndexedBinaryHeap(std::size_t num_nodes) : pos_(num_nodes, kNone) {}
    static const char* name() { return "indexed_binary"; }

    void push_or_update(const Item& it) {
        const std::uint32_t p = pos_[it.node];
        if (p == kNone) {
            a_.push_back(it);
            sift_up(a_.size() - 1, it);
            peak_ = std::max(peak_, a_.size());
        } else if (it.g < a_[p].g) {
            // decrease-key in place. We compare g (the real path cost), not `better`:
            // with floating point, f can round to the same value even though g dropped,
            // and ignoring such an update would lose the cheaper path.
            if (sift_up(p, it) == p) sift_down(p, it);  // f may even have grown by rounding
        }
    }
    Item pop() {
        Item top = a_[0];
        Item last = a_.back();
        a_.pop_back();
        pos_[top.node] = kNone;
        if (!a_.empty()) sift_down(0, last);
        return top;
    }
    bool empty() const { return a_.empty(); }
    std::size_t size() const { return a_.size(); }
    std::size_t peak_size() const { return peak_; }
    std::size_t peak_bytes() const {
        return pos_.size() * sizeof(std::uint32_t) + a_.capacity() * sizeof(Item);
    }

private:
    void place(std::size_t i, const Item& it) {
        a_[i] = it;
        pos_[it.node] = static_cast<std::uint32_t>(i);
    }
    std::size_t sift_up(std::size_t i, const Item& it) {
        while (i > 0) {
            std::size_t p = (i - 1) / 2;
            if (!better(it, a_[p])) break;
            place(i, a_[p]);
            i = p;
        }
        place(i, it);
        return i;
    }
    void sift_down(std::size_t i, const Item& it) {
        const std::size_t n = a_.size();
        for (;;) {
            std::size_t c = 2 * i + 1;
            if (c >= n) break;
            if (c + 1 < n && better(a_[c + 1], a_[c])) ++c;
            if (!better(a_[c], it)) break;
            place(i, a_[c]);
            i = c;
        }
        place(i, it);
    }

    DefaultContainer<Item> a_;
    std::vector<std::uint32_t> pos_;
    std::size_t peak_ = 0;
};

}  // namespace pf
