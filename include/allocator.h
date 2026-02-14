#pragma once

#include <cstddef>
#include <memory>
#include <mutex>

constexpr uint32_t CANARY_VALUE = 0xDEADC0DE;

class Allocator {
   private:
    typedef struct Block {
        Block* next;
#ifdef DEBUG
        bool is_free;
        uint32_t pool_id;
        uint32_t canary_front;
#endif
    } Block;
    typedef struct MemoryPool {
        void* memory;
        Block* free_list;
        size_t block_size;
        size_t block_count;
    } MemoryPool;
    bool m_Initialized;
    std::unique_ptr<MemoryPool> m_MemoryPool;
    std::mutex m_Mutex;
#ifdef DEBUG
    uint32_t m_PoolId;
#endif

   public:
    bool is_initialized() const { return m_Initialized; }
    size_t block_size() const { return m_MemoryPool->block_size; }
    void* allocate();
    void free(void* ptr);
    Allocator(size_t block_size, size_t block_count);
    ~Allocator();
};