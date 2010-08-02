/*! 
 *  Reads raw sensor data and convert it to usefull data.
 *
 *  This uses the dsPic's ADC module to sample the sensor's analog data. After
 *  processing, usefull pitch, roll, yaw and angular rates are available.
 *  Sensors read:
 *     - 3-axis accelerometer  
 *     - 2-axis gyro + 1-axis gyro
 *     - SCP1000 for pressure and temperature (-> height)
 *     - GPS (separate task)
 *
 *  This is then all stored in global sensor_data variable.
 *
 *  @file     sensors.c
 *  @author   Tom Pycke
 *  @date     24-dec-2009
 *  @since    0.1
 */
 
 #include <math.h>
 
// Include all FreeRTOS header files
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/croutine.h"
#include "FreeRTOS/semphr.h"

// Include all gluon libraries
#include "scp1000/scp1000.h"
#include "adc/adc.h"
#include "gps/gps.h"
#include "button/button.h"
#include "pid/pid.h"
#include "uart1_queue/uart1_queue.h"
#include "led/led.h"

#include "sensors.h"
#include "configuration.h"
#include "ahrs.h"
#include "navigation.h"


//! Contains all usefull (processed) sensor data
struct SensorData sensor_data;

// Usefull constants
#define G 9.81
#define RAD2DEG (180.0/3.14159)
static const float acc_value_g = 6600.0;

extern xSemaphoreHandle xSpiSemaphore;

#define INVERT_X -1.0   // set to -1 if front becomes back

/*!
 *   FreeRTOS task that reads all the sensor data and stored it in the
 *   sensor_data struct.
 *
 *   The current execution rate is 100Hz.
 */
 
void sensors_task( void *parameters )
{
	unsigned int temperature_10 = 200;
	float last_height = 0.0;
	double dt_since_last_height = 0.0;

		
	/* Used to wake the task at the correct frequency. */
	portTickType xLastExecutionTime; 

	uart1_puts("Sensors task initializing...");
	adc_open();	
	scp1000_init();
	ahrs_init();
	uart1_puts("done\r\n");
	
	/* Initialise xLastExecutionTime so the first call to vTaskDelayUntil()	works correctly. */
	xLastExecutionTime = xTaskGetTickCount();

	for( ;; )
	{
#ifdef ENABLE_QUADROCOPTER
		vTaskDelayUntil( &xLastExecutionTime, ( ( portTickType ) 4 / portTICK_RATE_MS ) );   // 250Hz
		dt_since_last_height += 0.004;
#else
		vTaskDelayUntil( &xLastExecutionTime, ( ( portTickType ) 20 / portTICK_RATE_MS ) );   // 50Hz
		dt_since_last_height += 0.02;
#endif
		if (scp1000_dataready())
		{
			// this should be at 9Hz ->0.11s
			if (xSemaphoreTake( xSpiSemaphore, ( portTickType ) 0 ))  // Spi1 is shared with SCP1000 and Dataflash
			{
				sensor_data.pressure = scp1000_get_pressure();
				sensor_data.temperature = scp1000_get_temperature();
				xSemaphoreGive( xSpiSemaphore );
			}	
			temperature_10 = (unsigned int)sensor_data.temperature * 10;
			float height = scp1000_pressure_to_height(sensor_data.pressure, sensor_data.temperature);
			if (height > -30000.0 && height < 30000.0)   // sometimes we get bad readings ~ -31000
				sensor_data.pressure_height = height;
			sensor_data.vertical_speed = sensor_data.vertical_speed * 0.5 + (sensor_data.pressure_height - last_height)/dt_since_last_height * 0.5;
			last_height = sensor_data.pressure_height;
			dt_since_last_height = 0.0;
		}
		
		
		// read accelerometer data 
		sensor_data.acc_x_raw = adc_get_channel(6);
		sensor_data.acc_z_raw = adc_get_channel(1);
		sensor_data.acc_y_raw = adc_get_channel(0);

		sensor_data.gyro_x_raw = adc_get_channel(4);
		sensor_data.gyro_y_raw = adc_get_channel(7);
		sensor_data.gyro_z_raw = adc_get_channel(5);  //*0.6 = 3V max

		sensor_data.idg500_vref = adc_get_channel(3);
				
		adc_start();  // restart ADC sampling to make sure we have our samples on the next loop iteration.

		// scale to "g" units. We prefer "g" over SI units (m/s^2) because this allows to cancel out the gravity constant as it is "1"
		sensor_data.acc_x = ((double)(sensor_data.acc_x_raw) - (double)config.sensors.acc_x_neutral) / (-acc_value_g*INVERT_X);
		sensor_data.acc_y = ((double)(sensor_data.acc_y_raw) - (double)config.sensors.acc_y_neutral) / (-acc_value_g*INVERT_X);
		sensor_data.acc_z = ((double)(sensor_data.acc_z_raw) - (double)config.sensors.acc_z_neutral) / (-acc_value_g);
				
		// scale to rad/sec
		sensor_data.p = ((double)(sensor_data.gyro_x_raw)-config.sensors.gyro_x_neutral) * (-0.02518315*3.14159/180.0 * INVERT_X);  // 0.02518315f
		sensor_data.q = ((double)(sensor_data.gyro_y_raw)-config.sensors.gyro_y_neutral) * (-0.02538315*3.14159/180.0 * INVERT_X);
		sensor_data.r = ((double)(sensor_data.gyro_z_raw)-config.sensors.gyro_z_neutral) * (0.0062286*3.14159/180.0);  //(2^16-1 - (2^5-1)) / 3.3 * 0.0125*(22)/(22+12)		
	
		// x = (Pitch; Roll)'
#ifdef ENABLE_QUADROCOPTER
		ahrs_filter(0.005);	
#else
		ahrs_filter(0.02);	
#endif
	}
}


//! This semaphore is set in the uart2 interrupt routine when a new GPS message arrives
xSemaphoreHandle xGpsSemaphore = NULL; 


#define LONG_TIME 0xffff
void sensors_gps_task( void *parameters )
{
	int i = 0;
	uart1_puts("Gps task initializing...");
	sensor_data.gps.status = EMPTY;	
	uart1_puts("done\r\n");
	
	navigation_init ();
	
	portTickType xLastExecutionTime = xTaskGetTickCount();; 
	vTaskDelayUntil( &xLastExecutionTime, ( ( portTickType ) 1000 / portTICK_RATE_MS ) );   // 1s
	
	if (sensor_data.gps.status == EMPTY)
		led2_off();
	else if (sensor_data.gps.status == VOID)
		led2_on();	
	

	for( ;; )
	{
		/* Wait until it is time for the next cycle. */
		if( xSemaphoreTake( xGpsSemaphore, LONG_TIME ) == pdTRUE )
		{
			gps_update_info(&(sensor_data.gps));
			
			// Speed is use for calculating accelerations (in the attitude filter)
			// When the GPS is no longer locked, we don't know the speed -> no reliable attitude
			// Use pre-configured cruising speed as measured speed
			if (sensor_data.gps.satellites_in_view < 4 && 
				sensor_data.gps.speed_ms > 0)
					sensor_data.gps.speed_ms = config.control.cruising_speed_ms;
				
			i++;
			if (i % 2 == 0)
				navigation_update();
			
			if (i % 6 == 0 || (i+1) % 6 == 0 || (i+2) % 6 == 0)  // this is used for both RMC and GGA, so only update every other tick
			{
				if (sensor_data.gps.status == ACTIVE)
					led2_off();
			}	
			else
				led2_on();

		}
	}
}