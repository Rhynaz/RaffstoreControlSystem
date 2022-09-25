#pragma once

#include "esp_types.h"

typedef enum update_error
{
    UPDATE_OK,
    UPDATE_FAILED,
    UPDATE_ERR_IMAGE_TOO_LARGE,
    UPDATE_ERR_NO_PARTITION_FOUND,
    UPDATE_ERR_VALIDATION_FAILED,
    UPDATE_ERR_CHANGE_BOOT_FAILED,
} update_error_t;

update_error_t update_prepare();
update_error_t update_write(void *buffer, size_t buffer_size, size_t *remaining_size);
update_error_t update_finish();

void update_abort();
void update_mark_valid();

__attribute__((noreturn)) void update_reboot();
