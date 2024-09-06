#include "myi2c.h"
#include "sensor_mpu6050.h"



/*
    sensor:mpu6050 
*/
extern mpu6050_handle_t mpu6050;
void sensor_mpu6050_init(void);

/*
    sensor->task
*/
void sensor_task(void *arg);
