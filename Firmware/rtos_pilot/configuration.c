/*! 
 *  Contains the memory allocation and method implementations for the configuration struct.
 *
 *  This code handles the reading and writing of the configuration struct to the dataflash chip.
 
 *  @file     configuration.c
 *  @author   Tom Pycke
 *  @date     24-dec-2009
 *  @since    0.1
 */
 
// Gluonpilot library includes
#include "microcontroller/microcontroller.h"
#include "dataflash/dataflash.h"
#include "gps/gps.h"

// rtos_pilot includes
#include "sensors.h"
#include "communication.h"
#include "configuration.h"

//! Memory allocation for the configuration data.
struct Configuration config;

//! Contains the hardware version (defined in configuration.h)
int HARDWARE_VERSION;


/*!
 *  Loads the configuration struct from the 1st dataflash page.
 *  @todo  Add global min and max value for the output.
 */
void configuration_load()
{
	dataflash.read(CONFIGURATION_PAGE, sizeof(struct Configuration), (unsigned char*)&config);
}


void configuration_determine_hardware_version()
{
	// output
	TRISGbits.TRISG14 = 0;
	// input
	TRISGbits.TRISG12 = 1;
	TRISGbits.TRISG13 = 1;
    TRISEbits.TRISE2 = 1;
	
	// are they connected? --> v0.1n or newer
	PORTGbits.RG14 = 1;
	microcontroller_delay_us(10);
	if (PORTGbits.RG12 == 1)
	{
		PORTGbits.RG14 = 0;
		microcontroller_delay_us(10);
		if (PORTGbits.RG12 == 0)
		{
			//printf("RG12 and RG14 connected!\r\n");

			if (PORTGbits.RG13 == 0)
			{
				PORTGbits.RG14 = 1;
				microcontroller_delay_us(10);
				if (PORTGbits.RG13 == 1)
                {
                    if (PORTEbits.RE2 == 1)
                    {
                        PORTGbits.RG14 = 0;
                        microcontroller_delay_us(10);
                        if (PORTEbits.RE2 == 0)
                            HARDWARE_VERSION = V01Q;
                    } else
                        HARDWARE_VERSION = V01O;
                }
				else
					HARDWARE_VERSION = V01N;
			}
		}	
		else
			HARDWARE_VERSION = V01J;
	} 
	else
	{
		HARDWARE_VERSION = V01J;
	}	
}	


/*!
 *  Writes the configuration struct to the 1st dataflash page.
 */
void configuration_write()
{
	dataflash.write(CONFIGURATION_PAGE, sizeof(struct Configuration), (unsigned char*)&config);
}


/*!
 *  Called on user request. Usefull when an upgrade causes the "struct config" to change.
 */
void configuration_default()
{
	int i;
	config.control.channel_ap = 3;
	config.control.channel_motor = 2;
	config.control.channel_pitch = 0;
	config.control.channel_roll = 1;
	config.control.channel_yaw = 4;
	
	for (i = 0; i < 8; i++)
		config.control.channel_neutral[i] = 1500;
		
	config.control.manual_trim = 1;
	config.control.reverse_servo1 = 0;
	config.control.reverse_servo2 = 0;
	config.control.reverse_servo3 = 0;
	config.control.reverse_servo4 = 0;
	config.control.reverse_servo5 = 0;
	config.control.reverse_servo6 = 0;
	
	config.control.manual_trim = 0;
	config.control.use_pwm = 1;
	
	for (i = 0; i < 6; i++)
	{
		config.control.servo_max[i] = 2000;
		config.control.servo_min[i] = 1000;
		config.control.servo_neutral[i] = 1500;
	}
	
	config.control.aileron_differential = 0;
	config.control.cruising_speed_ms = 12;
	config.control.max_pitch = 20.0/180.0*3.14;
	config.control.min_pitch = -10.0/180.0*3.14;
	config.control.max_roll = 40.0/180.0*3.14;
	config.control.servo_mix = AILERON;
	
	config.control.stabilization_with_altitude_hold = 0;
    config.control.auto_throttle_cruise_pct = 90;
    config.control.auto_throttle_min_pct = 30;
    config.control.auto_throttle_max_pct = 100;
    config.control.auto_throttle_p_gain = 8;  // 0.8
        

	pid_init(&config.control.pid_heading2roll, 0.0, 0.7, 0.0, -1.0, 1.0, 0.0);
	pid_init(&config.control.pid_pitch2elevator , 0.0, 0.7, 0.0, -1.0, 1.0, 0.0);
	pid_init(&config.control.pid_roll2aileron, 0.0, 0.5, 0.0, -1.0, 1.0, 0.0);
	pid_init(&config.control.pid_altitude2pitch, 0.0, 0.03, 0.0, -1.0, 1.0, 0.0);
	
	config.control.waypoint_radius_m = 30;
	
    config.control.autopilot_auto_throttle = 0;   // disable auto throttle

    config.control.altitude_mode = PRESSURE;
    config.gps.operational_baudrate = 115200l;
    config.gps.initial_baudrate = 38400l;
    config.gps.enable_waas = 0;
    config.sensors.acc_x_neutral = 32000;
	config.sensors.acc_y_neutral = 32000;
	config.sensors.acc_z_neutral = 32000;
	
	config.sensors.gyro_x_neutral = 27180.0f;
	config.sensors.gyro_y_neutral = 26304.0f;
	config.sensors.gyro_z_neutral = 31850.0f;

    config.sensors.imu_rotated = ROTATION_0;
    config.sensors.neutral_pitch = 0.0f;

	config.telemetry.stream_GpsBasic = 5;
	config.telemetry.stream_GyroAccProc = 40;
	config.telemetry.stream_GyroAccRaw = 30;
	config.telemetry.stream_PPM = 60;
	config.telemetry.stream_PressureTemp = 50;
	config.telemetry.stream_Attitude = 5;
	config.telemetry.stream_Control = 10;

    config.osd.show_altitude = 1;
    config.osd.show_arrow_home = 1;
    config.osd.show_artificial_horizon = 1;
    config.osd.show_block_name = 1;
    config.osd.show_current = 1;
    config.osd.show_distance_home = 1;
    config.osd.show_flight_time = 1;
    config.osd.show_gps_status = 1;
    config.osd.show_mah = 1;
    config.osd.show_mode = 1;
    config.osd.show_rc_link = 1;
    config.osd.show_speed = 1;
    config.osd.show_vario = 0;
    config.osd.show_voltage1 = 1;
    config.osd.show_voltage2 = 0;
    config.osd.show_block_name = 1;

    config.osd.rssi = None;
    config.osd.voltage_low = 30;
    config.osd.voltage_high = 80;
}
