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
        b_alloc_cfg.min_block_size = 512;
        b_alloc_cfg.total_mem_pool_size = KB_TO_BYTES(4);
        ASSERT_EQ(buddy_alloc_create(&b_alloc_cfg, &b_alloc), 0);
    }

    void TearDown() override {
        ASSERT_EQ(buddy_alloc_destroy(b_alloc), 0);
    }
};

TEST(buddy_allocator_create_destroy_test, create_destroy) {
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

TEST_F(buddy_allocator_test, initial_free_list) {
    // At creation, single block of full size should exist at level 0
    // levels: 4096→512 yields 3 splits: levels 0...3
    uint32_t num_of_levels;
    uint32_t free_blocks_count;
    ASSERT_EQ(buddy_alloc_get_num_of_levels(b_alloc, &num_of_levels), 0);
    ASSERT_EQ(num_of_levels, 4u);
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 0, &free_blocks_count), 0);
    EXPECT_EQ(free_blocks_count, 1);
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 1, &free_blocks_count), 0);
    EXPECT_EQ(free_blocks_count, 0);
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 2, &free_blocks_count), 0);
    EXPECT_EQ(free_blocks_count, 0);
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 3, &free_blocks_count), 0);
    EXPECT_EQ(free_blocks_count, 0);        
}

TEST_F(buddy_allocator_test, alloc_free_sanity) {

    uint32_t block_size1 = KB_TO_BYTES(1);
    block_t *out_block1;
    std:: cout << "\nAllocating a block 1: size: " << block_size1 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size1, &out_block1), 0);

    block_t *out_block2;    
    uint32_t block_size2 = KB_TO_BYTES(1);
    std:: cout << "\nAllocating a block 2: size: " << block_size2 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size2, &out_block2), 0);

    std:: cout << "\nfreeing block 1: size: " << block_size1 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block1), 0);

    block_t *out_block3;    
    uint32_t block_size3 = KB_TO_BYTES(2);    
    std:: cout << "\nAllocating a block 3: size: " << block_size3 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size3, &out_block3), 0);

    block_t *out_block4;    
    uint32_t block_size4 = 512;    
    std:: cout << "\nAllocating a block 4: size: " << block_size4 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size4, &out_block4), 0);

    block_t *out_block5;    
    uint32_t block_size5 = KB_TO_BYTES(1);
    std:: cout << "\nAllocating a block 5: size: " << block_size5 << "\n";
    ASSERT_NE(buddy_alloc_allocate(b_alloc, block_size5, &out_block5), 0);
   
    std:: cout << "\nFreeing block 4: size: " << block_size4 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block4), 0);

    std:: cout << "\nTry Allocating block 5 again: size: " << block_size5 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size5, &out_block5), 0);

    std:: cout << "\nFreeing block 5: size: " << block_size5 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block5), 0);

    std:: cout << "\nFreeing block 3: size: " << block_size3 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block3), 0);

    std:: cout << "\nFreeing block 2: size: " << block_size2 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block2), 0);    

}

TEST_F(buddy_allocator_test, simple_allocate_and_free) {
    block_t *out_block;    
    uint32_t block_size = 512;
    uint32_t block_level;
    uint32_t free_blocks_count;

    std:: cout << "\nAllocating a block: size: " << block_size << " should take a level 3 block \n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size, &out_block), 0);
    EXPECT_EQ(buddy_alloc_get_block_level(out_block, &block_level), 0);
    EXPECT_EQ(block_level, 3u);

    std:: cout << "\nCounting free blocks at level 3, should be 1 (buddy) \n";
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 3, &free_blocks_count), 0);
    std:: cout << "\nlevel 3, blocks count: " << free_blocks_count <<  "\n";
    EXPECT_EQ(free_blocks_count, 1);

    std:: cout << "\nfree allocated block \n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block), 0);

    std:: cout << "\nCounting free blocks at level 3, should be 0 (merging both and add upward) \n";
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 3, &free_blocks_count), 0);
    std:: cout << "\nlevel 3, blocks count: " << free_blocks_count <<  "\n";
    EXPECT_EQ(free_blocks_count, 0);

    std:: cout << "\nCounting free blocks at level 2, should be 0 (merging both and add upward) \n";
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 2, &free_blocks_count), 0);
    std:: cout << "\nlevel 2, blocks count: " << free_blocks_count <<  "\n";
    EXPECT_EQ(free_blocks_count, 0);

    std:: cout << "\nCounting free blocks at level 1, should be 0 (merging both and add upward) \n";
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 1, &free_blocks_count), 0);
    std:: cout << "\nlevel 2, blocks count: " << free_blocks_count <<  "\n";
    EXPECT_EQ(free_blocks_count, 0);

    std:: cout << "\nCounting free blocks at level 0, should be 1 (merging both and add upward up to start point) \n";
    EXPECT_EQ(buddy_alloc_count_free_blocks(b_alloc, 0, &free_blocks_count), 0);
    std:: cout << "\nlevel 2, blocks count: " << free_blocks_count <<  "\n";
    EXPECT_EQ(free_blocks_count, 1);    
}


/*
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

        * No solution yet (future solution can be adding a bit field to track used or free blocks)        
    */
    /*
    uint32_t block_size1 = KB_TO_BYTES(8);
    block_t *out_block1;
    std:: cout << "\nAllocating a block 1: size: " << block_size1 << "\n";
    ASSERT_EQ(buddy_alloc_allocate(b_alloc, block_size1, &out_block1), 0);

    std:: cout << "\nfreeing block 1: size: " << block_size1 << "\n";
    ASSERT_EQ(buddy_alloc_free(b_alloc, out_block1), 0);

    std:: cout << "\ntry to free block 1 again: size: " << block_size1 << "\n";
    ASSERT_NE(buddy_alloc_free(b_alloc, out_block1), 0);
}

*/

