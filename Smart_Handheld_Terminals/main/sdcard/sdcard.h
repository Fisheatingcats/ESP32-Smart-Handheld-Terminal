#pragma once

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define EXAMPLE_MAX_CHAR_SIZE    64
#define MOUNT_POINT "/sdcard"

#define USE_SD_CARD_DEMO

#ifdef USE_SD_CARD_DEMO
// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
// 可通过menuconfig中的"SD SPI Example Configuration"菜单设置引脚分配。
// 也可以直接修改以下4行代码来更改引脚分配。
#define PIN_NUM_MISO  CONFIG_EXAMPLE_PIN_MISO
#define PIN_NUM_MOSI  CONFIG_EXAMPLE_PIN_MOSI
#define PIN_NUM_CLK   CONFIG_EXAMPLE_PIN_CLK
#define PIN_NUM_CS    CONFIG_EXAMPLE_PIN_CS

#else if 

#define PIN_NUM_MISO  42
#define PIN_NUM_MOSI  41
#define PIN_NUM_CLK   40
#define PIN_NUM_CS    48

#endif


esp_err_t s_write_file(const char *path, char *data);
esp_err_t s_read_file(const char *path);

void sdcard_test(void);
