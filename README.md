# Memory Pool Allocator

A fast and efficient memory pool allocator implementation in C++ using a free-list approach. This allocator pre-allocates a fixed-size pool of memory blocks, enabling O(1) allocation and deallocation operations without the overhead of repeated system calls.

## Features

- **Fast Allocation**: O(1) time complexity for both allocation and deallocation
- **Reduced Fragmentation**: Pre-allocated contiguous memory reduces heap fragmentation
- **Cache-Friendly**: Better cache locality compared to scattered heap allocations
- **Zero Overhead**: Minimal bookkeeping with intrusive free-list design
- **Debug Support**: Optional memory poisoning in debug builds to detect use-after-free bugs
- **Modern C++**: Written in C++17 with smart pointers and RAII principles

## How It Works

The allocator maintains a pool of fixed-size memory blocks organized as a singly-linked free list:

1. **Initialization**: Pre-allocates a contiguous chunk of memory divided into fixed-size blocks
2. **Allocation**: Returns the first block from the free list and updates the head
3. **Deallocation**: Prepends the freed block back to the head of the free list
4. **Intrusive List**: Uses the block memory itself to store the `next` pointer when free

## Building

### Prerequisites

- CMake 3.10 or higher
- C++20 compatible compiler (GCC, Clang, MSVC)
- GoogleTest (fetched automatically via CMake)

### Build Instructions

#### Using the build script (Linux/macOS):

```bash
chmod +x build.sh
./build.sh
```

#### Using CMake directly:

```bash
mkdir out && cd out
cmake ..
make
```

This will build:
- Main executable: `bin/mem_pool_allocator`
- Test executable: `tests/bin/mem_pool_allocator_tests`

#### Running Tests

```bash
make test
# or run the test executable directly:
./tests/bin/mem_pool_allocator_tests
```

The test executable is compiled with AddressSanitizer enabled for memory safety checks.

## Usage

### Basic Example

```cpp
#include "allocator.h"
#include <iostream>

int main() {
    // Create a pool with 100 blocks of 64 bytes each
    Allocator allocator(64, 100);
    
    if (!allocator.is_initialized()) {
        std::cerr << "Failed to initialize allocator\n";
        return 1;
    }
    
    // Allocate a block
    void* block1 = allocator.allocate();
    if (block1) {
        // Use the memory
        int* data = static_cast<int*>(block1);
        *data = 42;
        
        // Free when done
        allocator.free(block1);
    }
    
    return 0;
}
```

### Using with Custom Types

```cpp
struct MyObject {
    int id;
    double value;
    char name[32];
};

// Create pool for MyObject instances
Allocator allocator(sizeof(MyObject), 50);

// Allocate and construct
void* mem = allocator.allocate();
if (mem) {
    MyObject* obj = new (mem) MyObject{1, 3.14, "test"};
    
    // Use object...
    
    // Destruct and free
    obj->~MyObject();
    allocator.free(mem);
}
```

## API Reference

### Constructor

```cpp
Allocator(size_t block_size, size_t block_count)
```

Creates a memory pool allocator.

- **block_size**: Size of each block in bytes (minimum: `sizeof(void*)`)
- **block_count**: Number of blocks in the pool

### Methods

#### `void* allocate()`

Allocates a single block from the pool.

- **Returns**: Pointer to allocated block, or `nullptr` if pool is exhausted
- **Complexity**: O(1)

#### `void free(void* block)`

Returns a block to the pool.

- **Parameters**: `block` - Pointer to block previously obtained from `allocate()`
- **Complexity**: O(1)

#### `bool is_initialized() const`

Checks if the allocator was successfully initialized.

- **Returns**: `true` if initialized, `false` if initialization failed

## Performance Benefits

Compared to `malloc`/`free`:

- **Faster**: No system calls or heap traversal
- **Predictable**: Constant-time operations
- **Less Fragmentation**: Pre-allocated contiguous memory
- **Better Cache Performance**: Improved spatial locality

Ideal for:

- Game engines (entity/component pools)
- Network servers (packet buffers)
- Real-time systems requiring predictable performance
- Applications with many same-sized allocations

## Debug Mode

When compiled with `-DDEBUG`, freed memory is poisoned with `0xDD` pattern to help detect use-after-free bugs.

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Limitations

- **Fixed Block Size**: All allocations are the same size
- **Fixed Capacity**: Cannot grow beyond initial block count
- **Manual Management**: No automatic object construction/destruction
- **Not Thread-Safe**: Requires external synchronization for concurrent access

## License

This project is available for educational and commercial use.

## Author

Kunal Dubey
