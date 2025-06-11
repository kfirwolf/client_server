#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <infra_log.h>

#define FILE_BUFFER_SIZE (1024) // 1 KB

static int creating_data_file(const char *path, size_t f_size) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to open file: %s", strerror(errno));
        return -1;
    }

    uint8_t buffer[FILE_BUFFER_SIZE]; // 1KB buffer
    size_t full_chunks = f_size / sizeof(buffer);
    for (size_t i = 0; i < full_chunks; i++) {
        // Fill buffer with random 8-bit values
        for (size_t j = 0; j < sizeof(buffer); j++) {
            buffer[j] = rand() % 256; // Random 8-bit value
        }

        if (fwrite(buffer, sizeof(buffer), 1, file) != 1) {
            NET_INFRA_LOG(LOG_ERROR, "Failed writing to file");
            fclose(file);
            return -1;
        }
    }

    size_t remaining = f_size % sizeof(buffer);
    if (remaining > 0) {
        for (size_t j = 0; j < remaining; j++) {
            buffer[j] = rand() % 256;
        }

        if (fwrite(buffer, 1, remaining, file) != remaining) {
            NET_INFRA_LOG(LOG_ERROR, "Failed writing remaining bytes to file");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    NET_INFRA_LOG(LOG_DEBUG, "Binary file '%s' created successfully with random values.", path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        NET_INFRA_LOG(LOG_ERROR, "Usage: %s <bin_file_path> <file_size_in_bytes>", argv[0]);
        return EXIT_FAILURE;
    }

    const char *file_path = argv[1];
    char *endptr = NULL;
    size_t file_size = strtoull(argv[2], &endptr, 10);
    if (*endptr != '\0' || file_size == 0) {
        NET_INFRA_LOG(LOG_ERROR, "Invalid file size: %s", argv[2]);
        return EXIT_FAILURE;
    }

    if (creating_data_file(file_path, file_size) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating bin file");
        return EXIT_FAILURE;
    }

    NET_INFRA_LOG(LOG_DEBUG, "Creating bin file with path: %s and size: %zu bytes", file_path, file_size);
    return EXIT_SUCCESS;
}
