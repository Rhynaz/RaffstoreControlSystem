#include "update.h"

#include "config.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

static const char *const TAG = "Update     ";

static esp_ota_handle_t update_handle;
static const esp_partition_t *update_partition;

update_error_t update_prepare(size_t image_size)
{
    if (image_size > CONFIG_UPDATE_PARTITION_SIZE)
    {
        ESP_LOGE(TAG, "Image too large. (%u of %u KiB)", image_size, CONFIG_UPDATE_PARTITION_SIZE / 1024);
        return UPDATE_ERR_IMAGE_TOO_LARGE;
    }

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL)
    {
        ESP_LOGE(TAG, "Failed to access next OTA flash partition.");
        return UPDATE_ERR_NO_PARTITION_FOUND;
    }

    ESP_LOGI(TAG, "Begin.");
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to begin. (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return UPDATE_FAILED;
    }

    return UPDATE_OK;
}

update_error_t update_write(void *buffer, size_t buffer_size, size_t *remaining_size)
{
    ESP_LOGI(TAG, "Write next %u bytes. (%u KiB remaining)", buffer_size, *remaining_size / 1024);

    esp_err_t err = esp_ota_write(update_handle, buffer, buffer_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to flash partition. (%s)", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return UPDATE_FAILED;
    }

    *remaining_size -= buffer_size;
    return UPDATE_OK;
}

update_error_t update_finish()
{
    ESP_LOGI(TAG, "Finish.");
    esp_err_t err = esp_ota_end(update_handle);
    if (err == ESP_ERR_OTA_VALIDATE_FAILED)
    {
        ESP_LOGE(TAG, "Image validation failed.");
        return UPDATE_ERR_VALIDATION_FAILED;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to finish. (%s)", esp_err_to_name(err));
        return UPDATE_FAILED;
    }

    ESP_LOGI(TAG, "Set boot partition.");
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set boot partition. (%s)", esp_err_to_name(err));
        return UPDATE_ERR_CHANGE_BOOT_FAILED;
    }

    return UPDATE_OK;
}

void update_abort()
{
    esp_ota_abort(update_handle);
}

void update_mark_valid()
{
    esp_ota_img_states_t ota_state;

    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &ota_state) != ESP_OK)
        return;

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGI(TAG, "Mark current image as valid.");
        esp_ota_mark_app_valid_cancel_rollback();
    }
}

__attribute__((noreturn)) void update_reboot()
{
    esp_app_desc_t desc;
    esp_ota_get_partition_description(update_partition, &desc);

    ESP_LOGI(TAG, "Reboot to new firmware. (Version %s)", desc.version);
    esp_restart();
}
