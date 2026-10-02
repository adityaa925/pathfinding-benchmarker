// Minimal dynamic array (the role your DS-library dynamic array plays).
//
// The heaps in heaps.hpp are templated on a container with this interface:
//   push_back, pop_back, back, operator[], size, empty, capacity, reserve, clear
// To benchmark with the array from your own data-structures library, change the
// alias `DefaultContainer` in heaps.hpp -- nothing else needs to change.
//
// Restricted to trivially copyable T so growth can use memcpy.
#pragma once
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace pf {

template <class T>
class DynamicArray {
    static_assert(std::is_trivially_copyable_v<T>, "DynamicArray<T> needs trivially copyable T");

public:
    DynamicArray() = default;
    DynamicArray(const DynamicArray& o) {
        reserve(o.size_);
        if (o.size_) std::memcpy(data_, o.data_, o.size_ * sizeof(T));
        size_ = o.size_;
    }
    DynamicArray(DynamicArray&& o) noexcept : data_(o.data_), size_(o.size_), cap_(o.cap_) {
        o.data_ = nullptr;
        o.size_ = o.cap_ = 0;
    }
    DynamicArray& operator=(DynamicArray o) noexcept {
        swap(o);
        return *this;
    }
    ~DynamicArray() { ::operator delete(data_); }

    void swap(DynamicArray& o) noexcept {
        std::swap(data_, o.data_);
        std::swap(size_, o.size_);
        std::swap(cap_, o.cap_);
    }

    void reserve(std::size_t n) {
        if (n <= cap_) return;
        T* p = static_cast<T*>(::operator new(n * sizeof(T)));
        if (size_) std::memcpy(p, data_, size_ * sizeof(T));
        ::operator delete(data_);
        data_ = p;
        cap_ = n;
    }

    void push_back(const T& v) {
        T copy = v;  // v may alias our own storage
        if (size_ == cap_) reserve(cap_ ? cap_ * 2 : 8);
        data_[size_++] = copy;
    }
    void pop_back() noexcept { --size_; }
    void clear() noexcept { size_ = 0; }

    T& back() noexcept { return data_[size_ - 1]; }
    const T& back() const noexcept { return data_[size_ - 1]; }
    T& operator[](std::size_t i) noexcept { return data_[i]; }
    const T& operator[](std::size_t i) const noexcept { return data_[i]; }

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return cap_; }
    bool empty() const noexcept { return size_ == 0; }
    T* data() noexcept { return data_; }

private:
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_ = 0;
};

}  // namespace pf
