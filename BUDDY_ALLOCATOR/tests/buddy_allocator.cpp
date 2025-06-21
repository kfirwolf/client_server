#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include "buddy_allocator.h"

#define MB_TO_BYTES(x)  ((x) * 1048576)
#define KB_TO_BYTES(x) ((x) * 1024)

class buddy_allocator_test : public ::testing::Test {
protected:
    buddy_allocator_t *b_alloc = nullptr;
    void SetUp() override {
        buddy_alloc_cfg_t b_alloc_cfg;
        b_alloc_cfg.min_block_size = KB_TO_BYTES(4);
        b_alloc_cfg.total_mem_pool_size = MB_TO_BYTES(2);
        ASSERT_EQ(buddy_alloc_create(&b_alloc_cfg, &b_alloc), 0);
    }

    void TearDown() override {
        ASSERT_EQ(buddy_alloc_destroy(b_alloc), 0);
    }
};

TEST(buddy_allocator_create_destroy_test, Buddy_allocator_create_destroy) {
    buddy_allocator_t *alloc = nullptr;
    buddy_alloc_cfg_t cfg;
    cfg.min_block_size = KB_TO_BYTES(4);
    cfg.total_mem_pool_size = MB_TO_BYTES(2);

    // Creation should succeed
    std:: cout << "\nbuddy_alloc_create legal values\n";    
    ASSERT_EQ(buddy_alloc_create(&cfg, &alloc), 0);

    // And destruction too
    std:: cout << "\nbuddy_alloc_destroy legal values\n";  
    ASSERT_EQ(buddy_alloc_destroy(alloc), 0);

    // Creation should fail (null pointers)
    std:: cout << "\nbuddy_alloc_create illegal values (null)\n"; 
    ASSERT_NE(buddy_alloc_create(nullptr, nullptr), 0);

    // Creation should fail (illegal values)
    std:: cout << "\nbuddy_alloc_create illegal values (total size < min block)\n"; 
    cfg.min_block_size = KB_TO_BYTES(4);
    cfg.total_mem_pool_size = KB_TO_BYTES(2);
    ASSERT_NE(buddy_alloc_create(&cfg, &alloc), 0);

    // Creation should fail (illegal values)
    std:: cout << "\nbuddy_alloc_create illegal values (min_block_size)\n"; 
    cfg.min_block_size = KB_TO_BYTES(5);
    cfg.total_mem_pool_size = MB_TO_BYTES(1);    
    ASSERT_NE(buddy_alloc_create(&cfg, &alloc), 0);
    
    // Creation should fail (illegal values)
    std:: cout << "\nbuddy_alloc_create illegal values (total_mem_pool_size)\n"; 
    cfg.min_block_size = KB_TO_BYTES(4);
    cfg.total_mem_pool_size = MB_TO_BYTES(3);    
    ASSERT_NE(buddy_alloc_create(&cfg, &alloc), 0);  
}

TEST_F(buddy_allocator_test, buddy_allocator_alloc_free_sanity) {

    uint32_t block_size = KB_TO_BYTES(8);
    block_t out_block1;
    std:: cout << "\nAllocating a block 1: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block1), 0);

    block_t out_block2;    
    block_size = KB_TO_BYTES(256);
    std:: cout << "\nAllocating a block 2: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block2), 0);

    std:: cout << "\nfreeing block 1: size: " << out_block1.size << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, &out_block1), 0);

    block_t out_block3;    
    block_size = KB_TO_BYTES(256);    
    std:: cout << "\nAllocating a block 3: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block3), 0);

    block_t out_block4;    
    block_size = KB_TO_BYTES(1000);    
    std:: cout << "\nAllocating a block 4: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block4), 0);

    block_t out_block5;    
    block_size = KB_TO_BYTES(990);    
    std:: cout << "\nAllocating a block 5: size: " << block_size << "\n";
    ASSERT_NE(buddy_alloc_allocate(b_alloc, block_size, &out_block5), 0);
   
    std:: cout << "\nfreeing block 4: size: " << out_block4.size << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, &out_block4), 0);

    std:: cout << "\nTry Allocating block 5 again: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block5), 0);    
}

TEST_F(buddy_allocator_test, buddy_allocator_double_free) {

    /*
        //////////////DOUBLE FREE BUG//////////////////////

        1. Allocate block at offset 0, size 8KB → Level 3
        2. Free the block
        3. It merges upward until level 0.
        4. Free list at level 0 now has offset 0.
        5. Now you free the same block again:
        6. You calculate next_pow2(size) → 8KB → Level 3.
        7. Check free list at level 3 → empty, So loop exits immediately, and you: linked_list_add_block_as_head(b_alloc, level 3, offset 0);

        Now you have:
            -One block at level 0 with offset 0.
            -One block at level 3 with offset 0.

        * No solution yet (future solution can be adding a bit filed to track used or free blocks)        
    */

    uint32_t block_size = KB_TO_BYTES(8);
    block_t out_block1;
    std:: cout << "\nAllocating a block 1: size: " << block_size << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block1), 0);

    std:: cout << "\nfreeing block 1: size: " << out_block1.size << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, &out_block1), 0);

    std:: cout << "\ntry to free block 1 again: size: " << out_block1.size << "\n";
    ASSERT_NE(buddy_alloc_free(b_alloc, &out_block1), 0);
}

