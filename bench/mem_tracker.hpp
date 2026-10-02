// Counts live heap bytes by replacing global operator new/delete (see mem_tracker.cpp).
// Only linked into the benchmark executable. Single-threaded use.
#pragma once
#include <cstddef>

namespace mem {
std::size_t current();     // bytes currently allocated through operator new
std::size_t peak();        // high-water mark since the last reset_peak()
void reset_peak();         // peak := current
}  // namespace mem
