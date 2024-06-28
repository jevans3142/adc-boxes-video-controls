// Storage module
//-----------------------------------

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#include "pindefs.h"
#include "storage.h"


static const char *TAG = "storage";
sdmmc_card_t *card;

static esp_err_t init_sd_card(void)
{
    ESP_LOGI(TAG, "Initializing SD card - using 1 line SDMMC");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Initializes the slot in 1 bit MMC mode without CD/WP
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SD card mounted");
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;

}

static void deinit_sd_card(void)
{
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret != ESP_OK) 
    {
        if (ret == ESP_ERR_INVALID_ARG) 
        {
            ESP_LOGE(TAG, "Failed to unmount SD card - Card unregistered.");
        } else {
            ESP_LOGE(TAG, "Failed to unmount SD card - Invalid state, no mount");
        }
        return;
    }
    ESP_LOGI(TAG, "SD card unmounted");
}

static esp_err_t read_config_file(const char *path, struct Settings_Struct *settings)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open settings file for reading");
        return ESP_FAIL;
    }

    char line[MAX_CHAR_SIZE];

    while (fgets(line, sizeof(line), f) != NULL)
    {
        // Strip newline
        char *pos = strchr(line, '\n');
        if (pos) 
        {
            *pos = '\0';
        }

        // Ignore comments and blanks
        if ((strlen(line)>1 && line[0] == '/' && line[1] == '/') || (line[0] == '\0'))
        {
            continue;
        }

        // Now go through the various variables and settings to parse config file

        ESP_LOGI(TAG, "Read line from file: '%s'", line);

        char *equalssplit;
        equalssplit = strtok(line, "="); // First get the variable name before the equals


        // Go through routing panel sources
        if (strncmp(equalssplit, "routing_sources", strlen("routing_sources")) == 0)
        {   
            equalssplit = strtok(NULL, "="); // Get the post equals sign bits - need to split on commas

            char *commasplit;
            commasplit = strtok(equalssplit, ","); // Get the first source

            for (uint8_t button = 0; button<6; button++)
            {
                if (commasplit == NULL)
                {
                    // Check that we haven't run out of numbers due to a formatting error in the config file...
                    ESP_LOGW(TAG, "Formatting error in routing_sources values");
                    continue;
                }

                // TODO: Check for valid return from atoi? 
                settings->routing_sources[button] = (uint8_t) atoi(commasplit);
                commasplit = strtok(NULL, ",");
            }
            ESP_LOGI(TAG,"Read in sources");
            continue;
        }
        
        // Go through routing destination
        if (strncmp(equalssplit, "routing_destination", strlen("routing_destination")) == 0)
        {   
            equalssplit = strtok(NULL, "="); // Get the post equals sign bits

            if (equalssplit == NULL)
            {
                // Check that we haven't run out of number due to a formatting error in the config file...
                ESP_LOGW(TAG, "Formatting error in routing_destination value");
                continue;
            }

            // TODO: Check for valid return from atoi? 
            settings->routing_destination = (uint8_t) atoi(equalssplit);

            ESP_LOGI(TAG,"Read in destination");
            continue;
        }

        // Router properties
        if (strncmp(equalssplit, "router_ip", strlen("router_ip")) == 0)
        {   
            // IP address
            equalssplit = strtok(NULL, "="); // Get the post equals sign bits - need to split on points

            char *pointsplit;
            pointsplit = strtok(equalssplit, "."); // Get the octet

            uint32_t temp_ip = 0;

            for (uint8_t octet = 0; octet<4; octet++)
            {
                if (pointsplit == NULL)
                {
                    // Check that we haven't run out of numbers due to a formatting error in the config file...
                    ESP_LOGW(TAG, "Formatting error in router IP address");
                    continue;
                }

                // TODO: Check for valid return from atoi? 
                temp_ip = temp_ip + ((uint32_t) atoi(pointsplit) << ((3-octet)*8));
                pointsplit = strtok(NULL, ".");
            }

            settings->router_ip = temp_ip;
            ESP_LOGI(TAG,"Read in IP address");
            continue;
        }

        if (strncmp(equalssplit, "router_port", strlen("router_port")) == 0)
        {   
            // Router port
            equalssplit = strtok(NULL, "="); // Get the post equals sign bits

            if (equalssplit == NULL)
            {
                // Check that we haven't run out of number due to a formatting error in the config file...
                ESP_LOGW(TAG, "Formatting error in router port");
                continue;
            }

            // TODO: Check for valid return from atoi? 
            settings->router_port = (uint32_t) atoi(equalssplit);

            ESP_LOGI(TAG,"Read in router port");
            continue;
        }        
    }

    fclose(f);

    return ESP_OK;
}


struct Settings_Struct get_settings(void)
{
    struct Settings_Struct base_settings;

    // First assign some worst case fallback values in case no settings file loads
    for (uint8_t button = 0; button<6; button++)
    {
        base_settings.routing_sources[button] = (button + 1);
    }
    base_settings.routing_destination = 5;

    base_settings.router_ip = 3232238377; //192.168.11.41
    base_settings.router_port = 9990;

    // Get settings from SD card
    esp_err_t ret = init_sd_card();
    if (ret != ESP_OK) 
    {
        ESP_LOGW(TAG, "SD card init fail - no card?");
        return base_settings;
    }

    ret = read_config_file(MOUNT_POINT CFG_FILE, &base_settings);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Settings file read failed, returning fallback values");
        deinit_sd_card();
        return base_settings;
    }

    deinit_sd_card();
    return base_settings;
}