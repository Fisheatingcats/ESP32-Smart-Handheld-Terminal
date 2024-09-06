#include "sensor.h"

static const char *TAG = "sensor";

/*
    sensor->MPU6050 
*/
mpu6050_handle_t mpu6050=NULL;
void sensor_mpu6050_init(void)
{
    esp_err_t ret;
    mpu6050 = mpu6050_create(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS);
    if(mpu6050==NULL)
    {
        ESP_LOGE(TAG, "MPU6050 create failed");
    }
    
    ret = mpu6050_config(mpu6050, ACCE_FS_4G, GYRO_FS_500DPS);
    if(ret!=ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 create failed");
    }
    ret = mpu6050_wake_up(mpu6050);
    if(ret!=ESP_OK)
    {
        ESP_LOGE(TAG, "MPU6050 create failed");
    }
}

/*
    sensor task
*/
void sensor_task(void *arg)
{
    esp_err_t ret;
    uint8_t mpu6050_deviceid;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    ret=mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    ESP_LOGI(TAG, "mpu6050_deviceid:0x%x\n", mpu6050_deviceid);
    while (1)
    {
        ret = mpu6050_get_acce(mpu6050, &acce);
        ESP_LOGI(TAG, "acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);
        
        // ret = mpu6050_get_gyro(mpu6050, &gyro);
        // ESP_LOGI(TAG, "gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

        // ret = mpu6050_get_temp(mpu6050, &temp);
        // ESP_LOGI(TAG, "t:%.2f \n", temp.temp);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    mpu6050_delete(mpu6050);
    ret = i2c_driver_delete(I2C_MASTER_NUM);
    vTaskDelete(NULL);
}