#include <gtest/gtest.h>
#include "net_server.h" // Include your server header
#include "net_client.h"

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     54321

net_server_t *server = nullptr;

void test_server_callback(uint32_t client_ip, uint16_t client_port, void *data, size_t data_size, void *ctx) {
}

// Define a fixture to set up common objects for all tests if needed
class NetServerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common objects or settings if needed before each test
    }

    void TearDown() override {

        net_server_destroy(server);
    }
};

TEST(NetServerTest, ServerStart) {
    net_server_cfg_t server_cfg;
    server_cfg.local_port = SERVER_PORT;

    EXPECT_EQ(net_server_create(&server_cfg, &server), 0) << "Server creation failed";
    ASSERT_NE(server, nullptr);

    // Valid server start
    EXPECT_EQ(net_server_start(server, test_server_callback, nullptr), 0) << "Server failed to start";

    // Invalid server start (null server)
    EXPECT_NE(net_server_start(nullptr, test_server_callback, nullptr), 0) << "Server start should fail with null server";

        // Invalid server start (null callback)
    EXPECT_NE(net_server_start(server, nullptr, nullptr), 0) << "Server start should fail with null callback";

    TearDown();
}


TEST(NetServerTest, ServerCreate) {

    net_server_cfg_t server_cfg;
    server_cfg.local_port = SERVER_PORT;

        // Valid server creation
    EXPECT_EQ(net_server_create(&server_cfg, &server), 0) << "Server creation failed";
    EXPECT_NE(server, nullptr) << "Server pointer is null after creation";

    // Invalid server creation (null config)
    EXPECT_NE(net_server_create(nullptr, &server), 0) << "Server creation should fail with null config";

    // Invalid server creation (null server pointer)
    EXPECT_NE(net_server_create(&server_cfg, nullptr), 0) << "Server creation should fail with null server pointer";

    TearDown();

}



TEST(NetServerTest, ServerStop) {
    net_server_cfg_t server_cfg;
    server_cfg.local_port = SERVER_PORT;
    server = nullptr;

    ASSERT_EQ(net_server_create(&server_cfg, &server), 0);
    ASSERT_NE(server, nullptr);

    ASSERT_EQ(net_server_start(server, test_server_callback, nullptr), 0);

    // Valid server stop
    EXPECT_EQ(net_server_stop(server), 0) << "Server failed to stop";

    // Invalid server stop (null server)
    EXPECT_NE(net_server_stop(nullptr), 0) << "Server stop should fail with null server";

    net_server_destroy(server); // Cleanup
}


TEST(NetServerTest, ServerDestroy) {
    net_server_cfg_t server_cfg;
    server_cfg.local_port = SERVER_PORT;
    server = nullptr;

    ASSERT_EQ(net_server_create(&server_cfg, &server), 0);
    ASSERT_NE(server, nullptr);

    // Valid server destroy
    EXPECT_EQ(net_server_destroy(server), 0) << "Server failed to destroy";

    // Invalid server destroy (null server)
    EXPECT_NE(net_server_destroy(nullptr), 0) << "Server destroy should fail with null server";
}