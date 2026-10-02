#include "mem_tracker.hpp"

#include <cstdlib>
#include <new>

namespace {
std::size_t g_current = 0;
std::size_t g_peak = 0;
constexpr std::size_t kHeader = 16;  // keeps 16-byte alignment of the returned pointer

void* tracked_alloc(std::size_t n) {
    void* raw = std::malloc(n + kHeader);
    if (!raw) throw std::bad_alloc();
    *static_cast<std::size_t*>(raw) = n;
    g_current += n;
    if (g_current > g_peak) g_peak = g_current;
    return static_cast<char*>(raw) + kHeader;
}

void tracked_free(void* p) noexcept {
    if (!p) return;
    void* raw = static_cast<char*>(p) - kHeader;
    g_current -= *static_cast<std::size_t*>(raw);
    std::free(raw);
}
}  // namespace

namespace mem {
std::size_t current() { return g_current; }
std::size_t peak() { return g_peak; }
void reset_peak() { g_peak = g_current; }
}  // namespace mem

void* operator new(std::size_t n) { return tracked_alloc(n); }
void* operator new[](std::size_t n) { return tracked_alloc(n); }
void operator delete(void* p) noexcept { tracked_free(p); }
void operator delete[](void* p) noexcept { tracked_free(p); }
void operator delete(void* p, std::size_t) noexcept { tracked_free(p); }
void operator delete[](void* p, std::size_t) noexcept { tracked_free(p); }
