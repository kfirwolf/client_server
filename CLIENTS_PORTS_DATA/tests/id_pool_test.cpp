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
            cfg.id_start_offset = OFFSET;
            ASSERT_EQ(id_pool_create(&pool, &cfg), 0);
        }

        void TearDown() override {
            ASSERT_EQ(id_pool_destroy(pool), 0);
            pool = nullptr;
        }
    };

// 1. Basic allocate and free
TEST_F(IdPoolTest, IdPoolBasicAllocateFree) {

    uint32_t id = -1;

    //allocate
    int ret = id_pool_allocate(pool, &id);

    if (ret < 0) {
        std::cout << "[ERROR] allocation failed with " << ret << std::endl;
    }
    std::cout << "[TEST] allocated id = " << id << std::endl;

    // Free
    EXPECT_EQ(id_pool_free(pool, id), 0);

}


// 2. Basic allocate/free sequence + Exhausting the pool
TEST_F(IdPoolTest, IdPoolAllocateFree) {

    uint32_t id = -1;
    int id_to_free = -1;
    int idx_id_to_free = 2;
    int ret = 0;
    for (size_t i = 0; i < ALLOCATE_AMOUNT; i++)
    {
        ret = id_pool_allocate(pool, &id);
        if (i == idx_id_to_free) {
            id_to_free = id;
        }
        if (ret < 0) {
            std::cout << "[ERROR] allocation failed with " << ret << std::endl;
          }
        std::cout << "[TEST] allocated id = " << id << std::endl;
    }

    // Now empty
    EXPECT_EQ(id_pool_allocate(pool, &id), -ENOENT);

    // Free one and allocate again
    EXPECT_EQ(id_pool_free(pool, id_to_free), 0);
    EXPECT_EQ(id_pool_allocate(pool, &id), 0);
    EXPECT_EQ(id, id_to_free);
}

// 3. Free invalid IDs
TEST_F(IdPoolTest, IdPoolFreeInvalidId) {
    // below offset
    EXPECT_EQ(id_pool_free(pool, (OFFSET -1)), -ESPIPE);
    // above offset+capacity-1 = 102
    EXPECT_EQ(id_pool_free(pool, CAPACITY + OFFSET +1), -ESPIPE);
}

// 4. Double free leads to “stack overflow”
TEST_F(IdPoolTest, IdPoolDoubleFree) {
    uint32_t id = -1;
    int ret = id_pool_allocate(pool, &id);
    EXPECT_EQ(id, OFFSET);

    EXPECT_EQ(id_pool_free(pool, id), 0);
    //try to free id that was already freed
    EXPECT_EQ(id_pool_free(pool, id), -EALREADY);
}

// 5. Null-pointer error paths
TEST_F(IdPoolTest, IdPoolNullCreateAllocateAndFree) {
    uint32_t id = -1;
    EXPECT_EQ(id_pool_allocate(nullptr, &id),       -EINVAL);
    EXPECT_EQ(id_pool_free(nullptr, 100),      -EINVAL);
    EXPECT_EQ(id_pool_destroy(nullptr),        -EINVAL);
    id_pool_t *p = nullptr;
    id_pool_cfg_t c{.capacity = 0, .id_start_offset = 0};
    EXPECT_EQ(id_pool_create(nullptr, &c),     -EINVAL);
    EXPECT_EQ(id_pool_create(&p,     &c),      -EINVAL);
}