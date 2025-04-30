#include <gtest/gtest.h>
#include <iostream>
#include <cstring>

#define CAPACITY 5
#define OFFSET 100
#define ALLOCATE_AMOUNT CAPACITY

extern "C" {
    #include "id_pool.h"
    }

class IdPoolTest : public ::testing::Test {
    protected:
        id_pool_t *pool = nullptr;
        id_pool_cfg_t cfg;

        void SetUp() override {
            id_pool_cfg_t cfg;
            cfg.capacity = CAPACITY;
            cfg.id_offset = OFFSET;
            ASSERT_EQ(id_pool_create(&pool, &cfg), 0);
        }

        void TearDown() override {
            ASSERT_EQ(id_pool_destroy(pool), 0);
            pool = nullptr;
        }
    };

// 1. Basic allocate/free sequence + Exhausting the pool
TEST_F(IdPoolTest, IdPoolAllocateFree) {

    int id = -1;
    int id_to_free = -1;
    int idx_id_to_free = 2;

    for (size_t i = 0; i < ALLOCATE_AMOUNT; i++)
    {
        id = id_pool_allocate(pool);
        EXPECT_EQ(id, (int)(OFFSET + i));
        if (i == idx_id_to_free) {
            id_to_free = id;
        }
        if (id < 0) {
            std::cout << "[ERROR] allocation failed with " << id << std::endl;
          }
        std::cout << "[TEST] allocated id = " << id << std::endl;
    }

    // Now empty
    EXPECT_EQ(id_pool_allocate(pool), -ENOENT);


    // Free one and allocate again
    EXPECT_EQ(id_pool_free(pool, id_to_free), 0);
    EXPECT_EQ(id_pool_allocate(pool), OFFSET + idx_id_to_free);
}

// 2. Free invalid IDs
TEST_F(IdPoolTest, IdPoolFreeInvalidId) {
    // below offset
    EXPECT_EQ(id_pool_free(pool, (OFFSET -1)), -ESPIPE);
    // above offset+capacity-1 = 102
    EXPECT_EQ(id_pool_free(pool, CAPACITY + OFFSET +1), -ESPIPE);
}

// 3. Double free leads to “stack overflow”
TEST_F(IdPoolTest, IdPoolDoubleFree) {
    int id = id_pool_allocate(pool);
    EXPECT_EQ(id, 100);

    EXPECT_EQ(id_pool_free(pool, id), 0);
    //try to free id that was already freed
    EXPECT_EQ(id_pool_free(pool, id), -EALREADY);
}

// 4. Null-pointer error paths
TEST_F(IdPoolTest, IdPoolNullCreate) {
    id_pool_t *p = nullptr;
    id_pool_cfg_t c{.capacity=0, .id_offset=0};
    EXPECT_EQ(id_pool_create(nullptr, &c),     -EINVAL);
    EXPECT_EQ(id_pool_create(&p,     &c),      -EINVAL);
}

TEST_F(IdPoolTest, IdPoolNullAllocateAndFree) {
    EXPECT_EQ(id_pool_allocate(nullptr),       -EINVAL);
    EXPECT_EQ(id_pool_free(nullptr, 100),      -EINVAL);
    EXPECT_EQ(id_pool_destroy(nullptr),        -EINVAL);
}