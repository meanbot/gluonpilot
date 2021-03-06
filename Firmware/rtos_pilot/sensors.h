#ifndef SENSORS_H
#define SENSORS_H

#include "hmc5843/hmc5843.h"
#include "gps/gps.h"

enum BoardRotation { ROTATION_0 = 0, ROTATION_90 = 1, ROTATION_180 = 2, ROTATION_270 = 3 };

struct SensorData
{
	unsigned int acc_x_raw, acc_y_raw, acc_z_raw;
	unsigned int gyro_x_raw, gyro_y_raw, gyro_z_raw;
	struct intvector magnetometer_raw;

	unsigned int idg500_vref;
	float acc_x, acc_y, acc_z;
	float p, q, r;
	float roll, pitch, yaw;
	float roll_acc, pitch_acc;
	float vertical_speed; // estimated speed along z axis
	float p_bias, q_bias;  // used in kalman filter

	float pressure;
	float temperature;
	float pressure_height;
	int temperature_10;
	unsigned int battery1_voltage_10;
    float battery1_current;
    float battery1_mAh;
    unsigned int battery2_voltage_10;

	struct gps_info gps;
};

struct SensorConfig
{
	float acc_x_neutral;
	float acc_y_neutral;
	float acc_z_neutral;

	float gyro_x_neutral;
	float gyro_y_neutral;
	float gyro_z_neutral;

    enum BoardRotation imu_rotated;
    float neutral_pitch;
};

extern struct SensorData sensor_data;

#endif // SENSORS_ANALOG_H
