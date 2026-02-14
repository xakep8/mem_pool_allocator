#include "allocator.h"

#include <iostream>
#include <memory>

size_t Allocator::align_up(size_t size) {
    constexpr size_t alignment = alignof(Block);
    return (size + alignment - 1) & ~(alignment - 1);
}

Allocator::Allocator(size_t block_size, size_t block_count) {
    if (block_size == 0 || block_count == 0) {
        m_Initialized = false;
        return;
    }

    m_MemoryPool = std::make_unique<MemoryPool>();
    m_MemoryPool->block_count = block_count;
    size_t payload_size = block_size;
    size_t raw_block_size = sizeof(Block) + payload_size;

#ifdef DEBUG
    raw_block_size += sizeof(uint32_t);
#endif

    raw_block_size = align_up(raw_block_size);

    m_MemoryPool->block_size = raw_block_size;
    m_MemoryPool->payload_size = payload_size;
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
        Block* block = std::construct_at(reinterpret_cast<Block*>(start + (i * m_MemoryPool->block_size)));
        block->next = m_MemoryPool->free_list;
#ifdef DEBUG
        block->is_free = true;
        block->pool_id = m_PoolId;
        block->canary_front = CANARY_VALUE;

        uint32_t* rear =
            reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(block) + m_MemoryPool->block_size - sizeof(uint32_t));
        *rear = CANARY_VALUE;
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
    std::lock_guard<std::mutex> lock(m_Mutex);
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
    block->canary_front = CANARY_VALUE;

    uint32_t* rear =
        reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(block) + m_MemoryPool->block_size - sizeof(uint32_t));
    *rear = CANARY_VALUE;
#endif
    return reinterpret_cast<char*>(block) + sizeof(Block);
}

void Allocator::free(void* ptr) {
    if (ptr == nullptr) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_Initialized || !m_MemoryPool) return;

    char* mem_start = static_cast<char*>(m_MemoryPool->memory);
    char* mem_end = mem_start + (m_MemoryPool->block_size * m_MemoryPool->block_count);

    char* raw_ptr = reinterpret_cast<char*>(ptr);
    char* block_ptr = raw_ptr - sizeof(Block);

    if (block_ptr < mem_start || block_ptr >= mem_end) {
        std::cerr << "Invalid free (pointer not from pool)\n";
        std::abort();
    }

    std::ptrdiff_t offset = block_ptr - mem_start;

    if (offset % m_MemoryPool->block_size != 0) {
        std::cerr << "Invalid free (not block aligned)\n";
        std::abort();
    }

    Block* block = reinterpret_cast<Block*>(block_ptr);
#ifdef DEBUG
    if (block->pool_id != m_PoolId) {
        std::cerr << "Invalid free (wrong allocator)\n";
        std::abort();
    }
    if (block->is_free) {
        std::cerr << "Double free error\n";
        std::abort();
    }
    uint32_t* rear =
        reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(block) + m_MemoryPool->block_size - sizeof(uint32_t));

    if (block->canary_front != CANARY_VALUE || *rear != CANARY_VALUE) {
        std::cerr << "Memory corruption detected (canary smashed)\n";
        std::abort();
    }
    block->is_free = true;
#endif
    block->next = m_MemoryPool->free_list;
    m_MemoryPool->free_list = block;
}