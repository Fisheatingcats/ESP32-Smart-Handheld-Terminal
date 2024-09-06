/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sdcard.h"
#include "driver/gpio.h"
#include "sensor.h"

#include "myspi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

static const char *TAG = "SHT/main";

#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

#if CONFIG_EXAMPLE_LCD_TOUCH_ENABLED
esp_lcd_touch_handle_t tp = NULL;
#endif

void app_main(void)
{
    ESP_LOGI(TAG,"Hello ESP32!");
    gpio_config_t temp_gpioio_config = {
        .pin_bit_mask = (1ULL << 45 ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&temp_gpioio_config);
    gpio_set_level(45,0);


    /*
        初始化i2c总线
    */
    i2c_master_init();
    sensor_mpu6050_init();//初始化mpu6050
    
    sdcard_test();//测试sd卡

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 6, NULL);//创建传感器任务
    
    
    while (1)
    {
        // ESP_LOGI(TAG,"Hello world!");
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



#define BUFFER_SIZE 1024*1024 
int http_downloadtosdcard(void)
{
    int res=0;

    ESP_LOGI(TAG, "NVS init...");
    esp_err_t ret =  nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    mywifi_init();
    wifi_init_sta();

    int64_t content_length = 0;

    //获取文件长度
    esp_err_t err = get_file_length(TEST_URL_GET_PNG_ALL, &content_length);
    if (err != ESP_OK || content_length <= 0) {  
        ESP_LOGE(TAG, "Failed to get file length or invalid length");  
        res = -5;
        return res;  
    }  

    /*Initialize SD card and mount filesystem*/

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = true,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        res = -1;
        return res;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        res = -2;
        return res;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);

    /*Open file */
  
    FILE *file = fopen(MOUNT_POINT"/testpng.png", "w+b");  
    if (file == NULL) {  
        ESP_LOGE(TAG, "Failed to open file for writing");  
        res = -3;
        return res;  
    }
    else
    {
        printf("file %s existed\n", MOUNT_POINT"/testpng.png");
    }  
      
    
    /*Download file in chunks*/
    char * url_down_buff=malloc(sizeof(char)*BUFFER_SIZE);

    esp_http_client_config_t config = {
        .url = TEST_URL_GET_PNG_ALL,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = url_down_buff,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    uint16_t times=0;
    for (int64_t offset = 0; offset < content_length; offset += BUFFER_SIZE) {
        int64_t remaining = content_length - offset;  
        int64_t bytes_to_read = (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining;
        ESP_LOGI(TAG, "http start");

        times++;
        printf("http try times:%d\n",times);//打印下载次数

        // Set Range header  设置请求头
        char range_header[64];  
        snprintf(range_header, sizeof(range_header), "bytes=%" PRIu64 "-%" PRIu64, offset, offset + bytes_to_read - 1);  
        esp_http_client_set_header(client, "Range", range_header); 

        err = esp_http_client_perform(client);//下载文件
        if (err != ESP_OK) {  
            ESP_LOGE(TAG, "Failed to download file chunk"); 
            esp_http_client_cleanup(client);
            offset -= (offset > 0) ? bytes_to_read : 0;  // 避免offset变为负数  
            esp_http_client_cleanup(client);  
            continue; 
        }  

        // 将数据写入文件  
        size_t written = fwrite(url_down_buff, 1, bytes_to_read, file);  
        if (written != bytes_to_read) {  
            ESP_LOGE(TAG, "Failed to write complete data to file");  
            fclose(file);  
            res = -4;
            return res;  
        }  
        ESP_LOGI(TAG, "http stop");
    
    }
    
    fclose(file); 
    
    esp_vfs_fat_sdcard_unmount(mount_point, card);//卸载sd卡
    ESP_LOGI(TAG, "Card unmounted");
    spi_bus_free(host.slot);//释放总线
    esp_http_client_cleanup(client);//结束http

    return 0;
}

