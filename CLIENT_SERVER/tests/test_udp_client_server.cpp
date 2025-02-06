#include <gtest/gtest.h>
#include "net_server.h"
#include "net_client.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>

// Global variables for test validation
std::atomic<bool> data_received(false);
std::vector<uint8_t> received_data;
#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     54321

void test_server_callback(uint32_t client_ip, uint16_t client_port, void *data, size_t data_size, void *ctx) {
    received_data.assign(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + data_size);
    data_received = true;
}

void net_infra(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << "\n"; // Reset to decimal format
}


class UDPClientServerTest : public ::testing::Test {
protected:
    net_server_t *server = nullptr;
    net_client_t *client = nullptr;

    void SetUp() override {
        net_server_cfg_t server_cfg;
        server_cfg.local_port = SERVER_PORT;
        ASSERT_EQ(net_server_create(&server_cfg, &server), 0);
        ASSERT_EQ(net_server_start(server, test_server_callback, nullptr), 0);
    }

    void TearDown() override {
        net_server_stop(server);
        net_server_destroy(server);
    }
};

TEST_F(UDPClientServerTest, SendAndReceiveData) {
    net_client_cfg_t client_cfg;
    client_cfg.server_ip = SERVER_IP;
    client_cfg.server_port = SERVER_PORT;

    ASSERT_EQ(net_client_create(&client_cfg, &client), 0);
    
    uint8_t test_message[] = {2, 1, 5, 8, 5, 4, 2, 7, 2};

    std::cout << "client send data (hex): ";
    net_infra(test_message, sizeof(test_message));

    net_client_iov_t iov = {test_message, sizeof(test_message)};
    net_client_data_t data = {&iov, 1};
    
    ASSERT_EQ(net_client_send(client, &data), 0);
    
    // Wait for the server to receive data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(data_received);

    std::cout << "server received data (hex): ";
    net_infra(test_message, sizeof(test_message));

    EXPECT_EQ(received_data, std::vector<uint8_t>(test_message, test_message + sizeof(test_message)));
    
    net_client_destroy(client);
}