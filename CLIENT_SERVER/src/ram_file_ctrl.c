#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "ram_file_ctrl.h"

#define DEFAULT_MAX_FILE_SIZE (10 * 1024 * 1024)  // 10 MB
#define DEFAULT_MAX_BUFFER_SIZE (1024)            // 1 KB
#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif


struct ram_file_t {
    FILE *file;
    char *file_path;
    size_t buffer_size;
    size_t max_file_size;
    uint8_t delete_files;  
};

static int delete_file_if_exists(const char *file_path) {
    if (file_path == NULL) {
        return 0;
    }
    // Check if the file exists.
    if (access(file_path, F_OK) == 0) {
        if (remove(file_path) != 0) {
            // Return the negative errno value to indicate failure.
            return -errno;
        }
    }
    return 0;
}

int ram_file_create(const ram_file_cfg_t *cfg, ram_file_t **ram_file_mngr) {

if (cfg == NULL || cfg->file_path == NULL || ram_file_mngr == NULL) {
    return -EINVAL;
}

    ram_file_t *mngr = calloc(1, sizeof(ram_file_t));

    if (mngr == NULL) {
        return -ENOMEM;
    }

    mngr->file_path = strdup(cfg->file_path);
    if (mngr->file_path == NULL) {
        free(mngr);
        return -ENOMEM;//this err because strdup try to alloc mem inside
    }

    mngr->file = fopen(cfg->file_path, "rb+");
    if (!mngr->file) {
        // File might not exist, so create it.
        mngr->file = fopen(cfg->file_path, "wb+");
        if (!mngr->file) {
            free(mngr->file_path);
            free(mngr);
            return -EIO;
            // Handle error: unable to open or create file.
        }
    }

    mngr->max_file_size = (cfg->max_file_size == 0 ? DEFAULT_MAX_FILE_SIZE : cfg->max_file_size);
    mngr->buffer_size   = (cfg->buffer_size == 0 ? DEFAULT_MAX_BUFFER_SIZE : cfg->buffer_size);   
    mngr->delete_files = cfg->delete_files;

    (*ram_file_mngr) = mngr;
    return 0;
}

int ram_file_destroy(ram_file_t *ram_file_mngr) {

    if (ram_file_mngr == NULL) {
        return -EINVAL;
    }

    if (ram_file_mngr->file != NULL) {
        fclose(ram_file_mngr->file);
    }

    int ret = 0;
    if (ram_file_mngr->file_path != NULL && ram_file_mngr->delete_files) {
        ret = delete_file_if_exists(ram_file_mngr->file_path);
        free(ram_file_mngr->file_path);
    }

    free(ram_file_mngr);

    return ret;
}

int ram_file_read(ram_file_t *ram_file_mngr, size_t offset, uint8_t *data_out, size_t data_size) {

    if (unlikely(ram_file_mngr == NULL || ram_file_mngr->file == NULL)) {
        return -EINVAL;
    }

    if (unlikely(data_out == NULL || data_size == 0)) {
        return -EINVAL; //change to a proper error
    }

    if (unlikely((offset + data_size) > ram_file_mngr->max_file_size)) {
        // The read would exceed the allowed maximum file size.
        return -EFBIG;
    }    

    if (unlikely(fseek(ram_file_mngr->file, (long)offset, SEEK_SET) != 0)) {
        return -EIO;
    }

    size_t num_of_bytes = fread(data_out, 1, data_size, ram_file_mngr->file);

    if (unlikely(num_of_bytes != data_size)) {
        // Optionally free the allocated memory if a full read is required.
        // free(*data_out);
        return -EIO; // Or return a different error code.
    }

    return 0;
}

int ram_file_write(ram_file_t *ram_file_mngr, size_t offset, const uint8_t *data, size_t data_size) {

    if (unlikely(ram_file_mngr == NULL || ram_file_mngr->file == NULL)) {
        return -EINVAL;
    }

    if (unlikely(data == NULL || data_size == 0)) {
        return -EINVAL;
    }

    size_t write_end = offset + data_size;
    if (unlikely(write_end > ram_file_mngr->max_file_size)) {
        // The write would exceed the allowed maximum file size.
        return -EFBIG;
    }    

    if(unlikely(fseek(ram_file_mngr->file, offset, SEEK_SET) != 0)) {
        //cant find the data in the offset
        return -errno;
    }

    size_t num_of_bytes = fwrite(data, 1, data_size, ram_file_mngr->file);
    if (unlikely(num_of_bytes != data_size)) {
        return -EIO;
    }

   // Ensure immediate persistence
    if (unlikely(fflush(ram_file_mngr->file) != 0)) {
        return -errno;  // Return specific fflush error
    }

    return 0;
}
