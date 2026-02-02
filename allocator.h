#pragma once


class Allocator{
    private:
    typedef struct Block{
        Block* next;
    };
    struct MemoryPool{
        void* memory;

    };
};