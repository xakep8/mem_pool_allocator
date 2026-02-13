#include "allocator.h"

#include <iostream>

Allocator::Allocator(size_t block_size, size_t block_count) {
    if (block_size == 0 || block_count == 0) {
        m_Initialized = false;
        return;
    }

    m_MemoryPool = std::make_unique<MemoryPool>();
    m_MemoryPool->block_count = block_count;
    size_t alignment = alignof(Block);
    block_size = std::max(block_size, sizeof(Block));
    block_size = (block_size + alignment - 1) & ~(alignment - 1);
    m_MemoryPool->block_size = block_size;
    m_MemoryPool->memory = std::malloc(m_MemoryPool->block_size * block_count);
    if (!m_MemoryPool->memory) {
        m_Initialized = false;
        return;
    }
#ifdef DEBUG
    m_PoolId = reinterpret_cast<uintptr_t>(this) & 0xFFFFFFFF;
#endif
    char* start = reinterpret_cast<char*>(m_MemoryPool->memory);
    for (size_t i = 0; i < block_count; i++) {
        Block* block = reinterpret_cast<Block*>(start + (i * m_MemoryPool->block_size));
        block->next = m_MemoryPool->free_list;
#ifdef DEBUG
        block->is_free = true;
        block->pool_id = m_PoolId;
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
    if (!m_Initialized || !m_MemoryPool) return nullptr;

    if (m_MemoryPool->free_list == nullptr) {
        return nullptr;
    }
    Block* block = m_MemoryPool->free_list;
    m_MemoryPool->free_list = block->next;
#ifdef DEBUG
    if (!block->is_free) {
        std::cerr << "Allocator corruption detected\n";
        std::abort();
    }
    block->is_free = false;
#endif
    return block;
}

void Allocator::free(void* ptr) {
    if (ptr == nullptr) return;

    if (!m_Initialized || !m_MemoryPool) return;

    char* mem_start = static_cast<char*>(m_MemoryPool->memory);
    char* mem_end = mem_start + (m_MemoryPool->block_size * m_MemoryPool->block_count);

    if (ptr < mem_start || ptr >= mem_end) {
        std::cerr << "Invalid free (pointer not from pool)\n";
        std::abort();
    }

    std::ptrdiff_t offset = static_cast<char*>(ptr) - mem_start;

    if (offset % m_MemoryPool->block_size != 0) {
        std::cerr << "Invalid free (not block aligned)\n";
        std::abort();
    }

    Block* block = static_cast<Block*>(ptr);
#ifdef DEBUG
    if (block->pool_id != m_PoolId) {
        std::cerr << "Invalid free (wrong allocator)\n";
        std::abort();
    }
    if (block->is_free) {
        std::cerr << "Double free error\n";
        std::abort();
    }
    block->is_free = true;
#endif
    block->next = m_MemoryPool->free_list;
    m_MemoryPool->free_list = block;
}