#include "allocator_slab.h"

#include <iostream>

SlabAllocator::SlabAllocator() {
    m_Slabs.emplace_back(std::make_unique<Allocator>(64, 100));
    m_Slabs.emplace_back(std::make_unique<Allocator>(128, 100));
    m_Slabs.emplace_back(std::make_unique<Allocator>(256, 100));
    m_Slabs.emplace_back(std::make_unique<Allocator>(512, 100));
}

void* SlabAllocator::allocate(size_t size) {
    for (auto& slab : m_Slabs) {
        if (size <= slab->block_size()) {
            return slab->allocate();
        }
    }
    return nullptr;
}

void SlabAllocator::free(void* ptr, size_t size) {
    for (auto& slab : m_Slabs) {
        if (size <= slab->block_size()) {
            slab->free(ptr);
            return;
        }
    }
}