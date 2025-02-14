#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include "ram_file_ctrl.h"

#define FILE_PATH "/tmp/ram_file_ctrl_data.bin";

uint8_t data1[5] = {1,2,3,4,5};
uint8_t data2[4] = {6,7,8,9}; 

static void print_data(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << "\n"; // Reset to decimal format
}


class RamFileCtrlTest : public ::testing::Test {
protected:
    std::string filename = FILE_PATH;
    ram_file_t *mngr = nullptr;

    void SetUp() override {
        ram_file_cfg_t cfg;
        cfg.file_path = FILE_PATH;
        cfg.delete_files = 1;
        ASSERT_EQ(ram_file_create(&cfg, &mngr), 0);
    }

    void TearDown() override {
        ASSERT_EQ(ram_file_destroy(mngr), 0);
    }
};

TEST_F(RamFileCtrlTest, RamFileCtrlWriteReadData) {

    std:: cout << "writing data1 to file. file data: ";
    print_data(data1, 5); 
    ASSERT_EQ(ram_file_write(mngr, 0, data1, sizeof(data1)), 0);

    std:: cout << "writing data2 to file. file data: ";
    print_data(data2, 4); 
    ASSERT_EQ(ram_file_write(mngr, sizeof(data1), data2, sizeof(data2)), 0);

    // std::filesystem::exists returns a bool indicating whether the file exists.
    EXPECT_TRUE(std::filesystem::exists(filename)) << "File does not exist: " << filename;

    uint8_t data_array1[5] = {};

    ASSERT_EQ(ram_file_read(mngr, 0, data_array1, 5), 0);
    std:: cout << "Reading file. file data1: ";
    print_data(data_array1, 5);

    ASSERT_EQ(memcmp(data_array1, data1, 5), 0) << "\nExtracted data from file not equal saved data to file";

    uint8_t data_array2[4] = {};    
    ASSERT_EQ(ram_file_read(mngr, sizeof(data1), data_array2, 4), 0) << "\nfailed to read data from file";
    std:: cout << "Reading file. file data2: ";
    print_data(data_array2, 4);

    ASSERT_EQ(memcmp(data_array2, data2, 4), 0) << "\nExtracted data from file not equal saved data to file";
}
