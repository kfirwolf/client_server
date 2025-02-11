#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct ram_file_t ram_file_t;

typedef struct {
    const char *file_path;  // Path to the file used as backing store.
    size_t buffer_size; // Maximum size of the internal buffer.
    size_t max_file_size;
} ram_file_cfg_t;

/// @brief Creating the ram file manager object so it can be used to write or read data
/// @param cfg Configuration data set by the user
/// @param ram_file_mngr Will point to the created ram file manager.
/// @return Success (0) or an error code
int ram_file_create(const ram_file_cfg_t *cfg, ram_file_t **ram_file_mngr);

/// @brief Destroying the ram file manager object
/// @param ram_file_mngr Ram file manager
/// @return Success (0) or an error code
int ram_file_destroy(ram_file_t *ram_file_mngr);

/// @brief Read data from memory
/// @param ram_file_mngr Ram file manager
/// @param data_out Where extracted data will be stored
/// @param data_size Size of one data chink to read
/// @param data_count Number of data chunks to read
/// @param data_offset Offset in memory from which the data will be read 
/// @return Success (0) or an error code
int ram_file_read(ram_file_t *ram_file_mngr, uint8_t *data_out, size_t data_size, size_t data_offset);

/// @brief Wrtie data to memory
/// @param ram_file_mngr ram file manager object
/// @param data data chunks to write
/// @param data_size size of data chunk
/// @param data_count number of data chunks
/// @param data_offset Offset in memory from which the data will be stored
/// @return Success (0) or an error code
int ram_file_write(ram_file_t *ram_file_mngr, const uint8_t *data, size_t data_size, size_t data_offset);


#ifdef __cplusplus
}
#endif