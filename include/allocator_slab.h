#pragma once

#include <memory>
#include <vector>

#include "allocator.h"

class SlabAllocator {
   private:
    std::vector<std::unique_ptr<Allocator>> m_Slabs;

   public:
    SlabAllocator();
    void* allocate(size_t size);
    void free(void* ptr, size_t size);
};