#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct ram_file_t ram_file_t;

/**
 * @brief Configuration structure for a RAM-backed file.
 *
 * This structure defines the configuration parameters for a RAM-based file 
 * system that uses a backing store.
 */
typedef struct {
    /**
     * @brief  Path to the file used as backing store.
     */
    const char *file_path;
    /**
     * @brief Maximum size of the internal buffer.
     */
    size_t buffer_size;
    /**
     * @brief Maximum allowed file size.
     */
    size_t max_file_size;
    /**
     * @brief delete the saved files when destory function is called
     */
    uint8_t delete_files;   
} ram_file_cfg_t;

/**
 * @brief Creating the ram file manager object so it can be used to write or read data
 * @param cfg Configuration data set by the user
 * @param ram_file_mngr Will point to the created ram file manager.
 * @return Success (0) or an error code
 */
int ram_file_create(const ram_file_cfg_t *cfg, ram_file_t **ram_file_mngr);

/**
 * @brief Destroying the ram file manager object
 * @param ram_file_mngr Ram file manager
 * @return Success (0) or an error code
 */
int ram_file_destroy(ram_file_t *ram_file_mngr);

/**
 * @brief Read data from memory
 * @param ram_file_mngr Ram file manager
 * @param offset Offset in memory from which the data will be read
 * @param data_out Where extracted data will be stored
 * @param data_size Size of one data chunk to read
 * @return Success (0) or an error code
 */
int ram_file_read(ram_file_t *ram_file_mngr, size_t offset, uint8_t *data_out, size_t data_size);

/**
 * @brief Wrtie data to memory
 * @param ram_file_mngr ram file manager object
 * @param offset Offset in memory from which the data will be stored
 * @param data data chunks to write
 * @param data_size size of data chunk
 * @return Success (0) or an error code
 */
int ram_file_write(ram_file_t *ram_file_mngr, size_t offset, const uint8_t *data, size_t data_size);


#ifdef __cplusplus
}
#endif