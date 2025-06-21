#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include "store_mngr.h"

#define MB_TO_BYTES(x)  ((x) * 1048576)
#define KB_TO_BYTES(x) ((x) * 1024)

class store_manager_test : public ::testing::Test {
protected:
    store_mngr_t *str_mngr = nullptr;
    const uint32_t total_mem_pool_size = MB_TO_BYTES(2);
    const uint32_t min_block_size = KB_TO_BYTES(4);

    void SetUp() override {
        store_mngr_cfg_t cfg;
        cfg.store_size = total_mem_pool_size;
        cfg.buddy_min_block_size = min_block_size;
        cfg.clear_when_free_block = false;
        cfg.type = TYPE1;
        ASSERT_EQ(store_mngr_create(&str_mngr, &cfg), 0);
    }

    void TearDown() override {
        ASSERT_EQ(store_mngr_destroy(str_mngr), 0);
    }

};

TEST_F(store_manager_test, store_manager_alloc_free_sanity) {
    uint32_t block_size = KB_TO_BYTES(4);
    store_mngr_block_t *block;

    std:: cout << "\nstore_mngr_block_alloc and free legal values\n";
    ASSERT_EQ(store_mngr_block_alloc(str_mngr, block_size, block), 0);
    ASSERT_EQ(store_mngr_block_free(str_mngr, block), 0);

    std:: cout << "\nstore_mngr_block_alloc and free legal max values\n";
    block_size = total_mem_pool_size;
    ASSERT_EQ(store_mngr_block_alloc(str_mngr, block_size, block), 0);
    ASSERT_EQ(store_mngr_block_free(str_mngr, block), 0);    

    std:: cout << "\nstore_mngr_block_alloc and free legal min values\n";
    block_size = min_block_size;
    ASSERT_EQ(store_mngr_block_alloc(str_mngr, block_size, block), 0);
    ASSERT_EQ(store_mngr_block_free(str_mngr, block), 0);    


}

TEST_F(store_manager_test, store_manager_alloc_free_failed) {
    store_mngr_block_t *block;
    uint32_t block_size;

    std:: cout << "\nstore_mngr_block_alloc to large block values\n"; 
    block_size = total_mem_pool_size +1;
    ASSERT_NE(store_mngr_block_alloc(str_mngr, block_size, block), 0);
}


