#include "allocator.h"

#include <iostream>

Allocator::Allocator(size_t block_size, size_t block_count) {
    m_MemoryPool = std::make_unique<MemoryPool>();
    m_MemoryPool->block_count = block_count;
    m_MemoryPool->block_size = std::max(block_size, sizeof(Block));
    m_MemoryPool->memory = std::malloc(m_MemoryPool->block_size * block_count);
    if (!m_MemoryPool->memory) {
        m_Initialized = false;
        return;
    }
    char* start = reinterpret_cast<char*>(m_MemoryPool->memory);
    for (size_t i = 0; i < block_count; i++) {
        Block* block = reinterpret_cast<Block*>(start + (i * m_MemoryPool->block_size));
        block->next = m_MemoryPool->free_list;
#ifdef DEBUG
        block->is_free = false;
#endif
        m_MemoryPool->free_list = block;
    }
    m_Initialized = true;
}

Allocator::~Allocator() {
    if (m_MemoryPool && m_MemoryPool->memory) {
        std::free(m_MemoryPool->memory);
        m_MemoryPool->memory = nullptr;
    }
    m_Initialized = false;
}

void* Allocator::allocate() {
    if (m_MemoryPool->free_list == nullptr) {
        return nullptr;
    }
    Block* block = m_MemoryPool->free_list;
    m_MemoryPool->free_list = block->next;
#ifdef DEBUG
    block->is_free = false;
#endif
    return block;
}

void Allocator::free(void* ptr) {
    if (ptr == nullptr) return;
    Block* block = static_cast<Block*>(ptr);
#ifdef DEBUG
    if (block->is_free) {
        std::cerr << "Double free error\n";
        throw std::abort();
    }
    block->is_free = true;
#endif
    block->next = m_MemoryPool->free_list;
    m_MemoryPool->free_list = block;
}