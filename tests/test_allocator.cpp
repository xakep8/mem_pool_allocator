#include <gtest/gtest.h>

#include <atomic>
#include <random>
#include <thread>
#include <vector>

#include "allocator.h"
#include "allocator_slab.h"

TEST(AllocatorTests, ExhaustsPoolCorrectly) {
    Allocator alloc(128, 10);

    std::vector<void*> ptrs;

    while (void* p = alloc.allocate()) {
        ptrs.push_back(p);
    }

    EXPECT_EQ(ptrs.size(), 10);
}

TEST(AllocatorTests, ReusesFreedBlocks) {
    Allocator alloc(128, 5);

    std::vector<void*> ptrs;

    for (int i = 0; i < 5; ++i) ptrs.push_back(alloc.allocate());

    for (void* p : ptrs) alloc.free(p);

    void* p1 = alloc.allocate();
    void* p2 = alloc.allocate();

    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
}

TEST(AllocatorTests, FreedBlockGetsReused) {
    Allocator alloc(128, 1);

    void* p1 = alloc.allocate();
    ASSERT_NE(p1, nullptr);

    alloc.free(p1);

    void* p2 = alloc.allocate();
    EXPECT_EQ(p1, p2);
}

TEST(AllocatorTests, BlocksAreAligned) {
    Allocator alloc(64, 4);

    void* p = alloc.allocate();
    ASSERT_NE(p, nullptr);

    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % alignof(void*), 0);
}

TEST(AllocatorDeathTests, DoubleFreeCausesAbort) {
#ifdef DEBUG
    Allocator alloc(128, 2);

    void* p = alloc.allocate();
    alloc.free(p);

    EXPECT_DEATH(alloc.free(p), "Double free");
#endif
}

TEST(AllocatorDeathTests, InvalidFreeCausesAbort) {
#ifdef DEBUG
    Allocator alloc(128, 2);

    int x = 42;

    EXPECT_DEATH(alloc.free(&x), "Invalid free");
#endif
}

TEST(AllocatorStressTests, RepeatedAllocateFreeCycles) {
    Allocator alloc(128, 50);

    for (int i = 0; i < 1000; ++i) {
        std::vector<void*> ptrs;

        for (int j = 0; j < 50; ++j) {
            void* p = alloc.allocate();
            ASSERT_NE(p, nullptr);
            ptrs.push_back(p);
        }

        for (void* p : ptrs) {
            alloc.free(p);
        }
    }

    SUCCEED();
}

TEST(AllocatorStressTests, RandomAllocFreePattern) {
    Allocator alloc(128, 100);

    std::vector<void*> live_blocks;
    std::mt19937 rng(42);

    for (int i = 0; i < 5000; ++i) {
        bool do_alloc = live_blocks.empty() || (rng() % 2 == 0);

        if (do_alloc) {
            void* p = alloc.allocate();
            if (p) {
                live_blocks.push_back(p);
            }
        } else {
            size_t idx = rng() % live_blocks.size();
            alloc.free(live_blocks[idx]);
            live_blocks.erase(live_blocks.begin() + idx);
        }
    }

    // Free remaining
    for (void* p : live_blocks) alloc.free(p);

    SUCCEED();
}

TEST(AllocatorStressTests, ExhaustionBehavior) {
    Allocator alloc(64, 10);

    std::vector<void*> ptrs;

    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate();
        ASSERT_NE(p, nullptr);
        ptrs.push_back(p);
    }

    EXPECT_EQ(alloc.allocate(), nullptr);  // pool exhausted

    for (void* p : ptrs) alloc.free(p);

    EXPECT_NE(alloc.allocate(), nullptr);  // should work again
}

TEST(AllocatorStressTests, AlignmentConsistency) {
    Allocator alloc(128, 20);

    for (int i = 0; i < 100; ++i) {
        void* p = alloc.allocate();
        ASSERT_NE(p, nullptr);

        EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % alignof(void*), 0);

        alloc.free(p);
    }
}

TEST(AllocatorDeathTests, RandomDoubleFree) {
#ifdef DEBUG
    Allocator alloc(128, 10);

    void* p = alloc.allocate();
    alloc.free(p);

    EXPECT_DEATH(alloc.free(p), "Double free");
#endif
}

TEST(AllocatorDeathTests, MisalignedFree) {
#ifdef DEBUG
    Allocator alloc(128, 10);

    void* p = alloc.allocate();
    ASSERT_NE(p, nullptr);

    void* bad_ptr = static_cast<char*>(p) + 1;

    EXPECT_DEATH(alloc.free(bad_ptr), "not block aligned");
#endif
}

TEST(AllocatorThreadTests, ConcurrentAllocFree) {
    Allocator alloc(128, 100);
    std::atomic<bool> failed{false};

    auto worker = [&]() {
        for (int i = 0; i < 1000; ++i) {
            void* p = alloc.allocate();
            if (!p) {
                failed = true;
                return;
            }
            alloc.free(p);
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    EXPECT_FALSE(failed.load());
}

TEST(AllocatorThreadTests, PerThreadPoolsNoContention) {
    std::atomic<bool> failed{false};

    auto worker = [&]() {
        thread_local Allocator alloc(128, 50);

        for (int i = 0; i < 1000; ++i) {
            void* p = alloc.allocate();
            if (!p) {
                failed = true;
                return;
            }
            alloc.free(p);
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    EXPECT_FALSE(failed.load());
}

TEST(SlabAllocatorTests, SelectsCorrectSlab) {
    SlabAllocator alloc;

    void* p1 = alloc.allocate(50);   // should hit 64B slab
    void* p2 = alloc.allocate(100);  // 128B slab

    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
}

TEST(SlabAllocatorTests, ReuseWorks) {
    SlabAllocator alloc;

    void* p = alloc.allocate(60);
    alloc.free(p, 60);

    void* p2 = alloc.allocate(60);

    EXPECT_EQ(p, p2);
}
